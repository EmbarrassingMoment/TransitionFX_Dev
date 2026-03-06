# TransitionFX API Reference

このドキュメントでは、TransitionFXプラグインのAPIについて詳しく解説します。

## 1. はじめに (Introduction)

TransitionFXプラグインは、主に以下の2つのコンポーネントで構成されています。

*   **UTransitionManagerSubsystem**: 遷移効果の実行、管理、およびティック処理を行うシングルトンサブシステムです。
*   **UTransitionPreset**: 遷移効果の設定（エフェクトクラス、マテリアル、持続時間、イージングタイプなど）を定義するデータアセットです。

## 2. ブループリントAPI (Blueprint API Reference)

主な機能は `UTransitionBlueprintLibrary` を通じて提供され、レイテントアクション（Latent Action）として実装されています。

### レイテントアクション (Latent Action Nodes)

これらのノードは、遷移が完了するまで実行を待機します。

#### Play Transition And Wait
指定されたプリセットを使用して遷移を再生し、完了まで待機します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Preset** | Input | 使用する遷移プリセット (`UTransitionPreset`)。 |
| **Mode** | Input | 遷移モード (`Forward`: フェードアウト/0→1, `Reverse`: フェードイン/1→0)。 |
| **Play Speed** | Input | 再生速度の倍率 (デフォルト: 1.0)。 |
| **bInvert** | Input | マスクを反転するかどうか。 |
| **Override Params** | Input | マテリアルパラメータを動的に上書きするための構造体 (`FTransitionParameters`)。 |
| **Completed** | Output | 遷移が完了した後に実行されます。 |

#### Play Transition And Wait With Duration
再生速度の代わりに、具体的な持続時間（秒）を指定して遷移を再生します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Preset** | Input | 使用する遷移プリセット (`UTransitionPreset`)。 |
| **Mode** | Input | 遷移モード (`Forward`, `Reverse`)。 |
| **Duration** | Input | 遷移の持続時間（秒）。 |
| **bInvert** | Input | マスクを反転するかどうか。 |
| **Override Params** | Input | マテリアルパラメータを動的に上書きするための構造体。 |
| **Completed** | Output | 遷移が完了した後に実行されます。 |

#### Play Random Transition And Wait
指定されたプリセットのリストからランダムに1つを選択して再生します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Presets** | Input | 選択対象となる遷移プリセットの配列。 |
| **Mode** | Input | 遷移モード (`Forward`, `Reverse`)。 |
| **Play Speed** | Input | 再生速度の倍率。 |
| **bInvert** | Input | マスクを反転するかどうか。 |
| **Override Params** | Input | マテリアルパラメータを動的に上書きするための構造体。 |
| **Completed** | Output | 遷移が完了した後に実行されます。 |

### 制御ノード (Control Nodes)

#### Stop Transition
現在再生中の遷移を停止します。

#### Force Clear
すべての遷移状態を強制的にクリアし、入力をリセットします。緊急停止やリセットに使用します。

### レベル遷移ノード (Level Transition Nodes)

#### Open Level With Transition
「フェードアウト → レベルオープン → フェードイン」のシーケンスを処理します。新レベル側のフェードインは `PostLoadMapWithWorld` を通じて自動的に開始されます。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Level Name** | Input | 開くレベルの名前。 |
| **Preset** | Input | 使用する遷移プリセット。 |
| **Duration** | Input | 遷移の持続時間（フェードアウトとフェードインの両方に適用）。デフォルト: 1.0。 |

#### Open Level With Transition And Wait
遷移（フェードアウト）を再生し、完了を待ってから指定されたレベルを開きます。新レベルでは自動的にリバース遷移（フェードイン）が再生されます。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Level Name** | Input | 開くレベルの名前。 |
| **Preset** | Input | 使用する遷移プリセット。 |
| **Duration** | Input | 遷移の持続時間（フェードアウトとフェードインの両方に適用）。 |
| **Completed** | Output | フェードアウトが完了し `OpenLevel` が呼ばれた後に実行されます。 |

### ユーティリティノード (Utility Nodes)

#### Quick Fade To Black
デフォルトの `DA_FadeToBlack` プリセットを使用して、画面を素早く黒にフェードします。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Duration** | Input | フェードの持続時間（デフォルト: 1.0）。 |

#### Quick Fade From Black
デフォルトの `DA_FadeToBlack` プリセットを使用して、画面を黒から素早くフェードインします。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Duration** | Input | フェードの持続時間（デフォルト: 1.0）。 |

#### Is Any Transition Playing
現在遷移が再生中かどうかを返します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Return Value** | Output | 遷移がアクティブな場合 True。 |

#### Apply Easing
指定されたアルファ値にイージング関数を適用します。カスタムアニメーションカーブに便利な Pure 数学ノードです。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Alpha** | Input | 入力アルファ値（0.0〜1.0）。 |
| **Easing Type** | Input | 適用するイージングタイプ (`ETransitionEasing`)。 |
| **Custom Curve** | Input | EasingType が `Custom` の場合に使用するオプションのカスタムカーブ。 |
| **Return Value** | Output | イージングが適用されたアルファ値。 |

## 3. C++ API (C++ API Reference)

C++からは `UTransitionManagerSubsystem` を通じて遷移を制御します。

### サブシステムの取得

```cpp
UTransitionManagerSubsystem* TransitionSystem = GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
```

### 主要関数

#### StartTransition
遷移を開始します。

