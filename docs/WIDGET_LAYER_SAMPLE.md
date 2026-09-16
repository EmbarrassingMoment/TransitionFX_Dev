# Widget Layer Sample Level

ウィジェットレイヤー版トランジション（`WidgetTransitionEffect` / `DA_Widget_*`）が **UMG ごと画面を覆う** ことを、
PostProcess 版と並べて確認するための検証用レベル。

## 概要

- **レベル**: `Content/SampleLevel/L_WidgetLayerSample.umap`
- **ウィジェット**: `Content/Widget/WBP_WidgetLayerSample.uasset`
- **ロジック (C++)**: `Source/TransitionFX_Dev/WidgetLayerSampleWidget.h/.cpp`, `WidgetLayerSampleActor.h/.cpp`
- **生成スクリプト**: `Plugins/DevMaterialTools/Tools/build_widget_layer_sample.py`

PIE で実行すると、画面の **右半分が不透明な UMG パネル**、左半分が 3D シーン（キューブ + TextRender）になり、
画面下部に操作バーが表示される。

## 見え方の違い

| 操作 | 期待される挙動 |
|------|----------------|
| **Play PostProcess** | 左半分（3D ビュー）だけがトランジションで覆われ、右の UMG パネルと操作バーはそのまま見える |
| **Play Widget Layer** | UMG パネル・操作バー・3D ビューを含む **画面全体** が覆われる |

どちらも `StartTransition(Forward, bHoldAtMax=true)`（FadeOut） → `OnTransitionHoldStarted` → `HoldDuration`(0.3s) 待機 →
`InvertTransition(bAutoComplete=true)`（マスクを反転して Forward 0→1 で FadeIn） → `OnTransitionCompleted` の流れで再生する。
`ReverseTransition` で Progress を 1→0 に巻き戻すのではなく、`OpenLevelWithTransition` と同じ
「Forward のまま Invert で FadeIn」パターンなので、`M_Widget_*` の `Invert` 経路もあわせて検証できる。

## 操作方法

| ボタン | 機能 |
|--------|------|
| **Prev / Next** | `DA_Widget_*` プリセット（アルファベット順 9 種）を切り替え |
| **Play Widget Layer** | 選択中の `DA_Widget_*` を再生 |
| **Play PostProcess** | 同名の PostProcess 版 `DA_*` を再生（未登録なら Status に表示） |

上段のラベルに `[n/9] DA_Widget_X | PostProcess: DA_X`、その下の Status に再生状態が出る。

## アーキテクチャ

ShowCase レベルと違い、Level Blueprint は使わず、ヘッドレス生成しやすいようロジックを C++ に置いている。

### `AWidgetLayerSampleActor`（レベルに配置）

`BeginPlay` で `WidgetClass`（= `WBP_WidgetLayerSample`）を生成して `AddToViewport(WidgetZOrder=0)`、
マウスカーソル表示、`FInputModeGameAndUI` に切り替える。

### `UWidgetLayerSampleWidget`（`WBP_WidgetLayerSample` の親クラス）

| プロパティ | 型 | 用途 |
|-----------|-----|------|
| `WidgetPresets` | `TArray<TSoftObjectPtr<UTransitionPreset>>` | Prev / Next で巡回する `DA_Widget_*` |
| `PostProcessPresets` | `TArray<TSoftObjectPtr<UTransitionPreset>>` | 同じインデックスの PostProcess 版（null 可） |
| `HoldDuration` | `float` | 覆い切った状態で待つ秒数 (0.3) |

`BindWidgetOptional` で `PrevButton` / `NextButton` / `PlayWidgetButton` / `PlayPostProcessButton` /
`PresetNameText` / `StatusText` を拾う。同名ウィジェットが無くても動く（機能が減るだけ）ので、
デザイナ上でレイアウトを自由に組み替えられる。`NativeConstruct` でプリセットを同期ロードし
`PreloadTransitionPresets` でシェーダーをウォームアップする。

### WBP のレイアウト（Canvas ルート）

```
RootCanvas (CanvasPanel)
├─ UmgPanel (Border, 右半分・不透明, 下 170px は操作バー用に空ける)
│   └─ UmgPanelBox (VerticalBox) — 見出しと説明文
├─ SceneLabel (TextBlock, 左上 "3D SCENE (below UMG)")
└─ ControlBar (Border, 下部 170px・半透明黒)
    └─ ControlBarBox (VerticalBox)
        ├─ PresetNameText
        ├─ StatusText
        └─ ButtonRow (HorizontalBox) — PrevButton / NextButton / PlayWidgetButton / PlayPostProcessButton
```

## 再生成

エディタを閉じた状態で:

```bat
D:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
  -script="<abs>\Plugins\DevMaterialTools\Tools\build_widget_layer_sample.py" -EnablePlugins=PythonScriptPlugin
```

- WBP とレベルを **削除して作り直す**（デザイナで手を入れた内容は消える）
- `WidgetPresets` は `/TransitionFX/Data` の `DA_Widget_*` 全部、`PostProcessPresets` は同名の `DA_*` を自動で割り当てる。
  ウィジェットレイヤー版を追加したら再実行すれば一覧に載る
- `UWidgetTree` は Python から触れないため、ウィジェット生成とルート設定は
  `DevBlueprintTools.construct_widget_in_tree` / `set_widget_tree_root`（DevMaterialTools プラグインの C++）経由
- 結果は `Saved/DevMaterialTools/build_widget_layer_sample.result.json`（`result: PASS` を確認する。コマンドレットの exit code は常に 1）

## 注意事項

- `UWidgetLayerSampleWidget` / `AWidgetLayerSampleActor` はサンプルプロジェクト（`TransitionFX_Dev` モジュール）側のクラスで、
  `TransitionFX` プラグイン本体には含まれない
- Widget ZOrder はサンプル UI が 0、トランジションが `UTransitionPreset::WidgetZOrder`（既定 10000）。
  UI 側の ZOrder を 10000 以上にすると覆われなくなる（それ自体の確認にも使える）
