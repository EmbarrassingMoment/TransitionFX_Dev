# TransitionFX

<!-- IMAGE: hero_banner.gif - プラグインのヒーロー画像（複数エフェクトのモンタージュ/GIF） -->

## Description
TransitionFXは、Unreal Engine 5向けの軽量かつ高度なプロシージャル画面遷移システムです。
テクスチャを使用せず、SDF（Signed Distance Field）計算に基づいた高品質なトランジションを描画し、ブループリントからたった1つのノードで実装可能です。

## 設計思想
TransitionFXは、**インディーや小規模チームの開発者が、制作中のゲームに後から遷移エフェクトを簡単に追加できる**ことを最優先に設計されています。専任のテックアーティストがいなくても、追加のコードをほとんど書くことなく、ゲームにクオリティの高いトランジションを組み込むことを可能にすることを目標にしました。

---

### データ駆動設計

エフェクトの種類・持続時間・イージング・音声など、遷移に関するすべての設定を **Transition Preset（Data Asset）** に集約しています。ブループリントからは `Play Transition And Wait` ノードにプリセットを渡すだけで動作します。

プログラマーが呼び出し側のコードを変えることなく、プリセットを差し替えるだけでエフェクトを変更できるため、チーム内の役割分担がしやすく、後からの調整も容易です。

---

### Latent Action

TransitionFXでは一部のブループリントに **Latent Action** を採用しているため、コールバック関数やフラグ管理を自分で書く必要がありません。完了ピンが発火してから次の処理を繋ぐだけで処理の逐次実行が可能です。

`bHoldAtMax` と `ReleaseHold` を組み合わせると、「画面が完全に覆われた状態で一時停止 → バックグラウンドでレベルロード → 完了後に再開」というロード画面パターンも実現できます。

---

### GameInstance Subsystem

マネージャーは **GameInstance Subsystem** として動作するため、レベルをまたいでも状態が維持されます。フェードアウト→レベル遷移→フェードインという一連のシーケンスはプラグインが自動で管理します。入力ブロックやエフェクトのプール管理も自動で行われるため、既存のゲームに組み込んでも他のコードと干渉しにくい設計になっています。

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
*   **Audio Integration:** 効果音（SFX）をトランジションと同期させます。システムがオーディオのライフサイクルを管理し、開始時に再生し、トランジションがキャンセルされた場合は自動的に停止します。
*   **Event System:** `OnTransitionStarted`、`OnTransitionCompleted`、`OnTransitionHoldStarted`デリゲートを使用して、正確なゲームプレイロジックのタイミングを取得できます。
*   **Blueprint Support:** クリーンで簡単なスクリプティングのためのLatent Actionノード（`PlayTransitionAndWait`）が含まれています。

## Installation
1.  リリースページからプラグインをダウンロードします。
2.  `TransitionFX`フォルダをプロジェクトの`Plugins`ディレクトリに配置します。
3.  エディタのプラグインウィンドウで`TransitionFX`を有効にします。

<!-- IMAGE: install_enable_plugin.png - Plugins ウィンドウで TransitionFX を有効にするスクリーンショット -->

## Quick Start

### 1. Create a Preset
コンテンツブラウザで右クリック > `Miscellaneous` (その他) > `Data Asset`。
`TransitionPreset`クラスを選択し、名前を付けます（例：`DA_FadeBlack`）。

<!-- IMAGE: quickstart_create_data_asset.png - Content Browser で Data Asset を作成する手順のスクリーンショット -->

*   **Effect Class:** `PostProcessTransitionEffect`を選択します。
*   **Transition Material:** `M_Transition_Master`（または`M_Transition_Iris`、`M_Transition_Diamond`など）を選択します。
*   **Default Duration:** 秒単位で時間を設定します（例：`1.0`）。
*   **Progress Curve:** (任意) トランジションのイージングを制御するためのフロートカーブを設定します。
*   **bAutoBlockInput:** トランジション中のプレイヤー入力を自動的に無効にするには `True` に設定します。
*   **bTickWhenPaused:** ゲームが一時停止中でもトランジションを再生するには `True` に設定します。
*   **Priority:** レンダリングの優先順位を設定します（デフォルト：1000）。
*   **Audio:** (任意) 再生するサウンドアセットを割り当てます。ボリュームとピッチの制御が含まれます。

<!-- IMAGE: quickstart_preset_settings.png - TransitionPreset の設定パネル（プロパティ一覧）のスクリーンショット -->

### 2. Call from Blueprint
レベルブループリントまたはGameInstanceで`Play Transition And Wait`ノードを使用します。

<!-- IMAGE: quickstart_bp_play_node.png - Play Transition And Wait ノードの Blueprint スクリーンショット -->