```cpp
void StartTransition(
    UTransitionPreset* Preset,
    ETransitionMode Mode = ETransitionMode::Forward,
    float PlaySpeed = 1.0f,
    bool bInvert = false,
    bool bHoldAtMax = false,
    FTransitionParameters OverrideParams = FTransitionParameters()
);
```

#### StopTransition
現在の遷移を停止します。

```cpp
void StopTransition();
```

#### ReverseTransition
現在の遷移の再生方向を反転します（例：フェードアウトからフェードインへ）。

```cpp
void ReverseTransition(bool bAutoStop = true);
```

#### SetPlaySpeed
再生速度の乗数を動的に変更します。

```cpp
void SetPlaySpeed(float PlaySpeed = 1.0f);
```

#### GetCurrentProgress
現在の遷移の進行度（0.0〜1.0）を返します。

```cpp
float GetCurrentProgress() const;
```

#### IsTransitionPlaying
遷移が現在アクティブな場合に true を返します。

```cpp
bool IsTransitionPlaying() const;
```

#### IsCurrentTransitionFinished
現在の遷移がホールドフェーズを完了したか、完了している場合に true を返します。ポーリングに便利です。

```cpp
bool IsCurrentTransitionFinished() const;
```

#### ForceClear
アクティブな遷移を強制的にクリアし、入力をリセットします。緊急復旧に使用します。

```cpp
void ForceClear();
```

#### OpenLevelWithTransition
「フェードアウト → レベルオープン → フェードイン」のシーケンスを処理します。レベル遷移をまたいで状態が維持されます。

```cpp
void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);
```

#### PrepareAutoReverseTransition
次のレベルロード時に自動リバース遷移を行うようサブシステムを準備します。即座に遷移を開始することはありません。

```cpp
void PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration = 1.0f);
```

### デリゲート (Delegates / Event Dispatchers)

サブシステムには以下のイベントディスパッチャーが用意されています。

*   **OnTransitionStarted**: 遷移が開始されたときに呼び出されます。
*   **OnTransitionCompleted**: 遷移が完了したときに呼び出されます。
*   **OnTransitionHoldStarted**: 遷移が最大進行度（1.0）でホールド（一時停止）されたときに呼び出されます（`bHoldAtMax` が true の場合）。

※ `OnTransitionStop` という名前のデリゲートは存在しませんが、`StopTransition` を呼び出すと遷移は中断されます。

## 4. 高度な機能 (Advanced Features)

### 非同期ロード (Async Loading)
`AsyncLoadTransitionPresets` を使用すると、ソフトオブジェクト参照 (`TSoftObjectPtr`) のリストを使用してプリセットを非同期にロードし、シェーダーのプリロードを行うことができます。

```cpp
void AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete);
```

### プリロード (Preloading)
`PreloadTransitionPresets` は、ロード済みのプリセットを使用してシェーダーを事前にウォームアップし、遷移実行時のヒッチ（カクつき）を防ぎます。

```cpp
void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);
```

## 5. パラメータのオーバーライド (Parameter Overrides)

`FTransitionParameters` 構造体を使用することで、プリセットのマテリアルパラメータを実行時に動的に変更できます。

```cpp
FTransitionParameters Params;
Params.ScalarParams.Add(FName("Intensity"), 2.0f);
Params.VectorParams.Add(FName("Color"), FLinearColor::Red);
// TextureParams も同様に設定可能
```

この構造体は `StartTransition` や各種ブループリントノードに渡すことができます。

## 6. 手動制御とロード画面のワークフロー (Manual Control & Loading Screen Workflow)

TransitionFXは、遷移を自動再生するだけでなく、手動でタイミングを制御する機能を提供しています。特に、レベルロード中や重い処理の間に画面を隠す「ロード画面」として使用する場合に便利です。

### 重要なパラメータと関数

*   **bHoldAtMax (StartTransition)**:
    このパラメータを `true` に設定すると、遷移エフェクトは進行度が **1.0 (画面が完全に覆われた状態)** に達した時点で自動的に一時停止（ホールド）します。この状態では入力はブロックされたままになります。

*   **ReleaseHold**:
    ホールド状態にある遷移を再開し、完了させるための関数です。これを呼び出すと、遷移はホールド状態から抜け出し、シームレスに終了処理へ移行します。

### 実装ワークフロー例

1.  **遷移を開始 (Start Transition)**
    *   `StartTransition` (またはブループリントノード) を呼び出し、`bHoldAtMax` を `true` に設定します。
    *   画面がエフェクトで覆われ、進行度が 1.0 で停止します。

2.  **バックグラウンド処理 (Load Level / Spawn Actors)**
    *   画面が隠れている間に、レベルのロードやアクターのスポーンなど、プレイヤーに見せたくない処理を行います。

3.  **ホールド解除 (Call ReleaseHold)**
    *   ロード処理が完了したら、`ReleaseHold` 関数を呼び出します。

4.  **遷移完了 (Transition Finishes)**
    *   エフェクトが再開し、違和感なく遷移が終了します。

## 7. デバッグとユーティリティ (Debugging & Utilities)

開発中やデバッグ時に役立つ機能を提供しています。

### 強制クリア (Force Clear)

*   **ブループリントノード**: `ForceClear`
    *   すべての遷移状態を即座に破棄し、入力ブロックを解除します。遷移が予期せずスタックした場合の復旧に使用します。

*   **コンソールコマンド**: `TransitionFX.ForceClear`
    *   **重要**: QAテスターや開発者は、コンソールウィンドウ（`@` または `~` キー）でこのコマンドを入力することで、画面が真っ暗なままスタックする状態から即座に復旧できます。
