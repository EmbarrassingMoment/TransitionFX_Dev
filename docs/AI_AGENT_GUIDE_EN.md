# Using TransitionFX with AI Coding Agents

TransitionFX ships an `AGENTS.md` file inside the plugin folder (`Plugins/TransitionFX/AGENTS.md`) written for AI coding agents such as Claude Code, Codex, Cursor, GitHub Copilot, and Gemini CLI. It summarizes the public Blueprint/C++ API, the bundled presets, common patterns, and the rules an agent should follow when wiring transitions into your project. A `CLAUDE.md` next to it imports the same file for Claude Code.

This page explains how to make sure your agent actually reads it.

## Where is the plugin?

| How you installed | Plugin location | Does the agent see `AGENTS.md` automatically? |
| :--- | :--- | :--- |
| Copied into your project (`YourProject/Plugins/TransitionFX`), or added via **Add to Project** from Fab | Inside your project | Usually yes when it works on files in that folder, or searches the plugin. For "how do I use TransitionFX" questions, add the pointer below to be safe. |
| Installed to the engine from the Epic Games Launcher / Fab | Under the engine, e.g. `Engine/Plugins/Marketplace/TransitionFX` | **No.** Agents only read files inside your project. Add the pointer below. |

## Recommended: add a pointer to your project's agent instructions

Paste the following into the instruction file your tool reads at the root of **your** project. Adjust the path to where the plugin lives.

```markdown
## TransitionFX (screen transition plugin)

This project uses the TransitionFX plugin. Before writing or changing any screen-transition,
fade, loading-screen, or level-change code, read the plugin's agent guide:

- Project install: `Plugins/TransitionFX/AGENTS.md`
- Engine install:  `<EngineDir>/Engine/Plugins/Marketplace/TransitionFX/AGENTS.md`
  (online copy: https://github.com/EmbarrassingMoment/TransitionFX/blob/main/Plugins/TransitionFX/AGENTS.md)

Use the bundled `DA_*` / `DA_Widget_*` presets; do not edit assets inside the plugin folder.
```

File names by tool:

| Tool | File at your project root |
| :--- | :--- |
| Claude Code | `CLAUDE.md` (you can also write just `@Plugins/TransitionFX/AGENTS.md` on its own line to import the whole file) |
| OpenAI Codex, Cursor, Gemini CLI, Amp, Jules, and others | `AGENTS.md` |
| GitHub Copilot | `AGENTS.md`, or `.github/copilot-instructions.md` |
| Cursor (rules) | `.cursor/rules/transitionfx.mdc` with `alwaysApply: true` |

If your tool supports both `AGENTS.md` and its own file, `AGENTS.md` is enough.

## What the agent guide covers

- Data flow: Blueprint library → `TransitionManagerSubsystem` → `PostProcessTransitionEffect` / `WidgetTransitionEffect` → `Progress` material parameter
- Every Blueprint node and subsystem function with its pins and behavior
- The `TransitionPreset` properties and `FTransitionParameters` overrides
- The list of bundled PostProcess and widget-layer presets, and which effects have no widget-layer variant
- Patterns: fade out / fade in, loading screen with `bHoldAtMax`, level change, shader warmup, sequences
- Troubleshooting table (black screen, no input, UI still visible, missing nodes)
- Rules: prefer bundled presets, never edit plugin assets, choose the effect layer deliberately

## Tips

- Ask the agent to **read the guide first** when starting transition work, e.g. "Read Plugins/TransitionFX/AGENTS.md, then add a fade-to-black level change to the main menu."
- The guide is in English only. Agents handle it fine regardless of the language you prompt in.
- The human-readable docs ([Quick Start](QUICKSTART_EN.md), [API Reference](API_Reference_EN.md), [FAQ](FAQ_EN.md)) remain the full reference; the agent guide is a condensed version tuned for tools.
- If the agent proposes creating a new transition material from scratch, that is usually unnecessary. Point it at the preset list in the guide.
