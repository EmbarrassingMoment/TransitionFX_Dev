# AIコーディングエージェントでTransitionFXを使う

TransitionFXのプラグインフォルダには、AIコーディングエージェント向けの `AGENTS.md`（`Plugins/TransitionFX/AGENTS.md`）が同梱されています。Claude Code、Codex、Cursor、GitHub Copilot、Gemini CLI などが読む想定で、公開Blueprint/C++ API、同梱プリセット、よく使うパターン、エージェントが守るべきルールをまとめています。隣の `CLAUDE.md` はClaude Code用に同じファイルを取り込むだけのものです。

このページでは、お使いのエージェントに確実にこのファイルを読ませる方法を説明します。

## プラグインはどこにあるか

| インストール方法 | プラグインの場所 | エージェントは自動で `AGENTS.md` を読むか |
| :--- | :--- | :--- |
| プロジェクトにコピー（`YourProject/Plugins/TransitionFX`）、またはFabの **Add to Project** | プロジェクト内 | そのフォルダ内のファイルを触るときや、プラグインを検索したときは概ね読みます。「TransitionFXの使い方」系の依頼で確実にしたいなら下の参照を追加してください。 |
| Epic Games Launcher / Fab からエンジンにインストール | エンジン側（例: `Engine/Plugins/Marketplace/TransitionFX`） | **読みません。** エージェントはプロジェクト内のファイルしか見ないため、下の参照が必須です。 |

## 推奨: 自分のプロジェクトのエージェント指示ファイルに参照を追加する

**自分のプロジェクト**のルートにある、ツールが読む指示ファイルへ次の内容を貼り付けてください。パスはプラグインの実際の場所に合わせて調整します。

```markdown
## TransitionFX (screen transition plugin)

This project uses the TransitionFX plugin. Before writing or changing any screen-transition,
fade, loading-screen, or level-change code, read the plugin's agent guide:

- Project install: `Plugins/TransitionFX/AGENTS.md`
- Engine install:  `<EngineDir>/Engine/Plugins/Marketplace/TransitionFX/AGENTS.md`
  (online copy: https://github.com/EmbarrassingMoment/TransitionFX/blob/main/Plugins/TransitionFX/AGENTS.md)

Use the bundled `DA_*` / `DA_Widget_*` presets; do not edit assets inside the plugin folder.
```

ツール別のファイル名:

| ツール | プロジェクトルートに置くファイル |
| :--- | :--- |
| Claude Code | `CLAUDE.md`（`@Plugins/TransitionFX/AGENTS.md` と1行書くだけでファイル全体を取り込むこともできます） |
| OpenAI Codex、Cursor、Gemini CLI、Amp、Jules など | `AGENTS.md` |
| GitHub Copilot | `AGENTS.md` または `.github/copilot-instructions.md` |
| Cursor（ルール形式） | `.cursor/rules/transitionfx.mdc`（`alwaysApply: true`） |

ツールが `AGENTS.md` と独自ファイルの両方に対応している場合は `AGENTS.md` だけで十分です。

## エージェント向けガイドの内容

- データフロー: Blueprintライブラリ → `TransitionManagerSubsystem` → `PostProcessTransitionEffect` / `WidgetTransitionEffect` → マテリアルパラメータ `Progress`
- すべてのBlueprintノードとサブシステム関数のピンと挙動
- `TransitionPreset` のプロパティと `FTransitionParameters` による上書き
- 同梱のPostProcess版・ウィジェットレイヤー版プリセット一覧と、ウィジェット版が無いエフェクト
- パターン: フェードアウト/イン、`bHoldAtMax` によるローディング画面、レベル遷移、シェーダープリロード、シーケンス
- トラブルシューティング表（画面が黒いまま、入力が効かない、UIが隠れない、ノードが見つからない）
- ルール: 同梱プリセットを優先、プラグイン内アセットを編集しない、エフェクトのレイヤーを意図して選ぶ

## ヒント

- トランジション関連の作業を始めるときは「まず `Plugins/TransitionFX/AGENTS.md` を読んでから、メインメニューにフェードトゥブラックのレベル遷移を追加して」のように**先に読ませる**と確実です。
- ガイドは英語のみです。日本語で指示してもエージェントは問題なく読み取ります。
- 人が読むドキュメント（[クイックスタート](QUICKSTART_JP.md)、[APIリファレンス](API_Reference_JP.md)、[FAQ](FAQ_JP.md)）が完全版で、エージェント向けガイドはツール向けに要約したものです。
- エージェントが新しいトランジションマテリアルをゼロから作ろうとしたら、たいてい不要です。ガイド内のプリセット一覧を示してください。
