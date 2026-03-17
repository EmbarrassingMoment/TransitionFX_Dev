# TransitionFX Preview Tool Manual

## 1. Overview

TransitionFX Preview Tool は、TransitionFX プラグインに含まれる 23 種類の SDF ベースのスクリーントランジションをエディタ上で直接プレビューするためのツールです。

- **PIE（Play In Editor）不要** -- エディタ内で即座にエフェクトを確認できます
- **GIF 撮影に最適** -- 固定サイズのビューポートで ScreenToGif 等の外部キャプチャツールとの併用を想定しています
- **再生制御** -- 再生・逆再生・ループ・速度調整・手動スクラブに対応しています

---

## 2. ツールの開き方

エディタのメニューバーから開きます。

```
Tools > TransitionFX > TransitionFX Preview
```

ドッキング可能なタブとして開くため、レイアウトの好きな場所に配置できます。

---

## 3. UI の使い方

パネルは上から順に以下の構成です。

### 3.1 エフェクト選択 & Invert

| コントロール | 説明 |
|---|---|
| **Effect Dropdown** | プラグインに含まれるトランジションエフェクトを選択します。`/TransitionFX/Materials/Instances/` 内の全マテリアルインスタンスが自動的に一覧表示されます。 |
| **Invert** チェックボックス | トランジションのマスクを反転します。例えば「画面が黒く覆われる」エフェクトを「黒から元に戻る」方向に切り替えます。 |

### 3.2 プレビュービューポート

白背景の上にポストプロセスマテリアルとしてトランジションエフェクトが描画されます。サイズは下部の **Size** ドロップダウンで変更できます。

### 3.3 Progress スライダー

| コントロール | 説明 |
|---|---|
| **Progress スライダー** | ドラッグしてトランジションの進行度（0% 〜 100%）を手動で調整します。ドラッグ中は自動再生が一時停止します。 |
| **数値表示** | スライダーの右側に現在の進行度（0 〜 100）が表示されます。 |
| **Reset** ボタン | 進行度を 0% に戻し、再生を停止します。 |

### 3.4 再生コントロール

| ボタン | 説明 |
|---|---|
| **Play** | 順方向に再生を開始します。進行度が 100% に達している場合は 0% から再開します。 |
| **Reverse** | 逆方向に再生を開始します。進行度が 0% の場合は 100% から再開します。 |
| **Stop** | 再生を停止します。現在の進行度は維持されます。 |
| **Loop: ON / OFF** | ループの切り替えです。ON にすると、端に到達した際にピンポン（自動反転）方式で繰り返し再生します。 |

### 3.5 Speed & Size

| コントロール | 説明 |
|---|---|
| **Speed** | 再生速度の倍率です。0.1x 〜 5.0x の範囲で 0.1 刻みに調整できます。デフォルトは 1.0x です。 |
| **Size** | ビューポートの解像度を選択します。`480x270`（デフォルト）、`640x360`、`800x450` から選べます。 |

---

## 4. GIF 撮影のワークフロー

1. **Size を選択** -- キャプチャしたい解像度をドロップダウンから選びます
2. **エフェクトを選択** -- ドロップダウンからプレビューしたいトランジションを選びます
3. **ScreenToGif を起動** -- ビューポート領域に合わせてキャプチャ範囲を設定します
4. **Speed を調整**（任意） -- 必要に応じて再生速度を変更します
5. **ScreenToGif の録画を開始**
6. **Play ボタンをクリック** -- トランジションが再生されます
7. **録画を停止** -- GIF として保存します

> **Tip:** Loop を ON にしておくと、ピンポン再生が繰り返されるため、ベストなタイミングでの録画開始・停止がやりやすくなります。

---

## 5. 新しいトランジションマテリアルの追加方法

自作のトランジションマテリアルを Preview Tool に表示させるための手順です。

### 5.1 前提条件

トランジションマテリアルは **Post Process Material** として動作する必要があります。

- マテリアルの **Material Domain** を `Post Process` に設定してください
- マテリアルの **Blendable Location** を `After Tonemapping` に設定してください

### 5.2 必須パラメータ

マテリアル（または親マテリアル）に以下の **Scalar Parameter** を持たせてください。

| パラメータ名 | 型 | 値の範囲 | 説明 |
|---|---|---|---|
| `Progress` | Scalar | 0.0 〜 1.0 | トランジションの進行度。0.0 で開始状態、1.0 で完了状態。 |
| `Invert` | Scalar | 0.0 または 1.0 | マスクの反転フラグ。マテリアル内で If ノード（閾値 0.5）を使用して切り替えます。 |

