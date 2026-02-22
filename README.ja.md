# TransitionFX

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
*   **🔊 Audio Integration:** 効果音（SFX）をトランジションと同期させます。システムがオーディオのライフサイクルを管理し、開始時に再生し、トランジションがキャンセルされた場合は自動的に停止します。
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
*   **Audio:** (任意) 再生するサウンドアセットを割り当てます。ボリュームとピッチの制御が含まれます。

### 2. Call from Blueprint
レベルブループリントまたはGameInstanceで`Play Transition And Wait`ノードを使用します。

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
| **Fade** | 標準的な不透明度のフェード。シンプルで軽量です。 | ![Fade](https://via.placeholder.com/320x180/000000/FFFFFF?text=Fade) |
| **Iris** | 中央に向かって閉じるクラシックな円形ワイプ。アスペクト比補正済み。 | ![Iris](https://via.placeholder.com/320x180/000000/FFFFFF?text=Iris) |
| **Heart Iris** | プロシージャルSDFを使用したハート型のアイリスワイプ。 | ![HeartIris](https://via.placeholder.com/320x180/000000/FFFFFF?text=Heart+Iris) |
| **Flower Iris** | 丸い花びらを持つ花形のアイリスワイプ。花びらの数や形状（鋭さ）を調整可能。 | ![FlowerIris](https://via.placeholder.com/320x180/000000/FFFFFF?text=Flower+Iris) |
| **Diamond** | 中央に向かって閉じるダイヤモンド型のワイプ。レトロなスタイル。 | ![Diamond](https://via.placeholder.com/320x180/000000/FFFFFF?text=Diamond) |
| **Box** | 中央から拡大するシンプルな正方形。基本的な幾何学的トランジション。 | ![Box](https://via.placeholder.com/320x180/000000/FFFFFF?text=Box) |
| **Linear Wipe** | 方向性のあるワイプ（角度調整可能）。画面の端から端まで正確に覆います。 | ![Linear](https://via.placeholder.com/320x180/000000/FFFFFF?text=Linear+Wipe) |
| **Wavy Curtain** | Linear Wipeに似ていますが、カーテンのような波打つ境界線を持つ方向性ワイプです。 | ![WavyCurtain](https://via.placeholder.com/320x180/000000/FFFFFF?text=Wavy+Curtain) |
| **Radial Wipe** | 時計のような放射状ワイプ。滑らかなエッジと開始角度の調整をサポート。 | ![Radial](https://via.placeholder.com/320x180/000000/FFFFFF?text=Radial+Wipe) |
| **Box Grid** | 画面がグリッド状に分割され、ブロックが中央から波のように拡大します。 | ![BoxGrid](https://via.placeholder.com/320x180/000000/FFFFFF?text=Box+Grid) |
| **Polka Dots** | 拡大する円（ハーフトーンパターン）の波が画面を覆います。ポップでモダンな外観。 | ![PolkaDots](https://via.placeholder.com/320x180/000000/FFFFFF?text=Polka+Dots) |
| **Blinds** | スタイリッシュなストライプ/ベネチアンブラインド効果。ストライプが拡大・結合して画面を覆います。 | ![Blinds](https://via.placeholder.com/320x180/000000/FFFFFF?text=Blinds) |
| **Spiral** | 中央に渦巻く催眠的なスパイラル効果。回転スピンと開始角度を調整可能。 | ![Spiral](https://via.placeholder.com/320x180/000000/FFFFFF?text=Spiral) |
| **Random Tiles** | プロシージャルノイズを使用して、グリッドタイルがランダムな順序で現れる確率的なトランジション。 | ![RandomTiles](https://via.placeholder.com/320x180/000000/FFFFFF?text=Random+Tiles) |
| **Wind** | ストリークノイズを伴う方向性ワイプで、風が画像を吹き飛ばすような表現。 | ![Wind](https://via.placeholder.com/320x180/000000/FFFFFF?text=Wind) |
| **Cross Wipe** | 十字の形が中央から拡大し、画像を四隅に押しやって消滅させます。 | ![CrossWipe](https://via.placeholder.com/320x180/000000/FFFFFF?text=Cross+Wipe) |
| **Zoom Wipe** | フェードアウトしながらシーンを内側に歪めてズームする方向性ワイプ。 | ![ZoomWipe](https://via.placeholder.com/320x180/000000/FFFFFF?text=Zoom+Wipe) |
| **Texture Mask** | グレースケールテクスチャを使用してトランジションの順序を決定します（黒=開始、白=終了）。パラメータオーバーライドによるカスタムマスクテクスチャをサポート。 | ![TextureMask](https://via.placeholder.com/320x180/000000/FFFFFF?text=Texture+Mask) |
| **TV Switch Off** | ブラウン管テレビの電源を切った時のように、画面が上下に潰れて横線になり、最後に中央の点に向かって消滅するレトロなエフェクトです。 | ![TVSwitchOff](https://via.placeholder.com/320x180/000000/FFFFFF?text=TV+Switch+Off) |
| **Hexagon** | SFテイストのハニカム（六角形）ワイプです。画面中央から波紋のように、各セルが滑らかに縮小して消滅します。 | ![Hexagon](https://via.placeholder.com/320x180/000000/FFFFFF?text=Hexagon) |

> **💡 Texture Mask（テクスチャマスク）のヒント:**
> マスクテクスチャをインポートする際は、正確な値を読み取るために **sRGB** のチェックを外し（sRGBオフ）、Compression Settings（圧縮設定）を **Masks (no sRGB)** または **Grayscale** に設定してください。

## ⏳ Transition Timing & Easing (イージングとタイミング)
Transition Presetの`EasingType`プロパティを使用して、トランジションが時間とともにどのように進行するかを制御します。

| Easing Type | Description |
| :--- | :--- |
| **Linear** | 一定速度（デフォルト）。単純なフェードに適しています。 |
| **Sine / Cubic / Expo** | 滑らかな加速と減速。（In、Out、InOutのバリエーションが利用可能）。 |
| **Bounce / Elastic** | トランジションの終わりにバウンドや弾性効果を追加します。 |
| **Custom** | 独自の `FloatCurve` アセットを指定できます。 |

*注: `Transition Curve` スロットは、`Custom` が選択された場合にのみ表示されます。*

これらのカーブの視覚化については、[easings.net](https://easings.net/) を参照してください。

## 🚀 Performance Tips (パフォーマンス最適化)

### シェーダーのプリロード（ヒッチング回避）
トランジションが初めて再生される際のフレームドロップ（ヒッチング）を防ぐために、Preload APIを使用してシェーダーを事前コンパイルできます。

**問題:** Unreal Engineはシェーダーをオンデマンドでコンパイルするため、トランジションエフェクトが初めて再生される際にわずかなスタッター（ヒッチング）が発生することがあります。
**解決策:** `PreloadTransitionPresets` 関数は、一時的な動的マテリアルインスタンスを作成し、ゲームプレイが開始する *前* にエンジンにシェーダーを準備させます。

**使用方法:**
**GameInstance Init** や **Level BeginPlay** などの安全な場所で `PreloadTransitionPresets` を呼び出します。
この関数に、最も頻繁に使用するトランジションプリセットの配列を渡します。

```cpp
// C++ Example
TArray<UTransitionPreset*> MyPresets = { FadePreset, WipePreset };
TransitionSubsystem->PreloadTransitionPresets(MyPresets);
```
*これにより、GPUの準備を整えるために1フレーム分のダミーマテリアルが作成されます。*

**API リファレンス:**
*   **関数:** `TransitionManagerSubsystem->PreloadTransitionPresets(TArray<UTransitionPreset*> Presets)`

### ⏳ 非同期ロード（ソフト参照）
メモリを節約するためにトランジションアセットをオンデマンド（例：ロード画面中）でロードしたい場合は、Async APIを使用します。
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
