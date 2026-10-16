#!/usr/bin/env python3
"""Static agent-readiness test for the TransitionFX plugin.

Judges whether the plugin is easy for an AI agent to use, by applying the *checkable* rules from
Anthropic's own guidance to this repository. No Unreal Engine or model call is needed, so it runs
anywhere Python 3.9+ runs:

    python3 Tests/AIUsability/test_agent_readiness.py -v

Each test name carries a criterion ID whose source is listed in README.md next to this file:

    SKILL-*   Agent Skills authoring best practices and checklist
              https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices
    EVAL-*    `claude plugin eval` suite reference (case/grader formats, stable-signal guidance)
              https://code.claude.com/docs/en/plugin-evals
    TOOL-*    "Writing effective tools for agents" (descriptions as to a new hire, unambiguous
              names, namespacing, actionable errors)
              https://www.anthropic.com/engineering/writing-tools-for-agents
    CTX-*     Claude Code best practices for CLAUDE.md ("would removing this cause Claude to make
              mistakes?" implies what is there must be true)
              https://code.claude.com/docs/en/best-practices

The behavioural half of the judgement is the eval suite beside this file, run with
`claude plugin eval Tests/AIUsability`.
"""
from __future__ import annotations

import filecmp
import json
import re
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
REPO_ROOT = HERE.parents[1]
PLUGIN_ROOT = HERE  # the Claude Code plugin wrapper
SKILL_DIR = PLUGIN_ROOT / "skills" / "using-transitionfx"
SKILL_MD = SKILL_DIR / "SKILL.md"
REFERENCE_DIR = SKILL_DIR / "reference"
EVALS_DIR = PLUGIN_ROOT / "evals"
MANIFEST = PLUGIN_ROOT / ".claude-plugin" / "plugin.json"

UE_PLUGIN_ROOT = REPO_ROOT / "Plugins" / "TransitionFX"
RUNTIME_PUBLIC = UE_PLUGIN_ROOT / "Source" / "TransitionFX" / "Public"
RUNTIME_PRIVATE = UE_PLUGIN_ROOT / "Source" / "TransitionFX" / "Private"
CLAUDE_MD = REPO_ROOT / "CLAUDE.md"
API_REFERENCE = REPO_ROOT / "docs" / "API_Reference_EN.md"

sys.path.insert(0, str(HERE))
import sync_skill_references  # noqa: E402  (same directory)


# --------------------------------------------------------------------------- helpers
def read(path: Path) -> str:
    return path.read_text(encoding="utf-8")


def split_frontmatter(text: str) -> tuple[dict, str]:
    """Parse the YAML subset the plugin-eval and skill formats use.

    Supports `key: value`, single/double-quoted scalars, `[a, b]` flow lists, ints and bools.
    Frontmatter must start on line 1 (a Claude Code skills rule).
    """
    if not text.startswith("---\n"):
        raise ValueError("frontmatter must start on line 1 with '---'")
    end = text.find("\n---", 4)
    if end < 0:
        raise ValueError("frontmatter is not closed with '---'")
    block = text[4:end]
    body = text[end + 4:].lstrip("\n")
    data: dict = {}
    for raw in block.splitlines():
        if not raw.strip() or raw.lstrip().startswith("#"):
            continue
        if ":" not in raw:
            raise ValueError(f"unparseable frontmatter line: {raw!r}")
        key, _, value = raw.partition(":")
        data[key.strip()] = _scalar(value.strip())
    return data, body


def _scalar(v: str):
    if v.startswith("[") and v.endswith("]"):
        inner = v[1:-1].strip()
        return [_scalar(x.strip()) for x in inner.split(",")] if inner else []
    if len(v) >= 2 and v[0] == v[-1] and v[0] in "'\"":
        return v[1:-1]
    if v in ("true", "false"):
        return v == "true"
    if re.fullmatch(r"-?\d+", v):
        return int(v)
    if re.fullmatch(r"-?\d+\.\d+", v):
        return float(v)
    return v


def js_regex_to_python(pattern: str) -> str:
    """Approximate a JavaScript regex as Python `re` for a compile check."""
    return pattern  # the dialect used in this suite is common to both engines


def runtime_public_headers() -> list[Path]:
    return sorted(RUNTIME_PUBLIC.glob("*.h"))