### 5.3 マテリアルインスタンスの作成手順

1. **親マテリアルを作成**（または既存の `M_Transition_Master` を親にする）
   - `Content/TransitionFX/Materials/` にマテリアルを作成
   - Material Domain: `Post Process`
   - `Progress` と `Invert` の Scalar Parameter を追加

2. **マテリアルインスタンスを作成**
   - 親マテリアルから Material Instance を作成します

3. **命名規則に従ってリネーム**

   ```
   MI_Transition_<エフェクト名>
   ```

   例: `MI_Transition_MyCustomWipe`

   > Preview Tool のドロップダウンには `MI_Transition_` プレフィックスが自動的に除去された名前（例: `MyCustomWipe`）で表示されます。

4. **所定のフォルダに保存**

   ```
   /TransitionFX/Materials/Instances/
   ```

   Preview Tool は起動時にこのフォルダ内の `MaterialInstanceConstant` アセットを自動検出します。

5. **Preview Tool を再度開く** -- ドロップダウンに新しいエフェクトが表示されます

### 5.4 既存マテリアルの参考一覧

プラグインには以下の 23 種類のトランジションが含まれています。新規作成時の参考にしてください。

| エフェクト名 | マテリアルインスタンス |
|---|---|
| Blinds | `MI_Transition_Blinds` |
| Box | `MI_Transition_Box` |
| Checkerboard | `MI_Transition_Checkerboard` |
| CrossWipe | `MI_Transition_CrossWipe` |
| Diamond | `MI_Transition_Diamond` |
| Fade | `MI_Transition_Fade` |
| FlowerIris | `MI_Transition_FlowerIris` |
| Heart | `MI_Transition_Heart` |
| Hexagon | `MI_Transition_Hexagon` |
| Iris | `MI_Transition_Iris` |
| LinearWipe | `MI_Transition_LinearWipe` |
| Pixelate | `MI_Transition_Pixelate` |
| PolkaDots | `MI_Transition_PolkaDots` |
| RadialWipe | `MI_Transition_RadialWipe` |
| RandomTiles | `MI_Transition_RandomTiles` |
| Split | `MI_Transition_Split` |
| Spiral | `MI_Transition_Spiral` |
| TVSwitchOff | `MI_Transition_TVSwitchOff` |
| TextureMask | `MI_Transition_TextureMask` |
| Tiles | `MI_Transition_Tiles` |
| WavyCurtain | `MI_Transition_WavyCurtain` |
| Wind | `MI_Transition_Wind` |
| ZoomWipe | `MI_Transition_ZoomWipe` |

---

## 6. ランタイムでの使用（プリセット連携）

Preview Tool で確認したマテリアルをゲーム内のトランジションとして使用するには、**Transition Preset** を作成します。

### 6.1 Transition Preset の作成

1. コンテンツブラウザで右クリック > **Miscellaneous > Data Asset** を選択
2. クラスとして `TransitionPreset` を選択
3. 以下のプロパティを設定します:

| プロパティ | 説明 |
|---|---|
| `TransitionMaterial` | 使用するマテリアルインスタンス（例: `MI_Transition_MyCustomWipe`） |
| `DefaultDuration` | トランジションの再生時間（秒）。デフォルト: 1.0 |
| `EasingType` | イージングの種類（Linear, EaseInSine, EaseOutBounce 等） |
| `Priority` | PostProcess の優先度。デフォルト: 1000.0 |
| `bAutoBlockInput` | トランジション中にプレイヤー入力をブロックするか |
| `bTickWhenPaused` | ゲームがポーズ中でもトランジションを進行させるか |
| `TransitionSound` | トランジション再生時に鳴らすサウンド（任意） |

### 6.2 Blueprint からの呼び出し

```
Play Transition And Wait
├── Preset: 作成した TransitionPreset アセット
├── Mode: TransitionIn / TransitionOut / TransitionInOut
├── Speed: 再生速度の倍率（デフォルト: 1.0）
└── Invert: 反転フラグ
```

その他の便利な Blueprint 関数:

- `Quick Fade To Black` / `Quick Fade From Black` -- 簡易フェード
- `Open Level With Transition` -- レベル遷移時にトランジション付きで切り替え
- `Is Any Transition Playing` -- トランジション再生中かどうかを確認