*   **Fade Out (Forward):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Forward`, Speed: `1.0`, Invert: `False`)
    *（完了後、画面は黒のままになります）*

*   **Fade In (Reverse):**
    `Play Transition And Wait` (Preset: `DA_FadeBlack`, Mode: `Reverse`, Speed: `1.0`, Invert: `False`)
    *（完了時にエフェクトは自動的に削除されます）*

*   **Random Play (ランダム再生):**
    `Play Random Transition And Wait` ノードを使用すると、プリセットの配列からランダムにトランジションを再生できます。

### 3. Events
`TransitionManagerSubsystem`内の以下のイベントにバインドできます：
*   **OnTransitionStarted:** トランジション開始時に発火します。
*   **OnTransitionCompleted:** トランジション終了時に発火します。
*   **OnTransitionHoldStarted:** トランジションが最大進行度（1.0）でホールド（一時停止）されたときに発火します（`bHoldAtMax` が true の場合）。

## API Reference
`TransitionManagerSubsystem`は、高度な制御のためにいくつかの呼び出し可能な関数を提供します：

*   **StopTransition():** 現在のトランジションを即座に停止します。
*   **ReverseTransition(bool bAutoStop):** 再生方向を反転します（例：フェードアウトからフェードインへ）。
*   **SetPlaySpeed(float NewSpeed):** 再生速度の乗数を動的に変更します。
*   **GetCurrentProgress():** 現在の進捗状況（0.0〜1.0）を返します。
*   **IsTransitionPlaying():** トランジションが現在アクティブな場合にTrueを返します。
*   **IsCurrentTransitionFinished():** トランジションが終了状態に達している場合にTrueを返します（ポーリングに便利です）。

## Built-in Effects

| Effect Name | Description | Preview |
| :--- | :--- | :--- |
| **Fade** | 標準的な不透明度のフェード。シンプルで軽量です。 | <!-- IMAGE: effect_fade.gif --> |
| **Iris** | 中央に向かって閉じるクラシックな円形ワイプ。アスペクト比補正済み。 | <!-- IMAGE: effect_iris.gif --> |
| **Heart Iris** | プロシージャルSDFを使用したハート型のアイリスワイプ。 | <!-- IMAGE: effect_heart_iris.gif --> |
| **Flower Iris** | 丸い花びらを持つ花形のアイリスワイプ。花びらの数や形状（鋭さ）を調整可能。 | <!-- IMAGE: effect_flower_iris.gif --> |
| **Diamond** | 中央に向かって閉じるダイヤモンド型のワイプ。レトロなスタイル。 | <!-- IMAGE: effect_diamond.gif --> |
| **Box** | 中央から拡大するシンプルな正方形。基本的な幾何学的トランジション。 | <!-- IMAGE: effect_box.gif --> |
| **Linear Wipe** | 方向性のあるワイプ（角度調整可能）。画面の端から端まで正確に覆います。 | <!-- IMAGE: effect_linear_wipe.gif --> |
| **Split** | 画面中央から真っ二つに割れて開くスタイリッシュなワイプ。分割する角度（水平、垂直、斜めなど）を自由に調整可能です。 | <!-- IMAGE: effect_split.gif --> |
| **Wavy Curtain** | Linear Wipeに似ていますが、カーテンのような波打つ境界線を持つ方向性ワイプです。 | <!-- IMAGE: effect_wavy_curtain.gif --> |
| **Radial Wipe** | 時計のような放射状ワイプ。滑らかなエッジと開始角度の調整をサポート。 | <!-- IMAGE: effect_radial_wipe.gif --> |
| **Tiles** | 画面がグリッド状に分割され、ブロックが中央から波のように拡大します。 | <!-- IMAGE: effect_tiles.gif --> |
| **Polka Dots** | 拡大する円（ハーフトーンパターン）の波が画面を覆います。ポップでモダンな外観。 | <!-- IMAGE: effect_polka_dots.gif --> |
| **Blinds** | スタイリッシュなストライプ/ベネチアンブラインド効果。ストライプが拡大・結合して画面を覆います。 | <!-- IMAGE: effect_blinds.gif --> |
| **Spiral** | 中央に渦巻く催眠的なスパイラル効果。回転スピンと開始角度を調整可能。 | <!-- IMAGE: effect_spiral.gif --> |
| **Random Tiles** | プロシージャルノイズを使用して、グリッドタイルがランダムな順序で現れる確率的なトランジション。 | <!-- IMAGE: effect_random_tiles.gif --> |
| **Wind** | ストリークノイズを伴う方向性ワイプで、風が画像を吹き飛ばすような表現。 | <!-- IMAGE: effect_wind.gif --> |
| **Cross Wipe** | 十字の形が中央から拡大し、画像を四隅に押しやって消滅させます。 | <!-- IMAGE: effect_cross_wipe.gif --> |
| **Zoom Wipe** | フェードアウトしながらシーンを内側に歪めてズームする方向性ワイプ。 | <!-- IMAGE: effect_zoom_wipe.gif --> |
| **Texture Mask** | グレースケールテクスチャを使用してトランジションの順序を決定します（黒=開始、白=終了）。パラメータオーバーライドによるカスタムマスクテクスチャをサポート。 | <!-- IMAGE: effect_texture_mask.gif --> |
| **TV Switch Off** | ブラウン管テレビの電源を切った時のように、画面が上下に潰れて横線になり、最後に中央の点に向かって消滅するレトロなエフェクトです。 | <!-- IMAGE: effect_tv_switch_off.gif --> |
| **Hexagon** | SFテイストのハニカム（六角形）ワイプです。画面中央から波紋のように、各セルが滑らかに縮小して消滅します。 | <!-- IMAGE: effect_hexagon.gif --> |
| **Triangle** | 鋭利な三角形（ポリゴン）が画面中央から波紋のように縮小して消えていくスタイリッシュなワイプ。 | ![Triangle](https://via.placeholder.com/320x180/000000/FFFFFF?text=Triangle) |
| **Checkerboard** | 画面をチェッカーボード（市松模様）パターンで分割し、各タイルが拡大して画面を覆います。クラシックなレトロ風。 | <!-- IMAGE: effect_checkerboard.gif --> |
| **Pixelate** | 画面の解像度を徐々に下げていくピクセル化エフェクト。フェードアウトまでモザイクが進行します。 | <!-- IMAGE: effect_pixelate.gif --> |

> **Texture Mask（テクスチャマスク）のヒント:**
> マスクテクスチャをインポートする際は、正確な値を読み取るために **sRGB** のチェックを外し（sRGBオフ）、Compression Settings（圧縮設定）を **Masks (no sRGB)** または **Grayscale** に設定してください。

## Transition Timing & Easing (イージングとタイミング)
Transition Presetの`EasingType`プロパティを使用して、トランジションが時間とともにどのように進行するかを制御します。

| Easing Type | Description |
| :--- | :--- |
| **Linear** | 一定速度（デフォルト）。単純なフェードに適しています。 |
| **Sine / Cubic / Expo** | 滑らかな加速と減速。（In、Out、InOutのバリエーションが利用可能）。 |
| **Bounce / Elastic** | トランジションの終わりにバウンドや弾性効果を追加します。 |
| **Custom** | 独自の `FloatCurve` アセットを指定できます。 |

*注: `Transition Curve` スロットは、`Custom` が選択された場合にのみ表示されます。*

<!-- IMAGE: easing_curves.png - イージングタイプ比較チャート（Linear, Sine, Cubic, Expo, Bounce, Elastic） -->

これらのカーブの視覚化については、[easings.net](https://easings.net/) を参照してください。

## Performance Tips (パフォーマンス最適化)

### シェーダーのプリロード（ヒッチング回避）
トランジションが初めて再生される際のフレームドロップ（ヒッチング）を防ぐために、Preload APIを使用してシェーダーを事前コンパイルする機能を用意しました。

**問題:** Unreal Engineはシェーダーをオンデマンドでコンパイルするため、トランジションエフェクトが初めて再生される際にわずかなスタッター（ヒッチング）が発生することがあります。
**解決策:** `PreloadTransitionPresets` 関数は、一時的な動的マテリアルインスタンスを作成し、ゲームプレイが開始する *前* にエンジンにシェーダーを準備させることができます。

**使用方法:**
**GameInstance Init** や **Level BeginPlay** などの安全な場所で `PreloadTransitionPresets` を呼び出します。
この関数に、最も頻繁に使用するトランジションプリセットの配列を渡します。

<!-- IMAGE: performance_preload_bp.png - GameInstance Init での PreloadTransitionPresets ノードの Blueprint スクリーンショット -->

```cpp
// C++ Example
TArray<UTransitionPreset*> MyPresets = { FadePreset, WipePreset };
TransitionSubsystem->PreloadTransitionPresets(MyPresets);
```
*これにより、GPUの準備を整えるために1フレーム分のダミーマテリアルが作成されます。*

**API リファレンス:**
*   **関数:** `TransitionManagerSubsystem->PreloadTransitionPresets(TArray<UTransitionPreset*> Presets)`

### 非同期ロード（ソフト参照）
メモリを節約するためにトランジションアセットを非同期（例：ロード画面中）でロードしたい場合は、Async APIを使用します。
バックグラウンドでアセットをロードし、自動的にシェーダーのウォームアップを実行し、最後にコールバックイベントを発火させます。

**使用方法:**
1. **ソフトオブジェクト参照 (Soft Object References)** の配列を `AsyncLoadTransitionPresets` に渡します。
2. システムはバックグラウンドでそれらをロードし、シェーダーをウォームアップします。
3. 準備が整うと `OnComplete` イベントが発火します。

**ブループリントでの使用:**
ソフトオブジェクト参照の配列を渡します。ロジック（例：レベルを開く）を「On Complete」デリゲートピンに接続します。

```cpp
// C++ Example
TArray<TSoftObjectPtr<UTransitionPreset>> SoftPresets = { ... };

TransitionSubsystem->AsyncLoadTransitionPresets(SoftPresets, FTransitionPreloadCompleteDelegate::CreateLambda([]()
{
    UE_LOG(LogTransitionFX, Log, TEXT("Assets loaded and shaders ready!"));
}));
```

**API リファレンス:**
*   **関数:** `AsyncLoadTransitionPresets(TArray<TSoftObjectPtr<UTransitionPreset>> Presets, FTransitionPreloadCompleteDelegate OnComplete)`

## License
MIT License