_UFUNC_RE = re.compile(r"^\s*UFUNCTION\s*\((?P<spec>.*)\)\s*$")
_UPROP_RE = re.compile(r"^\s*UPROPERTY\s*\((?P<spec>.*)\)\s*$")


def _preceded_by_comment(lines: list[str], idx: int) -> bool:
    """True when the line before `idx` ends a `/** */` block or is a `//` line."""
    j = idx - 1
    while j >= 0 and not lines[j].strip():
        j -= 1
    if j < 0:
        return False
    prev = lines[j].strip()
    return prev.endswith("*/") or prev.startswith("//")


def _declaration_after(lines: list[str], idx: int) -> str:
    """Join the lines after a UFUNCTION/UPROPERTY macro up to the terminating ';'."""
    out = []
    for k in range(idx + 1, min(idx + 8, len(lines))):
        out.append(lines[k].strip())
        if ";" in lines[k]:
            break
    return " ".join(out)


def collect_reflected() -> dict:
    """Walk the runtime module's public headers and collect UFUNCTION/UPROPERTY/UENUM facts."""
    functions, properties, enums = [], [], []
    for header in runtime_public_headers():
        lines = read(header).splitlines()
        for i, line in enumerate(lines):
            m = _UFUNC_RE.match(line)
            if m:
                decl = _declaration_after(lines, i)
                name_m = re.search(r"\b(\w+)\s*\(", decl)
                functions.append({
                    "header": header.name, "line": i + 1, "spec": m["spec"], "decl": decl,
                    "name": name_m.group(1) if name_m else "?",
                    "documented": _preceded_by_comment(lines, i),
                })
                continue
            m = _UPROP_RE.match(line)
            if m:
                decl = _declaration_after(lines, i)
                name_m = re.search(r"(\w+)\s*(=|;|\{)", decl)
                properties.append({
                    "header": header.name, "line": i + 1, "spec": m["spec"], "decl": decl,
                    "name": name_m.group(1) if name_m else "?",
                    "documented": _preceded_by_comment(lines, i),
                })
        for em in re.finditer(r"UENUM\(BlueprintType\)\s*enum class (\w+)[^{]*\{(.*?)\};", read(header), re.S):
            values = []
            for vline in em.group(2).splitlines():
                vline = vline.strip().rstrip(",")
                if not vline or vline.startswith("//"):
                    continue
                values.append({"value": vline.split()[0], "has_display_name": "DisplayName" in vline})
            enums.append({"header": header.name, "name": em.group(1), "values": values})
    return {"functions": functions, "properties": properties, "enums": enums}


def category_of(spec: str) -> str | None:
    m = re.search(r'Category\s*=\s*"([^"]+)"', spec)
    return m.group(1) if m else None


def blueprint_exposed(functions: list[dict]) -> list[dict]:
    return [f for f in functions if re.search(r"\bBlueprint(Callable|Pure)\b", f["spec"])]


def display_name(camel: str) -> str:
    """Approximate UE's auto display name: 'PlayTransitionAndWait' -> 'Play Transition And Wait'."""
    s = re.sub(r"^b(?=[A-Z])", "", camel)
    return re.sub(r"(?<=[a-z0-9])(?=[A-Z])|(?<=[A-Z])(?=[A-Z][a-z])", " ", s)


def eval_cases() -> list[Path]:
    return sorted(p for p in EVALS_DIR.iterdir() if p.is_dir() and p.name not in ("results", "mocks")
                  and ((p / "prompt.md").exists() or (p / "case.yaml").exists()))


def graders_of(case: Path) -> list[tuple[Path, dict, str]]:
    out = []
    for g in sorted((case / "graders").glob("*.md")):
        fm, body = split_frontmatter(read(g))
        out.append((g, fm, body))
    return out


