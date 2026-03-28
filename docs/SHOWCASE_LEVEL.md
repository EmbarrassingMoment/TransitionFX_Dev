# ShowCase Level

TransitionFX プラグインに含まれる全トランジションエフェクトをインタラクティブにプレビューできるデモレベル。

## 概要

- **レベル**: `Content/ShowCase/L_ShowCase.umap`
- **ウィジェット**: `Content/ShowCase/WBP_ShowCase.uasset`

ShowCase レベルを PIE (Play In Editor) で実行すると、画面下部に操作パネルが表示される。
ボタン操作で全 25 種のトランジションプリセットを切り替え・再生できる。

## 操作方法

| ボタン | 機能 |
|--------|------|
| **Prev / Next** | 表示中のプリセットを前後に切り替え |
| **Play** | 選択中のプリセットを Forward → Hold → Reverse で再生 |
| **Replay** | 同じプリセットを再度再生 |
| **Play All** | 全プリセットを順番に自動再生 |

## アーキテクチャ

### Level Blueprint (L_ShowCase)

1. `BeginPlay` で `AsyncLoadTransitionPresets` を呼び出し、全プリセットを非同期ロード＋シェーダーウォームアップ
2. ロード完了コールバックで Soft Reference → Hard Reference に解決
3. `WBP_ShowCase` ウィジェットを生成し Viewport に追加
4. マウスカーソル表示、入力モードを UI Only に設定

### Widget Blueprint (WBP_ShowCase)

**デリゲートバインド方式**: `Event Construct` で `TransitionManagerSubsystem` のデリゲートを 1 回だけ Bind する。
Bind/Unbind の繰り返しを避け、`bIsPlayAll` 状態変数で単発再生/ループ再生を分岐する。

**再生フロー**:
```
StartTransition(Forward, bHoldAtMax=true)
  → Progress 0.0 → 1.0 (画面が覆われる)
  → OnTransitionHoldStarted → Delay 0.3s
  → ReverseTransition(bAutoStop=true)
  → Progress 1.0 → 0.0 (画面が現れる)
  → OnTransitionCompleted
  → bIsPlayAll ? 次のプリセットへ : 完了
```

**主要変数**:

| 変数 | 型 | 用途 |
|------|-----|------|
| `TransitionPresets` | `TArray<UTransitionPreset*>` | 全プリセット配列 |
| `CurrentIndex` | `Integer` | 選択中のインデックス |
| `bIsPlaying` | `Boolean` | 再生中フラグ |
| `bIsPlayAll` | `Boolean` | Play All モードフラグ |
| `TransitionManager` | `UTransitionManagerSubsystem*` | サブシステム参照キャッシュ |

## 含まれるプリセット (23 種)

Fade, Iris, Diamond, LinearWipe, RadialWipe, CrossWipe, Box, Triangle,
Hexagon, Spiral, Tiles, RandomTiles, Blinds, PolkaDots, CheckerBoard,
FlowerIris, TVSwitchOff, WavyCurtain, Wind, Pixelate, Split, TextureMask, FadeToBlack

## 注意事項

- C++ コードの変更は不要。すべて Blueprint / UMG で構成
- 全アセットは Unreal Editor 上で手動作成する必要がある
