# Fab リリース用サンプルプロジェクト チェックリスト

## Context
TransitionFX プラグイン (UE5用プロシージャル画面遷移システム) を Fab マーケットプレイスにリリースするにあたり、サンプルプロジェクトに含めるべきサンプル内容を洗い出す。Fab では「Example Map」の品質がレビュー通過の重要な要素であり、全機能を分かりやすくデモンストレーションする必要がある。

---

## 1. Overview / Showcase Map (最重要)

- [ ] **Overview Map (`L_TransitionFX_Overview`)** — 全エフェクトを一覧・試遊できるメインデモマップ
  - [ ] 22種の全トランジションエフェクトをボタン/メニューで切り替えて再生できるUI
  - [ ] Forward / Reverse 両モードの切り替えデモ
  - [ ] Easing タイプの比較デモ（同一エフェクトで異なるイージングを並べて確認）
  - [ ] 再生速度の変更デモ（PlaySpeed パラメータ）
  - [ ] Invert（反転）モードのデモ

## 2. 個別エフェクト デモ

- [x] **全22エフェクト + FadeToBlack = 23プリセットの Preset Data Asset が揃っている**
- [ ] 各エフェクトに対応する Material / Material Instance が正常に動作する
- [ ] Overview Map 上で全22エフェクトを実際に再生確認済み

## 3. ユースケース別サンプル

### 3-1. レベル遷移 (Level Transition)
- [ ] **レベル遷移デモマップ** — `OpenLevelWithTransition` を使った Fade Out → レベルロード → Fade In のシームレスな遷移
  - [ ] 遷移元マップ (`L_Transition_From` を整備)
  - [ ] 遷移先マップ (`L_Transition_TO` を整備)
  - [ ] 遷移を開始するUIボタンまたはトリガー

### 3-2. ローディングスクリーン (Loading Screen Workflow)
- [ ] **ローディングスクリーンサンプル** — `bHoldAtMax` + `ReleaseHold` を使ったワークフロー
  - [ ] Fade Out → 画面を覆ったまま保持 → 処理完了後に Fade In の一連の流れ
  - [ ] 擬似ローディング処理（タイマー等）と組み合わせたデモ

### 3-3. ポーズメニュー連携
- [ ] **ポーズ中のトランジション** — `bTickWhenPaused` を使ったポーズメニュー表示/非表示時のトランジション

### 3-4. オーディオ連携
- [ ] **サウンド付きトランジション** — Preset の Transition Sound を設定した再生デモ
  - [ ] サンプル用のSE素材（ライセンスフリー or 自作）

### 3-5. イベント連携
- [ ] **デリゲートのデモ** — `OnTransitionStarted`, `OnTransitionCompleted`, `OnTransitionHoldStarted` を受けてUI表示を更新するサンプル

### 3-6. ランダム選択
- [ ] **ランダムトランジション** — `PlayRandomTransitionAndWait` で複数プリセットからランダムに選択するデモ

### 3-7. パラメータオーバーライド
- [ ] **動的パラメータ変更** — `FTransitionParameters` (ScalarParams, VectorParams, TextureParams) を実行時に変更するデモ
  - [ ] 色の変更サンプル
  - [ ] テクスチャマスクの差し替えサンプル

## 4. Blueprint サンプル

- [ ] **WBP_TransitionShowcase** — Overview Map 用の操作UI Widget
  - [ ] エフェクト選択ドロップダウン/リスト
  - [ ] Forward / Reverse トグル
  - [ ] Easing 選択
  - [ ] 再生ボタン
  - [ ] 速度スライダー
- [ ] **BP_TransitionDemo_LevelChange** — レベル遷移のサンプル Blueprint
- [ ] **BP_TransitionDemo_LoadingScreen** — ローディングスクリーンのサンプル Blueprint
- [ ] **BP_TransitionDemo_PauseMenu** — ポーズメニュー連携のサンプル Blueprint
- [ ] **WBP_LevelTransitionTest** (既存) — 整備・ブラッシュアップ

## 5. UI / ビジュアル

- [ ] Overview Map に操作説明テキスト（英語・日本語）
- [ ] 各デモに簡潔な説明テキスト（何を実演しているか）
- [ ] マップ内にプラグイン名・バージョン表記

## 6. Fab 提出要件 対応