# --------------------------------------------------------------------------- SKILL-*
class SkillAuthoringRules(unittest.TestCase):
    """Agent Skills best practices: frontmatter limits, size, progressive disclosure, references."""

    @classmethod
    def setUpClass(cls):
        cls.fm, cls.body = split_frontmatter(read(SKILL_MD))

    def test_SKILL_01_frontmatter_has_only_name_and_description(self):
        self.assertEqual(set(self.fm), {"name", "description"},
                         "SKILL.md frontmatter should carry exactly `name` and `description`")

    def test_SKILL_02_name_rules(self):
        name = self.fm["name"]
        self.assertLessEqual(len(name), 64, "name is limited to 64 characters")
        self.assertRegex(name, r"^[a-z0-9-]+$", "name must be lowercase letters, numbers and hyphens")
        for reserved in ("anthropic", "claude"):
            self.assertNotIn(reserved, name, f"name must not contain the reserved word {reserved!r}")
        self.assertEqual(name, SKILL_DIR.name, "skill directory name must match frontmatter `name`")

    def test_SKILL_03_description_rules(self):
        desc = self.fm["description"]
        self.assertTrue(desc.strip(), "description must be non-empty")
        self.assertLessEqual(len(desc), 1024, "description is limited to 1,024 characters")
        self.assertNotRegex(desc, r"<[^>]+>", "description must not contain XML tags")
        self.assertNotRegex(desc, r"^(I|We|You)\b|\bI can\b|\byou can use this\b",
                            "description must be written in third person")
        self.assertRegex(desc, r"\bUse when\b", "description must say when to use the skill (trigger terms)")
        # Always-on cost: name + description is what every session pays for. The skill listing
        # truncates description + when_to_use at 1,536 characters (plugins/measure).
        self.assertLessEqual(len(self.fm["name"]) + len(desc), 1536)

    def test_SKILL_04_body_under_500_lines(self):
        n = len(self.body.splitlines())
        self.assertLess(n, 500, f"SKILL.md body is {n} lines; keep it under 500 and split the rest out")

    def _skill_links(self) -> list[Path]:
        links = []
        for target in re.findall(r"\]\(([^)]+)\)", read(SKILL_MD)):
            if target.startswith(("http://", "https://", "#")):
                continue
            target = target.replace("${CLAUDE_PLUGIN_ROOT}/", "")
            links.append(PLUGIN_ROOT / target)
        return links

    def test_SKILL_05_every_link_in_skill_resolves(self):
        missing = [str(p.relative_to(PLUGIN_ROOT)) for p in self._skill_links() if not p.is_file()]
        self.assertEqual(missing, [], f"SKILL.md links to files that do not exist: {missing}")

    def test_SKILL_06_every_reference_file_is_linked_directly_from_skill(self):
        """'Keep references one level deep from SKILL.md': each bundled file gets a direct link."""
        linked = {p.resolve() for p in self._skill_links()}
        unlinked = [str(p.relative_to(PLUGIN_ROOT)) for p in sorted(REFERENCE_DIR.rglob("*"))
                    if p.is_file() and p.resolve() not in linked]
        self.assertEqual(unlinked, [], f"reference files not linked from SKILL.md (nested only): {unlinked}")

    def test_SKILL_07_long_reference_docs_have_a_table_of_contents(self):
        """Reference files over 100 lines need a TOC near the top so a partial read shows the scope."""
        offenders = []
        for md in sorted(REFERENCE_DIR.rglob("*.md")):
            lines = read(md).splitlines()
            if len(lines) <= 100 or md.name == "CHANGELOG.md":
                continue
            head = "\n".join(lines[:60])
            has_heading = re.search(r"^#+\s*(table of )?contents\b|^#+\s*目次", head, re.I | re.M)
            has_anchor_list = len(re.findall(r"\]\(#[^)]+\)", head)) >= 3
            if not (has_heading or has_anchor_list):
                offenders.append(f"{md.relative_to(REFERENCE_DIR)} ({len(lines)} lines)")
        self.assertEqual(offenders, [], "reference docs longer than 100 lines without a table of contents "
                                        f"(fix in the source docs, then re-sync): {offenders}")

    # Repo files a bundled doc may link to without the bundle carrying them: licence/contributing
    # text and the Japanese twins of the English docs add nothing an agent needs for the task.
    UNBUNDLED_OK = re.compile(r"^(LICENSE|CONTRIBUTING\.md|README\.ja\.md|docs/[A-Za-z_]+_JP\.md)$")

    def test_SKILL_08_relative_links_inside_reference_docs_resolve(self):
        """Links the agent may follow from a reference doc must land on a bundled file."""
        broken = []
        for md in sorted(REFERENCE_DIR.rglob("*.md")):
            for target in re.findall(r"(?<!!)\[[^\]]*\]\(([^)]+)\)", read(md)):
                target = target.split("#", 1)[0].strip()
                if not target or target.startswith(("http://", "https://", "mailto:")):
                    continue
                if (md.parent / target).resolve().exists():
                    continue
                repo_rel = (REPO_ROOT / md.relative_to(REFERENCE_DIR)).parent / target
                if repo_rel.exists() and self.UNBUNDLED_OK.match(str(repo_rel.resolve().relative_to(REPO_ROOT))):
                    continue
                broken.append(f"{md.relative_to(REFERENCE_DIR)} -> {target}")
        self.assertEqual(broken, [], f"links inside bundled docs that do not resolve: {broken}")

    def test_SKILL_09_forward_slashes_only(self):
        """File references use forward slashes. Fenced shell examples for the Win64-only editor are exempt."""
        offenders = []
        for md in [SKILL_MD, *sorted(REFERENCE_DIR.rglob("*.md"))]:
            prose = re.sub(r"```.*?```", "", read(md), flags=re.S)
            if re.search(r"[A-Za-z]:\\|\\(?:Source|Plugins|Content|docs|Config)\\", prose):
                offenders.append(str(md.relative_to(PLUGIN_ROOT)))
        self.assertEqual(offenders, [], f"Windows-style paths found: {offenders}")

    def test_SKILL_10_no_time_sensitive_phrasing_in_skill(self):
        hits = re.findall(r"\b(?:before|after|until|as of|since) (?:19|20)\d{2}\b|\bcurrently\b", self.body, re.I)
        self.assertEqual(hits, [], f"time-sensitive phrasing in SKILL.md (move to an 'old patterns' section): {hits}")

    def test_SKILL_11_bundled_docs_match_the_shipped_docs(self):
        """The skill must describe the plugin as shipped: every copy equals its source file."""
        stale = [str(dst.relative_to(PLUGIN_ROOT)) for _, dst in sync_skill_references.stale()]
        self.assertEqual(stale, [], "stale reference copies; run `python3 Tests/AIUsability/sync_skill_references.py`: "
                                    f"{stale}")

    def test_SKILL_12_skill_names_only_symbols_that_exist(self):
        """Every CamelCase identifier the skill presents as plugin API must appear in the headers."""
        headers_text = "\n".join(read(h) for h in runtime_public_headers())
        known = set(re.findall(r"\b[A-Za-z_]\w+\b", headers_text))
        known |= {"PublicDependencyModuleNames", "BindDynamic", "GetGameInstance", "GetSubsystem", "UFUNCTION",
                  "UObject", "UWorld", "TMap", "FName", "ExpandEnumAsExecs", "BlueprintAssignable", "MustImplement",
                  "OpenLevel", "ServerTravel", "SetPlaySpeed", "Build", "SceneTexture", "Lerp"}
        denied = set(re.findall(r"\bno `([A-Za-z_][A-Za-z0-9_:]*)`", self.body))
        candidates = set(re.findall(r"`([A-Za-z_][A-Za-z0-9_:]*)`", self.body)) - denied
        unknown = sorted(
            c for c in candidates
            if re.match(r"^[A-Z][a-z]+[A-Z]|^[A-Z]{2,}[a-z]|^b[A-Z]|^[EFIU][A-Z]", c)
            and not c.startswith(("DA_", "MI_", "M_", "TransitionFX."))
            and c.split("::")[-1] not in known and c.split("::")[0] not in known
        )
        self.assertEqual(unknown, [], f"SKILL.md mentions identifiers not found in the public headers: {unknown}")


