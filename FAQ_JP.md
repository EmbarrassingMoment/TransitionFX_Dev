# TransitionFX FAQ（よくある質問）

TransitionFX プラグインに関するよくある質問をまとめています。

---

## 目次

- [トラブルシューティング](#トラブルシューティング)
- [仕様・挙動について](#仕様挙動について)
- [応用・ベストプラクティス](#応用ベストプラクティス)

---

## トラブルシューティング

### Q: トランジションがまったく表示されません。原因は何ですか？

以下を順番に確認してください。

1. **プリセットが設定されていない:** ノードの `Preset` ピンが接続されているか確認してください。`nullptr` が渡された場合、エラーがログに出力され、内部で `ForceClear` が呼ばれます。
2. **マテリアルが未設定:** `TransitionPreset` データアセットを開き、`Transition Material` が設定されていることを確認してください。マテリアルが設定されていない場合、トランジションは描画されません。
3. **マテリアルに `Progress` パラメータがない:** すべてのトランジションマテリアルには `Progress` というスカラーパラメータが必要です。存在しない場合、エフェクトはアニメーションしません（警告がログに出力されます）。
4. **Effect Class が未設定:** `Effect Class` が `PostProcessTransitionEffect`（またはカスタムエフェクトクラス）に設定されていることを確認してください。
5. **PostProcess 設定の競合:** プロジェクトで既に極端な設定の Post Process Volume を使用している場合、視覚的に干渉する可能性があります。プリセットの `Priority` 値（デフォルト: 1000）を確認してください。

### Q: ゲームをポーズするとトランジションが止まります。

`TransitionPreset` の `bTickWhenPaused` を `true` に設定してください。デフォルトは `false` で、ゲームがポーズ中はトランジションの Tick が停止します。ポーズメニューでトランジションを使用する場合は、このフラグを有効にする必要があります。

### Q: マテリアルインスタンスが見つからないというエラーが出ます。

以下の原因が考えられます：
- プリセットが参照しているマテリアルアセットがコンテンツブラウザで**移動またはリネーム**された。プリセット内のマテリアルを再設定してください。
- プラグインのコンテンツが正しくコピーされていない。`Plugins/TransitionFX/Content/Materials/` ディレクトリが存在し、`M_Transition_*.uasset` ファイルが含まれていることを確認してください。
- `TextureMask` エフェクトを使用している場合、マスクテクスチャの **sRGB がオフ**になっており、圧縮設定が **Masks (no sRGB)** または **Grayscale** になっていることを確認してください。

---

## 仕様・挙動について

### Q: トランジション中の入力自動ブロックはどのように動作しますか？

プリセットの `bAutoBlockInput` が `true`（デフォルト）の場合、サブシステムはトランジション開始時に PlayerController の `SetCinematicMode(true)` を呼び出し、プレイヤー入力を無効化してカーソルを非表示にします。トランジション完了時（または `ForceClear` 呼び出し時）にシネマティックモードが解除されます。
- 入力は自動でブロックされるため、追加の Blueprint ロジックは不要です。
- `StopTransition` で中断した場合も入力はアンブロックされます。
- `ForceClear` で中断した場合は、設定に関係なく強制的にアンブロックされます。

### Q: ServerTravel / シームレストラベルでも使えますか？

TransitionFX は **GameInstance Subsystem** として動作するため、サブシステム自体は `ServerTravel` を含むレベル遷移後も生存します。ただし、自動フェードイン機能（`PostLoadMapWithWorld`）は `OpenLevel` ベースの遷移向けに設計されています。`ServerTravel` の場合は、トラベル完了後にクライアント側で手動でフェードインをトリガーする必要があります。`OnPostLoadMapWithWorld` にバインドするか、ゲーム固有のコールバックを使用して `StartTransition` を Reverse モードで呼び出してください。

### Q: オブジェクトプーリングの仕組みはどうなっていますか？

サブシステムは内部的にトランジションエフェクトインスタンスのプール（`TMap<UClass*, FTransitionEffectPool>`）を維持しています。トランジション終了時、エフェクトオブジェクトは破棄される代わりにプールに返却されます。次に同じエフェクトクラスが要求された際に、プールされたインスタンスが再利用されます。

- **プール上限:** エフェクトクラスごとに最大 **3 インスタンス**
- **超過分の処理:** 上限を超えたインスタンスは参照が解除され、Unreal の GC によって回収されます
- **ユーザーによる設定は不要** — プーリングは完全に自動で動作します

### Q: 再生中に音量やピッチを動的に変更できますか？

いいえ。`TransitionPreset` の `SoundVolume` と `SoundPitch` はサウンドの再生開始時に適用されます。トランジション再生中にこれらを変更する API はありません。動的なオーディオ制御が必要な場合は、`OnTransitionStarted` デリゲートを使用して独自のオーディオロジックをトリガーし、サウンドを別途管理してください。

### Q: なぜ PlayerController コンポーネントではなく GameInstance Subsystem を使っているのですか？

GameInstance Subsystem はレベル遷移をまたいで永続化されます。典型的なトランジションシーケンス — フェードアウト → レベルオープン → フェードイン — は 2 つの異なるレベルにまたがります。PlayerController コンポーネントは旧レベルのアンロード時に破棄され、トランジション状態が失われてしまいます。GameInstance Subsystem は、保留中のプリセットや自動リバースフラグなどのすべての状態をレベル境界をまたいでシームレスに維持します。

### Q: TransitionFX が依存するモジュールは何ですか？

ランタイムモジュール（`TransitionFX`）の依存先：
- `Core`、`CoreUObject`、`Engine`、`InputCore` — 標準エンジンモジュール
- `UMG`、`Slate`、`SlateCore` — UI フレームワーク（CinematicMode による入力ブロックに使用）
- `RenderCore` — レンダリングユーティリティ（マテリアル / PostProcess 操作に使用）
- `DeveloperSettings` — 設定サポート

エディタモジュール（`TransitionFXEditor`）は追加で `UnrealEd`、`EditorStyle` および関連エディタモジュールに依存します。

サードパーティの依存関係はありません。

### Q: Latent Action パターンとは何ですか？

Latent Action は UE の Blueprint 機能で、非同期操作が完了するまでノードの**実行を一時停止**できるものです。TransitionFX では `Play Transition And Wait` が Latent Action として実装されており、トランジションを開始して完了まで実行ピンを保持し、完了後に `Completed` ピンを発火します。基本的な使用においては、手動でのタイマーチェック、ポーリング、コールバックバインドが不要になります。

### Q: 商用プロジェクトで TransitionFX を使用できますか？

はい。TransitionFX は **MIT ライセンス**の下で公開されており、ライセンス表示を含めることを条件に、商用利用、改変、再配布が制限なく許可されています。詳細はリポジトリルートの `LICENSE` ファイルを参照してください。

---

## 応用・ベストプラクティス

### Q: エフェクトごとのマテリアルパラメータを実行時にオーバーライドする方法は？

`Play Transition And Wait`（または `StartTransition`）の `Override Params` ピンを使用します。`FTransitionParameters` 構造体を作成し、各マップに値を設定します。

**エフェクト別の主なパラメータ名：**

| エフェクト | パラメータ名 | 型 | 説明 |
| :--- | :--- | :--- | :--- |
| 全エフェクト共通 | `Color` | Vector (LinearColor) | トランジションカラー（デフォルト: 黒） |
| Linear Wipe | `Angle` | Scalar (float) | ワイプの角度 |
| Split | `Angle` | Scalar (float) | 分割方向の角度 |
| Tiles / Polka Dots / Blinds / Checkerboard | `TileCount` | Scalar (float) | タイル数 / ドット数 / ストライプ数 |
| Spiral | `Spin` | Scalar (float) | 回転の強さ |
| Flower Iris | `PetalCount` | Scalar (float) | 花びらの数 |
| Random Tiles | `Seed` | Scalar (float) | ランダムシード値 |
| Wavy Curtain | `WaveFrequency` | Scalar (float) | 波の周波数 |
| Texture Mask | `MaskTexture` | Texture | カスタムグレースケールマスク |

> **Note:** パラメータ名はマテリアルの実装に依存します。正確なパラメータ名はマスターマテリアルをマテリアルエディタで開いて確認してください。

### Q: トランジションごとに異なるサウンドを設定できますか？

はい。各 `TransitionPreset` データアセットには `Transition Sound`、`Sound Volume`、`Sound Pitch` プロパティがあります。異なるプリセットに異なるサウンドアセットを割り当てるだけです。トランジション再生時は、アクティブなプリセットに設定されたサウンドが使用されます。

### Q: リストからランダムにトランジションを再生する方法は？

`Play Random Transition And Wait` ノードを使用します。`Presets` ピンに `TransitionPreset` 参照の配列を渡します。ノードはランダムに 1 つを選択して再生します。

```
Play Random Transition And Wait
├─ Presets   : [DA_Iris, DA_Diamond, DA_Spiral, DA_Hexagon]
├─ Mode      : Forward
└─ Play Speed: 1.0
```

重み付きランダムや直前のエフェクトの繰り返し回避が必要な場合は、Blueprint 側で選択ロジックを管理し、選択したプリセットで `Play Transition And Wait` を使用してください。

### Q: 再生速度を動的に変更できますか？

`TransitionManagerSubsystem` の `SetPlaySpeed` をトランジション再生中にいつでも呼び出せます。値は最小 0.01 にクランプされます。

```
Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Set Play Speed
        └─ Play Speed: 2.0（2倍速）
```

使用例：
- プレイヤーが「スキップ」ボタンを押した際にトランジションを高速化する
- ドラマチックな演出のためにトランジションを遅くする
- ゲームプレイイベントとトランジション速度を同期させる