### 6-1. 命名規則 (Naming Conventions)
- [x] 全アセットが UE 命名規則に準拠 (例: `M_`, `MI_`, `T_`, `DA_`, `WBP_`, `BP_`, `A_` 等)
- [ ] オーディオファイルに `A_` プレフィックス (※現在オーディオファイルなし — 追加時に確認)
- [x] マテリアル関数に `MF_` プレフィックス (対応済み)

### 6-2. 未使用ファイルの排除
- [x] プロジェクト内に未使用のアセットが残っていないか確認
- [x] テスト用の一時ファイルを削除

### 6-3. プラグイン設定
- [x] `TransitionFX.uplugin` の Description, DocsURL, SupportURL が正確
- [ ] MarketplaceURL フィールドの設定 (Fab 公開後に更新)

### 6-4. ドキュメント画像 (49/51 完了)
- [ ] `hero_banner.gif` (README 用ヒーローバナー)
- [x] `quickstart_create_data_asset.png`
- [ ] `quickstart_hold_workflow_bp.png`
- [x] `quickstart_forceclear_console.png`

## 7. プリロード / パフォーマンスデモ

- [ ] **シェーダープリロードのデモ** — `PreloadTransitionPresets` / `AsyncLoadTransitionPresets` の使用例
- [ ] 初回再生時のヒッチなしを示すサンプル

## 8. エディタ機能デモ

- [ ] **Preview Tool の使い方ガイド** — エディタ上で Tools > TransitionFX > TransitionFX Preview を開く手順を示すテキストまたはスクリーンショット

## 9. ストアページ用素材 (Fab 掲載)

- [ ] スクリーンショット (最低5枚、推奨10枚以上)
  - [ ] 各エフェクトのスクリーンショット or GIF
  - [ ] Blueprint ノードのスクリーンショット
  - [ ] Preview Tool のスクリーンショット
- [ ] 動画 (推奨) — 全エフェクトのデモ動画
- [ ] 商品説明文 (英語)
- [ ] テクニカル仕様リスト (機能一覧、対応エンジンバージョン、モジュール数等)

## 10. リリース前検証

- [ ] **プラグインパッケージングテスト** — Plugin を単独パッケージし、出力物に必要なファイルが含まれることを確認
- [ ] **クリーンプロジェクトインストールテスト** — 新規 UE5 プロジェクトにプラグインを追加し、正常にコンパイル・動作することを確認
- [ ] **コンパイル警告ゼロ確認** — Development / Shipping ビルドで Warning が出ないことを確認
- [ ] **LICENSE ファイルの同梱確認** — `Plugins/TransitionFX/` 内に LICENSE ファイルが含まれること
- [ ] **Fab 提出メタデータの準備** — カテゴリ選択、価格設定、技術仕様フォーム（対応エンジンバージョン、モジュール数等）

---

## 現状の棚卸

### 既にあるもの
- 23/23 プリセット Data Asset（22エフェクト + FadeToBlack）
- 22 マスターマテリアル + 22 マテリアルインスタンス
- 8 マテリアル関数
- 2 サンプルマップ (L_ShowCase, LevelA)
- 1 ShowCase Widget (WBP_ShowCase)
- Editor Preview Tool
- 充実したドキュメント (README EN/JP, API Reference EN/JP, Quick Start EN/JP, FAQ EN/JP, etc.)
- 49/51 ドキュメント画像
- FilterPlugin.ini（Fab パッケージング設定済み）

### 作成が必要なもの
- Overview/Showcase Map の実装（L_ShowCase は存在するが要整備）
- ユースケース別サンプル Blueprint 群
- 操作UI Widget (WBP_TransitionShowcase) の実装
- サウンドエフェクト素材
- 不足している2枚のドキュメント画像 (hero_banner.gif, quickstart_hold_workflow_bp.png)
- ストアページ用素材

---

## 優先度

| 優先度 | 項目 |
|--------|------|
| **P0 (必須)** | Overview Map, 全23プリセット完備, 命名規則準拠, 未使用ファイル排除, リリース前検証 |
| **P1 (強く推奨)** | レベル遷移デモ, ローディングスクリーンデモ, Blueprint サンプル群, ストアページ素材 |
| **P2 (推奨)** | ポーズメニューデモ, オーディオ連携, イベント連携デモ, パラメータオーバーライドデモ |
| **P3 (あると良い)** | ランダム選択デモ, プリロードデモ, 動画素材 |