# --------------------------------------------------------------------------- EVAL-*
ALLOWED_PROMPT_KEYS = {"schema_version", "name", "description", "tags", "plugins", "runs", "expected_outcome",
                       "model", "max_turns", "timeout_seconds", "allowed_tools", "append_system_prompt", "env"}
READ_ONLY_TOOLS = {"Read", "Glob", "Grep", "NotebookRead", "Skill", "AskUserQuestion", "Agent", "TodoWrite",
                   "TaskCreate", "TaskGet", "TaskList", "TaskUpdate", "TaskStop"}
GRADER_TYPES = {"regex", "tool_used", "tool_order", "file_exists", "llm", "baseline"}
RESULT_GRADERS = {"regex", "llm", "file_exists", "baseline"}
STEP_GRADERS = {"tool_used", "tool_order"}
TARGETS = {"last_message", "trace", "files", "mock_calls"}


class EvalSuiteShape(unittest.TestCase):
    """`claude plugin eval` reference: the suite must load, and follow the stable-signal guidance."""

    def test_EVAL_01_manifest_is_valid(self):
        data = json.loads(read(MANIFEST))
        self.assertRegex(data["name"], r"^[a-z0-9]+(-[a-z0-9]+)*$", "plugin name should be kebab-case")
        lowered = data["name"].lower()
        self.assertFalse(lowered.startswith(("claude-", "anthropic-", "anthropics-", "cc-plugin-")))
        self.assertNotIn(lowered, {"claude", "anthropic", "anthropics", "claude-code", "claude-mods"})
        for key in ("version", "description", "author"):
            self.assertIn(key, data, f"`claude plugin validate` warns when {key} is missing")
        self.assertTrue(SKILL_MD.is_file(), "skills/<name>/SKILL.md must exist in the default location")

    def test_EVAL_02_at_least_three_cases(self):
        """Skill checklist: 'At least three evaluations created'."""
        self.assertGreaterEqual(len(eval_cases()), 3, "write at least three eval cases")

    def test_EVAL_03_prompt_frontmatter_is_valid(self):
        for case in eval_cases():
            with self.subTest(case=case.name):
                fm, body = split_frontmatter(read(case / "prompt.md"))
                unknown = set(fm) - ALLOWED_PROMPT_KEYS
                self.assertEqual(unknown, set(), "unknown prompt.md frontmatter keys are a load error")
                self.assertTrue(body.strip(), "prompt body must not be empty")
                tools = set(fm.get("allowed_tools", []))
                self.assertTrue(tools <= READ_ONLY_TOOLS, f"non read-only tools need --allow-tools: {tools - READ_ONLY_TOOLS}")
                self.assertIn("Skill", tools, "the case must allow the Skill tool or the plugin can never fire")
                self.assertIn("Read", tools, "the skill points at reference files, so Read must be allowed")
                if "max_turns" in fm:
                    self.assertTrue(1 <= fm["max_turns"] <= 200)
                if "runs" in fm:
                    self.assertTrue(1 <= fm["runs"] <= 50)
                if "timeout_seconds" in fm:
                    self.assertTrue(1 <= fm["timeout_seconds"] <= 3600)
                for plug in fm.get("plugins", []):
                    self.assertTrue((case / plug / ".claude-plugin" / "plugin.json").is_file(),
                                    f"`plugins` entry {plug!r} does not point at a plugin root")

    def test_EVAL_04_prompt_reads_like_a_user_not_a_skill_call(self):
        """Prompts are 'phrased the way a user would type it rather than naming the skill'."""
        for case in eval_cases():
            with self.subTest(case=case.name):
                _, body = split_frontmatter(read(case / "prompt.md"))
                self.assertNotIn("using-transitionfx", body)
                self.assertNotRegex(body, r"\bskill\b", "do not name the skill in the prompt")

    def test_EVAL_05_graders_are_well_formed(self):
        for case in eval_cases():
            gs = graders_of(case)
            with self.subTest(case=case.name):
                self.assertGreaterEqual(len(gs), 1, "a case without graders fails to load")
            for path, fm, body in gs:
                with self.subTest(grader=f"{case.name}/{path.name}"):
                    self.assertIn(fm.get("type"), GRADER_TYPES)
                    if "weight" in fm:
                        self.assertGreater(fm["weight"], 0)
                    if "arm" in fm:
                        self.assertIn(fm["arm"], {"with-only", "both"})
                    t = fm["type"]
                    if t == "regex":
                        self.assertIn("pattern", fm)
                        re.compile(js_regex_to_python(fm["pattern"]))  # raises on a bad pattern
                        self.assertNotIn("(?i)", fm["pattern"], "inline (?i) is unsupported; use `flags: i`")
                        if "match" in fm:
                            self.assertRegex(str(fm["match"]), r"^(not_contains|count:\d+)$")
                        if "target" in fm:
                            self.assertIn(fm["target"], TARGETS)
                    elif t == "tool_used":
                        self.assertIn("tool", fm)
                        if "input_match" in fm:
                            re.compile(fm["input_match"])
                    elif t == "tool_order":
                        self.assertTrue({"before", "after"} <= set(fm))
                    elif t == "file_exists":
                        self.assertIn("path", fm)
                    elif t == "llm":
                        self.assertTrue(body.strip() or fm.get("criteria"), "llm grader needs a rubric body")
                    elif t == "baseline":
                        self.assertTrue((case / fm["baseline_file"]).is_file())

    def test_EVAL_06_each_case_grades_result_and_steps(self):
        """'Give each case one grader on the result ... and one on the steps Claude took.'"""
        for case in eval_cases():
            with self.subTest(case=case.name):
                types = {fm["type"] for _, fm, _ in graders_of(case)}
                self.assertTrue(types & RESULT_GRADERS, "no result grader (regex/llm/file_exists/baseline)")
                self.assertTrue(types & STEP_GRADERS, "no step grader (tool_used/tool_order)")

    def test_EVAL_07_each_case_checks_the_skill_fired(self):
        for case in eval_cases():
            with self.subTest(case=case.name):
                fired = [fm for _, fm, _ in graders_of(case)
                         if fm["type"] == "tool_used" and fm.get("tool") == "Skill"
                         and "using-transitionfx" in fm.get("input_match", "")]
                self.assertTrue(fired, "add a tool_used: Skill grader naming using-transitionfx")

    def test_EVAL_08_llm_rubrics_state_pass_and_fail_conditions(self):
        """Rubrics are 'written as concrete PASS and FAIL conditions'."""
        for case in eval_cases():
            for path, fm, body in graders_of(case):
                if fm["type"] != "llm":
                    continue
                with self.subTest(grader=f"{case.name}/{path.name}"):
                    self.assertRegex(body, r"\bPASS if\b")
                    self.assertRegex(body, r"\bFAIL if\b")

    def test_EVAL_09_each_case_has_a_deterministic_result_grader(self):
        """Keep `llm` graders for short outputs; a free regex grader stabilises every case's score."""
        for case in eval_cases():
            with self.subTest(case=case.name):
                types = [fm["type"] for _, fm, _ in graders_of(case)]
                self.assertIn("regex", types, "add a regex grader so the case does not depend on the judge alone")

    def test_EVAL_10_results_directory_is_ignored(self):
        text = read(PLUGIN_ROOT / ".gitignore")
        self.assertRegex(text, r"(?m)^evals/results/?$")


