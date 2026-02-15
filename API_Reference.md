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
