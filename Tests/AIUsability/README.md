# AI Usability (Agent Readiness) Test for TransitionFX

TransitionFX が **AI エージェントにとって使いやすいか** を判定するためのテスト一式です。
判定基準は自前で考えたものではなく、Anthropic が公開している「AI にとっての使いやすさ」を測る・担保するための文書から、
機械的に検証できる規則だけを取り出して本プラグインに適用しています。

リリース ZIP には含まれません（`.github/workflows/release.yml` で `Tests/` を除外）。

## 基準にした Anthropic の文書

| 文書 | 何を測る / 定めるか | 本テストでの ID |
| :--- | :--- | :--- |
| [Test plugins with evals (`claude plugin eval`)](https://code.claude.com/docs/en/plugin-evals) | プラグインを読み込んだ Claude が **目的の結果に到達する割合** を、プラグイン無し（baseline）と比較して数値化する公式の仕組み。ケース = 現実的なプロンプト + pass/fail の grader。`WITH` / `W/OUT` / `Δ` の 3 スコア。 | `evals/`（挙動テスト）、`EVAL-*`（スイート形式） |
| [Skill authoring best practices](https://platform.claude.com/docs/en/agents-and-tools/agent-skills/best-practices) | Claude が発見・利用しやすい Skill の規則とチェックリスト（`name` 64 文字・小文字/数字/ハイフン、`description` 1,024 文字・三人称・「いつ使うか」、本文 500 行未満、参照は 1 階層、100 行超の参照には目次、スラッシュ区切り、時期依存の記述禁止、評価は 3 件以上）。 | `SKILL-*` |
| [Writing effective tools for agents](https://www.anthropic.com/engineering/writing-tools-for-agents) | エージェント向けツールの設計基準と計測指標（精度、ツール呼び出し数、トークン消費、ツールエラー）。説明は「新入社員に説明するように」、曖昧でないパラメータ名、名前空間（prefix）、実行可能なエラーメッセージ。 | `TOOL-*` |
| [Claude Code best practices](https://code.claude.com/docs/en/best-practices) | `CLAUDE.md` は全セッションに読み込まれる前提知識。「これを消したら Claude が間違えるか」で内容を選ぶ → 書いてあることは正しくなければならない。 | `CTX-*` |

Anthropic には「コードベースの AI 使いやすさスコア」のような単一の数値指標は存在しません（調査時点）。
そのため本テストは **(1) 公式の挙動評価 (`plugin eval`) + (2) 公式規則の静的検証** の二層で判定します。

## 構成

```
Tests/AIUsability/
├── .claude-plugin/plugin.json      ← Claude Code プラグインとしてのラッパー（plugin eval の対象にするため）
├── skills/using-transitionfx/
│   ├── SKILL.md                    ← プラグインの使い方を Claude に渡す Skill（API の要点 + 参照先の案内）
│   └── reference/                  ← README / docs / 公開ヘッダの「そのままのコピー」（sync スクリプトで同期）
├── evals/<case>/                   ← plugin eval のケース（prompt.md + graders/*.md）× 10
├── sync_skill_references.py        ← reference/ を実ドキュメントと同期（--check で差分検出）
├── test_agent_readiness.py         ← 静的テスト（Python 3.9+ 標準ライブラリのみ、UE 不要）
└── README.md                       ← このファイル
```

UE プラグインは Claude Code プラグインではないので、そのままでは `claude plugin eval` にかけられません。
そこで **出荷しているドキュメントと公開ヘッダをそのままコピーした Skill** を薄いラッパーとして用意し、
「このプラグインのドキュメントを渡された Claude が、渡されない Claude よりどれだけ正しく使えるか」を測ります。
コピーが実ファイルとずれると静的テスト `SKILL-11` が失敗するので、測っているのは常に実物のドキュメントです。

## 実行方法

### 1. 静的テスト（無料・数十ミリ秒・CI 向き）

```bash
python3 Tests/AIUsability/test_agent_readiness.py -v
```

ドキュメントやヘッダを変更したら、先に参照コピーを同期してください。

```bash
python3 Tests/AIUsability/sync_skill_references.py
```

### 2. 挙動テスト（`claude plugin eval`、モデル呼び出しあり・課金対象）

前提: Claude Code v2.1.269 以降、git 2.31 以降、通常のセッションと同じ認証。

```bash
# 全ケース（10 ケース × 3 回 × WITH/W/OUT の 2 arm ≈ 60 回の agent 実行 + judge 呼び出し）
claude plugin eval Tests/AIUsability --trust-plugin --threshold 0.8 --no-publish --max-cost-usd 20

# 安い確認（regex 中心の 5 ケース、baseline 無し）
claude plugin eval Tests/AIUsability --tag smoke --ablation none --runs 1 --no-publish

# CI 向け（モデルを固定し JSON を保存、閾値未満なら exit 1）
claude plugin eval Tests/AIUsability --trust-plugin --json results.json --threshold 0.8 \
  --model claude-sonnet-5 --judge-model claude-haiku-4-5 --no-publish --max-cost-usd 20
```

結果は `Tests/AIUsability/evals/results/<timestamp>/`（`aggregate-result.json`, `report.html`）に書かれ、git 管理外です。

## 判定の読み方

**静的テスト**: 全件 `ok` なら、Anthropic の規則のうち機械検証できるものをすべて満たしています。
失敗したテスト名の ID（`SKILL-07` など）で上の表の文書を引けば、根拠と直し方が分かります。

**挙動テスト**: ケースごとに `WITH`（Skill あり）・`W/OUT`（Skill なし）・`Δ` が出ます。

| 観測 | 意味 |
| :--- | :--- |
| `WITH` が閾値（0.8）以上、`Δ` がプラス | ドキュメントを渡せば AI は正しく使える。**AI にとって使いやすい**。 |
| `WITH` が高いが `Δ` ≈ 0 | Claude が元々知っている。ドキュメントは差を生んでいない（害も無い）。 |
| `WITH` が低い | ドキュメント／ヘッダを渡しても正しく使えない。`report.html` の judge の説明を読み、該当ドキュメントを直す。 |
| `skill-fired` が不合格 | Skill がそもそも起動していない。`SKILL.md` の `description` がプロンプトの言い回しに反応していない。 |
| `Δ` がマイナス | ドキュメントが誤誘導している可能性。まず judge を疑い（`--judge-model sonnet`）、次に該当ドキュメントを疑う。 |

各ケースは、公式ガイダンス「結果を見る grader と、過程を見る grader を 1 つずつ」に従い、
`regex`（決定的・無料）+ `llm`（PASS/FAIL 条件を明記したルーブリック）+ `tool_used: Skill`（過程の指標）で構成しています。

| ケース | 何を試すか |
| :--- | :--- |
| `level-change-with-fade` | レベル遷移に `Open Level With Transition` を選べるか（手組みの Open Level + 逆再生に逃げないか） |
| `cover-umg-hud` | UMG を覆うのに `DA_Widget_*` / `WidgetTransitionEffect` を提示できるか |
| `cpp-red-iris-half-second` | C++ で Subsystem 取得、`StartTransition` 引数順、秒 → PlaySpeed 変換、`FadeColor` 上書き、Build.cs |
| `reverse-vs-invert` | Reverse（再生方向）と bInvert（マスク反転）を区別できるか |
| `loading-screen-hold` | `bHoldAtMax` + `ReleaseHold` を使えるか |
| `sequence-cannot-change-level` | シーケンスの制限（レベル遷移不可）を知り、代替を示せるか |
| `first-play-hitch` | `PreloadTransitionPresets` / `AsyncLoadTransitionPresets` を提示できるか |
| `stuck-black-screen` | `ForceClear` / `TransitionFX.ForceClear` と原因（Hold 解除忘れ、入力ブロック）を示せるか |
| `custom-effect-class` | `ITransitionEffect` の 5 メソッドと `EffectClass` への接続を正しく列挙できるか |
| `limits-honesty` | 同時再生・レプリケーション・UEFN・Widget 版 Pixelate に「No」と答えられるか（幻覚ガード） |

## 判定結果（2026-10-16、初回フル実行）

静的テスト: 37 件中 35 件合格（未修正 2 件は上記のとおり、目次の無い長文ドキュメント 3 本と、発生元を含まない Error ログ 1 行）。

挙動テスト（`--threshold 0.8`、各ケース 3 回 × WITH / W/OUT、Claude Code 2.1.301、約 3.0 USD、211 秒）:

| ケース | WITH | W/OUT | Δ |
| :--- | ---: | ---: | ---: |
| cover-umg-hud | 1.00 | 0.00 | +1.00 |
| cpp-red-iris-half-second | 0.93 | 0.07 | +0.87 |
| custom-effect-class | 1.00 | 0.00 | +1.00 |
| first-play-hitch | 1.00 | 0.00 | +1.00 |
| level-change-with-fade | 1.00 | 0.00 | +1.00 |
| limits-honesty | 1.00 | 0.50 | +0.50 |
| loading-screen-hold | 1.00 | 0.00 | +1.00 |
| reverse-vs-invert | 1.00 | 0.83 | +0.17 |
| sequence-cannot-change-level | 1.00 | 0.17 | +0.83 |
| stuck-black-screen | 1.00 | 0.00 | +1.00 |
| **全体** | **0.99** | | **+0.84** |

10 ケースすべてが閾値を超え、Skill は 30 回の WITH 実行すべてで起動しました。
判定: **ドキュメントと公開ヘッダを渡せば、AI はこのプラグインを正しく使える（AI にとって使いやすい）**。
一方 W/OUT がほぼ 0 なので、Claude はこのプラグインを事前知識として持っておらず、
使いやすさは **ドキュメントの質に全面的に依存** しています。ドキュメントを壊すとそのまま使えなくなる、ということでもあります。

唯一の WITH 減点（cpp ケース 3 回中 1 回）は、生成コードは正しいのに judge が `TransitionFXConfig::ColorParamName` を
`FadeColor` と認めなかったもので、ルーブリックを修正済みです。

## 測れないこと

- `plugin eval` の実行は隔離されており、UE エディタを起動できず、`CLAUDE.md` や `.claude/` も読み込まれません。
  測っているのは「ドキュメントとヘッダだけを頼りに正しい API を選び、正しいコード／手順を出せるか」であり、
  実際にエディタ内でアセットを操作できるかではありません。
- `llm` grader は judge モデルの判断なので揺れます。`regex` grader と併用し、3 回実行の平均で見ます。
- 静的テストの一部（ログメッセージの実行可能性、パラメータ名の曖昧さ）はヒューリスティックです。
  失敗した項目は「規則違反の疑い」として読み、妥当なら直し、妥当でなければテスト側の判定を調整してください。

## メンテナンス

- ドキュメント／公開ヘッダを変更したら `sync_skill_references.py` を実行する（忘れると `SKILL-11` が失敗）。
- 公開 API を追加したら、`docs/API_Reference_EN.md` に載せ（`TOOL-06`）、代表的な使い方を 1 ケース `evals/` に追加する。
- `SKILL.md` は 500 行未満を保ち、新しい参照ファイルは必ず `SKILL.md` から直接リンクする（`SKILL-06`）。