# --------------------------------------------------------------------------- TOOL-*
class ApiDescriptionQuality(unittest.TestCase):
    """'Writing effective tools for agents' applied to the plugin's reflected (Blueprint) API."""

    @classmethod
    def setUpClass(cls):
        cls.r = collect_reflected()
        cls.bp = blueprint_exposed(cls.r["functions"])
        cls.api_doc = read(API_REFERENCE)

    def test_TOOL_01_every_blueprint_function_has_a_description(self):
        """Describe each tool 'as you would to a new hire': every exposed node carries a doc comment."""
        missing = [f"{f['header']}:{f['line']} {f['name']}" for f in self.bp if not f["documented"]]
        self.assertEqual(missing, [], f"Blueprint-exposed functions without a doc comment: {missing}")

    def test_TOOL_02_every_exposed_property_and_event_has_a_description(self):
        exposed = [p for p in self.r["properties"]
                   if re.search(r"\b(EditAnywhere|BlueprintReadWrite|BlueprintReadOnly|BlueprintAssignable|Config)\b", p["spec"])]
        missing = [f"{p['header']}:{p['line']} {p['name']}" for p in exposed if not p["documented"]]
        self.assertEqual(missing, [], f"exposed properties/events without a doc comment: {missing}")

    def test_TOOL_03_blueprint_functions_are_namespaced_under_transitionfx(self):
        """Namespacing: every node lives under the `TransitionFX` category prefix."""
        bad = [f"{f['header']}:{f['line']} {f['name']} -> {category_of(f['spec'])!r}"
               for f in self.bp if not (category_of(f["spec"]) or "").startswith("TransitionFX")]
        self.assertEqual(bad, [], f"Blueprint functions outside the TransitionFX category: {bad}")

    def test_TOOL_04_parameter_names_are_unambiguous(self):
        """`user_id` beats `user`: flag one/two-letter or generic parameter names on exposed functions."""
        generic = {"value", "data", "obj", "object", "param", "arg", "flag", "val", "tmp", "in", "out", "ptr"}
        bad = []
        for f in self.bp:
            params = re.search(r"\((.*)\)", f["decl"])
            if not params:
                continue
            for p in params.group(1).split(","):
                p = p.strip()
                if not p:
                    continue
                name = re.sub(r"\s*=.*$", "", p).split()[-1].lstrip("&*")
                if len(name) <= 2 or name.lower() in generic:
                    bad.append(f"{f['name']}({name})")
        self.assertEqual(bad, [], f"ambiguous parameter names: {bad}")

    def test_TOOL_05_blueprint_enum_values_have_display_names(self):
        bad = [f"{e['name']}::{v['value']}" for e in self.r["enums"] for v in e["values"] if not v["has_display_name"]]
        self.assertEqual(bad, [], f"BlueprintType enum values without UMETA(DisplayName): {bad}")

    def test_TOOL_06_every_blueprint_function_is_in_the_api_reference(self):
        missing = []
        for f in self.bp:
            if f["name"] not in self.api_doc and display_name(f["name"]) not in self.api_doc:
                missing.append(f"{f['header']} {f['name']}")
        self.assertEqual(missing, [], f"Blueprint-exposed functions absent from docs/API_Reference_EN.md: {missing}")

    def test_TOOL_07_every_event_is_in_the_api_reference(self):
        events = [p["name"] for p in self.r["properties"] if "BlueprintAssignable" in p["spec"]]
        missing = [e for e in events if e not in self.api_doc]
        self.assertEqual(missing, [], f"BlueprintAssignable events absent from the API reference: {missing}")

    def test_TOOL_08_api_reference_headings_name_real_symbols(self):
        """Stale docs mislead an agent: each `####` entry must correspond to a symbol that still exists."""
        headers_text = "\n".join(read(h) for h in runtime_public_headers())
        symbols = set(re.findall(r"\b[A-Za-z_]\w+\b", headers_text))
        sections = re.split(r"(?m)^(?=#{1,4} )", self.api_doc)
        stale = []
        for sec in sections:
            m = re.match(r"#### (.+)", sec)
            if not m:
                continue
            heading = re.sub(r"\(.*?\)", "", m.group(1)).strip()
            if heading.replace(" ", "") in symbols:
                continue
            body_symbols = {s for s in re.findall(r"`([A-Za-z_]\w+)`", sec)} & symbols
            if not body_symbols:
                stale.append(heading)
        self.assertEqual(stale, [], f"API reference entries naming no existing symbol: {stale}")

    def test_TOOL_09_warning_and_error_logs_are_actionable(self):
        """Actionable errors: each Warning/Error log names its origin or interpolates the offending object."""
        bad = []
        for cpp in sorted(RUNTIME_PRIVATE.glob("*.cpp")):
            for m in re.finditer(r'UE_LOG\(\s*LogTransitionFX\s*,\s*(Warning|Error)\s*,\s*TEXT\("([^"]*)"\)', read(cpp)):
                msg = m.group(2)
                has_origin = re.match(r"^\s*(\w+(::\w+)?\s*:|[A-Z][a-z]+[A-Z]\w*\b)", msg) is not None
                has_object = "%s" in msg or "%d" in msg
                if not (has_origin or has_object) or len(msg) < 20:
                    bad.append(f"{cpp.name}: {msg}")
        self.assertEqual(bad, [], f"log messages with neither an origin prefix nor an interpolated object: {bad}")

    def test_TOOL_10_single_log_category(self):
        """One namespace for diagnostics: every log line in the runtime module uses LogTransitionFX."""
        other = []
        for cpp in sorted(RUNTIME_PRIVATE.glob("*.cpp")):
            for m in re.finditer(r"UE_LOG\(\s*(\w+)", read(cpp)):
                if m.group(1) != "LogTransitionFX":
                    other.append(f"{cpp.name}: {m.group(1)}")
        self.assertEqual(other, [], f"log lines outside LogTransitionFX: {other}")


