# TransitionFX

> **Note:** 🚧 **現在開発中です** 🚧
> This project is currently under active development.

## Description
TransitionFXは、Unreal Engine 5向けの軽量かつ高度なプロシージャル画面遷移システムです。
テクスチャを使用せず、SDF（Signed Distance Field）計算に基づいた高品質なトランジションを描画し、ブループリントからたった1つのノードで実装可能です。

## Features
*   **UE 5.5+ Native:** 最新のUnreal Engine機能に最適化されています。
*   **Procedural Rendering:** テクスチャレスなSDFベースのレンダリングにより、あらゆる解像度で劣化せず、アスペクト比の歪みを自動的に補正します。
*   **Design-First Workflow:**
    *   **Data Asset Driven:** トランジションパターン、持続時間、カーブを再利用可能な「プリセット」として管理します。
    *   **Auto Input Blocking:** トランジション中のプレイヤー入力ブロックを自動的に処理します。
    *   **Pause Support:** ゲームが一時停止中でもスムーズに動作します。
*   **Versatile Control:**
    *   **Forward / Reverse:** トランジションモードを使用して、単一のプリセットで「フェードアウト」と「フェードイン」を制御します。
    *   **Speed Control:** `SetPlaySpeed`による動的な再生速度調整が可能です。
*   **Event System:** `OnTransitionStarted`と`OnTransitionCompleted`デリゲートを使用して、正確なゲームプレイロジックのタイミングを取得できます。
*   **Blueprint Support:** クリーンで簡単なスクリプティングのためのLatent Actionノード（`PlayTransitionAndWait`）が含まれています。

## Installation
1.  リリースページからプラグインをダウンロードします。
2.  `TransitionFX`フォルダをプロジェクトの`Plugins`ディレクトリに配置します。
3.  エディタのプラグインウィンドウで`TransitionFX`を有効にします。

## Quick Start

### 1. Create a Preset
コンテンツブラウザで右クリック > `Miscellaneous` (その他) > `Data Asset`。
`TransitionPreset`クラスを選択し、名前を付けます（例：`DA_FadeBlack`）。
*   **Effect Class:** `PostProcessTransitionEffect`を選択します。
*   **Transition Material:** `M_Transition_Master`（または`Iris`、`Diamond`）を選択します。
*   **Default Duration:** 秒単位で時間を設定します（例：`1.0`）。
*   **Progress Curve:** (任意) トランジションのイージングを制御するためのフロートカーブを設定します。
*   **bAutoBlockInput:** トランジション中のプレイヤー入力を自動的に無効にするには `True` に設定します。
*   **bTickWhenPaused:** ゲームが一時停止中でもトランジションを再生するには `True` に設定します。
*   **Priority:** レンダリングの優先順位を設定します（デフォルト：1000）。

### 2. Call from Blueprint
レベルブループリントまたはGameInstanceで`Play Transition And Wait`ノードを使用します。

*   **Fade Out (Forward):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Forward`, Speed: `1.0`)
    *（完了後、画面は黒のままになります）*

*   **Fade In (Reverse):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Reverse`, Speed: `1.0`)
    *（完了時にエフェクトは自動的に削除されます）*

### 3. Events
`TransitionManagerSubsystem`内の以下のイベントにバインドできます：
*   **OnTransitionStarted:** トランジション開始時に発火します。
*   **OnTransitionCompleted:** トランジション終了時に発火します。

## API Reference
`TransitionManagerSubsystem`は、高度な制御のためにいくつかの呼び出し可能な関数を提供します：

*   **StopTransition():** 現在のトランジションを即座に停止します。
*   **ReverseTransition(bool bAutoStop):** 再生方向を反転します（例：フェードアウトからフェードインへ）。
*   **SetPlaySpeed(float NewSpeed):** 再生速度の乗数を動的に変更します。
*   **GetCurrentProgress():** 現在の進捗状況（0.0〜1.0）を返します。
*   **IsTransitionPlaying():** トランジションが現在アクティブな場合にTrueを返します。
*   **IsCurrentTransitionFinished():** トランジションが終了状態に達している場合にTrueを返します（ポーリングに便利です）。

## Built-in Effects
*   **Fade:** 単純な不透明度フェード。
*   **Iris:** 中央に向かって閉じる円形ワイプ（アスペクト比補正済み）。
*   **Diamond:** 中央に向かって閉じるダイヤモンド型ワイプ（アスペクト比補正済み）。
*   *(More coming soon...)*

## License
MIT License