# --------------------------------------------------------------------------- CTX-*
class ClaudeMdAccuracy(unittest.TestCase):
    """CLAUDE.md is loaded into every Claude Code session on this repo; what it asserts must be true."""

    @classmethod
    def setUpClass(cls):
        cls.text = read(CLAUDE_MD)
        cls.r = collect_reflected()

    def _resolve(self, token: str) -> bool:
        if token.startswith(("http", "Project Settings", "/Script/")) or " " in token:
            return True  # URLs, UI paths and shell commands are not file paths
        for base in (REPO_ROOT, RUNTIME_PUBLIC.parent, UE_PLUGIN_ROOT / "Content"):
            if any(base.glob(token)) if ("*" in token) else (base / token).exists():
                return True
        if "/" not in token and "*" in token:
            return any(REPO_ROOT.rglob(token))
        return False

    def test_CTX_01_every_path_mentioned_exists(self):
        tokens = re.findall(r"`([^`\n]+)`", self.text)
        paths = [t for t in tokens
                 if ("/" in t or re.search(r"\.(h|cpp|py|md|yml|uasset|umap|uplugin|uproject)$", t))
                 and not re.search(r"[<>|→]", t) and "::" not in t and "(" not in t]
        missing = sorted({t for t in paths if not self._resolve(t)})
        self.assertEqual(missing, [], f"CLAUDE.md names paths that do not exist: {missing}")

    def test_CTX_02_numeric_inventory_claims_match_the_content_folder(self):
        data_dir = UE_PLUGIN_ROOT / "Content" / "Data"
        actual = {
            "presets": len(list(data_dir.glob("DA_*.uasset"))),
            "widget_presets": len(list(data_dir.glob("DA_Widget_*.uasset"))),
            "material_functions": len(list((UE_PLUGIN_ROOT / "Content" / "MaterialFunctions").glob("MF_*.uasset"))),
            "master_materials": len(list((UE_PLUGIN_ROOT / "Content" / "Materials").glob("M_Transition_*.uasset"))),
        }
        claims = {
            "presets": re.search(r"(\d+) DA_\*\.uasset presets", self.text),
            "widget_presets": re.search(r"(\d+) DA_Widget_\*", self.text),
            "material_functions": re.search(r"(\d+) reusable SDF helper functions", self.text),
            "master_materials": re.search(r"(\d+) of the (\d+) PostProcess effects", self.text),
        }
        for key, m in claims.items():
            with self.subTest(claim=key):
                self.assertIsNotNone(m, f"CLAUDE.md no longer states the {key} count; update this test")
                claimed = int(m.group(2) if key == "master_materials" else m.group(1))
                self.assertEqual(claimed, actual[key], f"CLAUDE.md says {claimed} {key}, repository has {actual[key]}")

    def test_CTX_03_category_convention_claim_matches_the_headers(self):
        m = re.search(r"All `UFUNCTION`/`UPROPERTY` use (.+)", self.text)
        self.assertIsNotNone(m, "CLAUDE.md Conventions no longer states the Category rule; update this test")
        claimed = set(re.findall(r'"([^"]+)"', m.group(1)))
        actual = {category_of(x["spec"]) for x in self.r["functions"] + self.r["properties"] if category_of(x["spec"])}
        self.assertEqual(actual - claimed, set(),
                         f"headers use categories CLAUDE.md does not list: {sorted(actual - claimed)}")

    def test_CTX_04_interface_method_list_is_complete(self):
        iface = read(RUNTIME_PUBLIC / "ITransitionEffect.h")
        methods = re.findall(r"virtual \w+ (\w+)\([^)]*\)\s*=\s*0;", iface)
        row = next((ln for ln in self.text.splitlines() if "`ITransitionEffect`" in ln and "Interface" in ln), "")
        self.assertTrue(row, "CLAUDE.md has no ITransitionEffect row")
        missing = [m for m in methods if f"`{m}`" not in row]
        self.assertEqual(missing, [], f"CLAUDE.md's ITransitionEffect row omits pure-virtual methods: {missing}")

    def test_CTX_05_claude_md_points_at_this_test(self):
        """An agent only runs a check it has been told exists."""
        self.assertIn("Tests/AIUsability", self.text, "CLAUDE.md Testing section should name Tests/AIUsability")


if __name__ == "__main__":
    unittest.main(verbosity=2)
