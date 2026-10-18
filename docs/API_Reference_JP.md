# TransitionFX API Reference

このドキュメントでは、TransitionFXプラグインのAPIについて詳しく解説します。

## 1. はじめに (Introduction)

TransitionFXプラグインは、主に以下のコンポーネントで構成されています。

*   **UTransitionManagerSubsystem**: 遷移効果の実行、管理、およびティック処理を行うシングルトンサブシステムです。
*   **UTransitionPreset**: 遷移効果の設定（エフェクトクラス、マテリアル、持続時間、イージングタイプなど）を定義するデータアセットです。`Effect Class` で描画経路を選びます。`PostProcessTransitionEffect`（PostProcess ボリューム、`Priority` で順序付け）か `WidgetTransitionEffect`（UMG/Slate UI も覆うフルスクリーン Slate オーバーレイ、`WidgetZOrder`（既定 10000）で順序付け、`MI_Widget_*` などの UI ドメインマテリアルが必要）です。
*   **UTransitionSequence**: 複数のプリセットを連続再生するためのデータアセットです（[シーケンスノード](#シーケンスノード-sequence-nodes)を参照）。
*   **UTransitionBlueprintLibrary**: 後述の Blueprint ノード（レイテントアクション、レベル遷移、クイックフェード、ヘルパー）です。
*   **UTransitionFXSettings**: **プロジェクト設定 > プラグイン > TransitionFX** のプロジェクト設定です（[オブジェクトプーリング](#オブジェクトプーリング-object-pooling)を参照）。

### エフェクトが消えるタイミング

トランジション完了時の動作は、画面を覆った状態で終わるかどうかで変わります。

*   **覆った状態で終わる** — Invert なしの `Forward`（フェードアウト）、または Invert ありの `Reverse`：`OnTransitionCompleted` が発火し、エフェクトは完全に覆った状態のまま**画面に残ります**。ロードや配置替えの間も背後が見えません。プリセットの `bAutoBlockInput` が有効なら入力もブロックされたままです。エフェクトは、次のトランジションを開始したとき（同じフレーム内で差し替え）、`Reverse Transition` / `Invert Transition` で再開したとき、`Stop Transition` / `Force Clear` を呼んだとき、または別のレベルへ遷移したときに取り除かれます。
*   **見える状態で終わる** — Invert なしの `Reverse`（フェードイン）、または Invert ありの `Forward`：自動的に停止してクリーンアップされます（エフェクト削除、入力復帰）。

どちらの場合も、完了後は `Is Transition Playing` が false、`Is Current Transition Finished` が true を返します。

## 2. ブループリントAPI (Blueprint API Reference)

主な機能は `UTransitionBlueprintLibrary` を通じて提供され、レイテントアクション（Latent Action）として実装されています。

### レイテントアクション (Latent Action Nodes)

これらのノードは、遷移が完了するまで実行を待機します。遷移が完了前に停止された場合（`Stop Transition` / `Force Clear`）や開始に失敗した場合（無効なプリセットなど）、`Play Transition And Wait`、`Play Transition And Wait With Duration`、`Play Random Transition And Wait` は待機をやめて **Completed** を実行するため、Blueprint が待ち続けることはありません。

#### Play Transition And Wait

![Play Transition And Wait Latent Action ノードの Blueprint スクリーンショット](images/api_play_transition_node.png)

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
指定されたプリセットのリストからランダムに1つを選択して再生します。配列が空の場合、または選択されたプリセットが null の場合、遷移を再生せずに即座に完了します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Presets** | Input | 選択対象となる遷移プリセットの配列。 |
| **Mode** | Input | 遷移モード (`Forward`, `Reverse`)。 |
| **Play Speed** | Input | 再生速度の倍率。 |
| **bInvert** | Input | マスクを反転するかどうか。 |
| **Override Params** | Input | マテリアルパラメータを動的に上書きするための構造体。 |
| **Completed** | Output | 遷移が完了した後に実行されます。 |

### サブシステムノード (Subsystem Nodes)

以下のノードは `UTransitionManagerSubsystem` インスタンスに対して直接呼び出します。サブシステムは `Get Game Instance Subsystem` で取得します。

#### Start Transition
指定されたプリセットで遷移を開始します。Latent Actionノードとは異なり、完了を待機しません。ロード画面ワークフロー向けの `bHoldAtMax` をサポートしています。

完了時、フェードアウト（Invert なしの Forward）は次のトランジション、`Reverse Transition` / `Invert Transition`、または `Stop Transition` まで画面を覆い続けます。フェードイン（Invert なしの Reverse）は自動的に停止してクリーンアップされます。[エフェクトが消えるタイミング](#エフェクトが消えるタイミング)を参照してください。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Preset** | Input | 使用する遷移プリセット (`UTransitionPreset`)。null の場合、またはマテリアルか `Effect Class` が未設定の場合、`ForceClear` が呼ばれエラーがログ出力されます。 |
| **Mode** | Input | 遷移モード (`Forward`: フェードアウト/0→1, `Reverse`: フェードイン/1→0)。 |
| **Play Speed** | Input | 再生速度の倍率 (デフォルト: 1.0)。最小値 0.01 にクランプされます。 |
| **bInvert** | Input | マスクを反転するかどうか。 |
| **bHold At Max** | Input | true の場合、遷移は完了せずに進行度 1.0 でホールドします。 |
| **Override Params** | Input | マテリアルパラメータを動的に上書きするための構造体 (`FTransitionParameters`)。 |

#### Stop Transition
現在再生中の遷移を停止します。完了したフェードアウトが画面を覆ったまま残っている場合は、そのエフェクトを取り除きます。入力は元に戻り、`OnTransitionCompleted` は発火しません。`Open Level With Transition` / `Open Level With Transition And Wait` のフェードアウトを完了前に停止すると、そのレベル遷移はキャンセルされます（レベル遷移ノードを参照）。

#### Force Clear
すべての遷移状態を強制的にクリアし、入力をリセットします。再生中のシーケンスと保留中のレベル遷移（次のレベルロード時に予定された自動フェードインを含む）もキャンセルします。緊急停止やリセットに使用します。

#### Reverse Transition
現在の遷移の再生方向を反転します（例：フェードアウトからフェードインへ）。再生中、最大進行度でのホールド中、そして完了したフェードアウトが画面を覆ったまま残っている間に使えます。見える状態で終わって停止した後は反転する対象がないため、警告を出して何もしません。`bAutoStop` が true（デフォルト）の場合はリバース完了時に自動的に終了処理され、false の場合は進行度 0 のままアクティブに残ります。

#### Invert Transition
現在の遷移のマスク反転状態をトグルし、反転したマスクで最初から（0→1）再生し直します。`Reverse Transition` と同様に、完了したフェードアウトが画面を覆ったまま残っている状態から呼ぶとフェードインになります。`bAutoComplete` が true（デフォルト）の場合は通常どおり完了し、false の場合は最大進行度でホールドします（`Release Hold` で解除）。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **bAutoComplete** | Input | true（デフォルト）で通常完了。false で進行度 1.0 でホールドします。 |

#### Set Play Speed
再生速度の乗数を動的に変更します。値は最小 0.01 にクランプされます。

#### Release Hold
最大進行度でのホールドを解除し、ホールド中の遷移を完了させます。`bHoldAtMax` ワークフローで使用します。ホールド中のフェードアウトを解除すると完了し（`OnTransitionCompleted` が発火）、画面は覆われたままになります。フェードインさせるには、代わりに `Reverse Transition` / `Invert Transition` を呼ぶか（ホールド中から直接呼べます）、Reverse のトランジションを開始してください。[手動制御とロード画面のワークフロー](#6-手動制御とロード画面のワークフロー-manual-control--loading-screen-workflow)を参照。

#### Add Progress Threshold
進捗の閾値（0.0〜1.0 にクランプ）を登録します。遷移中にイージング適用済みの進捗がこの値を（どちらの方向でも）超えると、`OnProgressThresholdReached` が1回だけブロードキャストされます。登録した閾値は新しい遷移ごとに再び有効になり、`Clear Progress Thresholds` を呼ぶまで登録されたままです。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Threshold** | Input | 0.0〜1.0 の閾値。 |

#### Clear Progress Thresholds
登録されているすべての進捗閾値を削除します。

### クエリノード (Query Nodes)

#### Get Current Progress
現在の遷移の進行度（0.0〜1.0）を**イージング適用前**の値（時間に比例する値）で返します。`OnTransitionProgressChanged` と進捗閾値はイージング適用後の値を使います。`TransitionManagerSubsystem` で利用可能です。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Return Value** | Output | イージング適用前の遷移の進行度（0.0〜1.0）。 |

#### Is Transition Playing
遷移が再生中またはホールド中の場合に true を返します。完了後は、完了したフェードアウトが画面を覆ったまま残っていても false を返します。`TransitionManagerSubsystem` で利用可能です。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Return Value** | Output | 遷移がアクティブな場合 True。 |

#### Is Current Transition Finished
現在の遷移が完了した（`OnTransitionCompleted` が発火した）後に true を返します。ホールドした遷移は、ホールドを解除して完了してから true になります。別の遷移が開始されるか、`Reverse Transition` / `Invert Transition` で再開されるまで true のままです。ポーリングに便利です。`TransitionManagerSubsystem` で利用可能です。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Return Value** | Output | 遷移が完了している場合 True。 |

### レベル遷移ノード (Level Transition Nodes)

#### Open Level With Transition

![Open Level With Transition ノード](images/api_open_level_node.png)

「フェードアウト → レベルオープン → フェードイン」のシーケンスを処理します。フェードアウトの完了から旧レベルが破棄されるまで画面は覆われたままで、新レベル側のフェードインは `PostLoadMapWithWorld` を通じて自動的に開始されます。`Preset` が null の場合はデフォルトの `DA_FadeToBlack` プリセットを使用します。再生可能なプリセットがない場合（デフォルトプリセットをロードできない、またはプリセットのマテリアルか `Effect Class` が未設定）は、遷移なしでレベルを開き、エラーをログ出力します。

フェードアウトを完了前に停止した場合（`Stop Transition` / `Force Clear`）や、別の遷移（`Start Transition`、Play Transition 系や Quick Fade 系のノード、別のレベル遷移）で置き換えた場合は、レベル遷移がキャンセルされます。レベルは開かれず、次のレベルロード時のフェードインも再生されず、シーケンスもすぐに再生できるようになります。フェードアウトが完了した後は、通常どおりレベルが開かれます。

*注: 内部的に、自動フェードインは `Reverse` モードではなく `Forward` モード + `bInvert=true` を使用します。視覚的な結果は同等です。*

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Level Name** | Input | 開くレベルの名前。 |
| **Preset** | Input | 使用する遷移プリセット。 |
| **Duration** | Input | 遷移の持続時間（フェードアウトとフェードインの両方に適用）。デフォルト: 1.0。 |

#### Open Level With Transition And Wait
遷移（フェードアウト）を再生し、完了を待ってから指定されたレベルを開きます。新レベルでは自動的にリバース遷移（フェードイン）が再生されます。`Preset` が null の場合は `DA_FadeToBlack` を使用し、再生可能なプリセットがない場合は遷移なしでただちにレベルを開いてエラーをログ出力し、**Completed** を実行します。フェードアウトが完了前に停止または置き換えられた場合（`Open Level With Transition` と同じ条件）は、レベルを開かず、自動フェードインもキャンセルし、警告をログ出力して **Completed** を実行します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Level Name** | Input | 開くレベルの名前。 |
| **Preset** | Input | 使用する遷移プリセット。 |
| **Duration** | Input | 遷移の持続時間（フェードアウトとフェードインの両方に適用）。 |
| **Completed** | Output | フェードアウトが完了し `OpenLevel` が呼ばれた後、またはレベル遷移がキャンセルされたとき（この場合レベルは開かれません）に実行されます。 |

### ユーティリティノード (Utility Nodes)

#### Quick Fade To Black

![Quick Fade To Black / Quick Fade From Black ノードの Blueprint スクリーンショット](images/api_quick_fade_node.png)

画面を素早く黒にフェードします。画面は `Quick Fade From Black`（または別のトランジション）が実行されるまで黒のままです。これは遷移の完了を待たない **Fire-and-Forget** 関数です。

どちらの Quick Fade ノードも、デフォルトの `DA_FadeToBlack` プリセットからエフェクトクラスとマテリアルだけを借りた一時プリセットで再生します。色は常に黒、イージングは `Linear`、入力はブロックされ、サウンドは再生されません。ほかの設定が必要な場合は、独自のプリセットで `Play Transition And Wait` を使ってください。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Duration** | Input | フェードの持続時間（デフォルト: 1.0）。 |

#### Quick Fade From Black
画面を黒から素早くフェードインします（`Quick Fade To Black` と同じ方法で作った Reverse のフェードです）。これは遷移の完了を待たない **Fire-and-Forget** 関数です。

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

### システムノード (System Nodes)

#### Preload Transition Presets

![Preload Transition Presets ノード](images/api_preload_node.png)

指定されたプリセットのシェーダーを事前コンパイルし、初回再生時のヒッチングを防ぎます。`TransitionManagerSubsystem` で利用可能です。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Presets** | Input | プリロードするトランジションプリセットの配列。 |

#### Async Load Transition Presets
ソフトオブジェクト参照からプリセットを非同期にロードし、シェーダーのウォームアップを行います。`TransitionManagerSubsystem` で利用可能です。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Soft Presets** | Input | トランジションプリセットのソフトオブジェクト参照の配列。 |
| **On Complete** | Output | ロードとシェーダーウォームアップ完了時に発火するデリゲート。 |

### シーケンスノード (Sequence Nodes)

`UTransitionSequence` データアセットを使って複数のトランジションを連続再生します。シーケンスにレベル遷移は含められません。また同時に再生できるシーケンスは 1 つだけで、シーケンス再生中に `StartTransition` や `OpenLevelWithTransition` を呼び出すとシーケンスはキャンセルされます。ステップ間ではシーンを見せずにエフェクトが差し替えられます。シーケンス終了時、最後のステップのエフェクトは通常のルールに従い（[エフェクトが消えるタイミング](#エフェクトが消えるタイミング)を参照）、最後のステップが覆った状態で終わる場合は画面を覆ったまま残ります。

#### Play Sequence And Wait
トランジションシーケンスを再生し、（ループを含む）全体の完了を待つレイテントアクションです。シーケンスが null または空の場合は警告を出して即座に完了します。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **World Context Object** | Input | ワールドコンテキストオブジェクト。 |
| **Sequence** | Input | 再生するトランジションシーケンス（`UTransitionSequence`）。 |
| **Completed** | Output | シーケンス全体（全ループ含む）の完了後に実行されます。 |

#### Play Sequence
完了を待たずにシーケンスの再生を開始します。`TransitionManagerSubsystem` で利用可能です。null または空のシーケンスは警告ログを出して何もしません。再生中のシーケンスやトランジションがあれば先に停止します。レベル遷移が保留中の場合は警告を出して無視されます。

| ピン名 | タイプ | 説明 |
| :--- | :--- | :--- |
| **Sequence** | Input | 再生するトランジションシーケンス（`UTransitionSequence`）。 |

#### Stop Sequence
再生中のシーケンス（存在する場合）とその下で動作しているトランジションを停止します。`OnSequenceCompleted` は**発火しません**（キャンセルは正常完了ではないため）。

#### Is Sequence Playing
シーケンスが再生中であれば true を返します。

#### Get Current Sequence Step
現在再生中のエントリのインデックスを返します。シーケンスが再生されていない場合は `-1` を返します。

## 3. C++ API (C++ API Reference)

C++からは `UTransitionManagerSubsystem` を通じて遷移を制御します。

### サブシステムの取得

```cpp
UTransitionManagerSubsystem* TransitionSystem = GetGameInstance()->GetSubsystem<UTransitionManagerSubsystem>();
```

### 主要関数

#### StartTransition
遷移を開始します。`Preset` が null、またはマテリアルかエフェクトクラスが未設定の場合、`ForceClear` が呼ばれエラーがログ出力されます。`PlaySpeed` は最小 0.01 にクランプされます。見える状態で終わる遷移（Reverse のフェードインなど）は完了時に自動的に停止してクリーンアップされ、覆った状態で終わる遷移（Forward のフェードアウトなど）は次の遷移、`ReverseTransition` / `InvertTransition`、または `StopTransition` までエフェクトを画面に残します（[エフェクトが消えるタイミング](#エフェクトが消えるタイミング)を参照）。

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
現在の遷移を停止します。完了した遷移が画面を覆ったまま残っている場合は、そのエフェクトを取り除きます。入力を元に戻し、`OnTransitionCompleted` はブロードキャストしません。レベル遷移のフェードアウトを完了前に停止すると、そのレベル遷移はキャンセルされます。

```cpp
void StopTransition();
```

#### ReleaseHold
最大進行度でのホールドを解除し、ホールド中の遷移を完了させます。`bHoldAtMax = true` を指定した `StartTransition` の後に使用します。解除されたフェードアウトは完了後も画面を覆い続けます。

```cpp
void ReleaseHold();
```

#### ReverseTransition
現在の遷移の再生方向を反転します（例：フェードアウトからフェードインへ）。完了したフェードアウトが画面を覆ったまま残っている状態でも使えます。反転する遷移が残っていない場合は警告を出して何もしません。

```cpp
void ReverseTransition(bool bAutoStop = true);
```

#### InvertTransition
現在の遷移のマスク反転状態をトグルし、最初から（0→1）再生し直します。完了したフェードアウトが画面を覆ったまま残っている状態でも使えます。`bAutoComplete` が false の場合は最大進行度でホールドします（`ReleaseHold` で解除）。

```cpp
void InvertTransition(bool bAutoComplete = true);
```

#### SetPlaySpeed
再生速度の乗数を動的に変更します。値は最小 0.01 にクランプされます。

```cpp
void SetPlaySpeed(float PlaySpeed = 1.0f);
```

#### GetCurrentProgress
現在の遷移の進行度（0.0〜1.0）をイージング適用前の値で返します。

```cpp
float GetCurrentProgress() const;
```

#### IsTransitionPlaying
遷移が再生中またはホールド中の場合に true を返します。完了後は、完了したフェードアウトが画面を覆ったまま残っていても false を返します。

```cpp
bool IsTransitionPlaying() const;
```

#### IsCurrentTransitionFinished
現在の遷移が（ホールドがあれば解除された後に）完了した場合に true を返します。ポーリングに便利です。

```cpp
bool IsCurrentTransitionFinished() const;
```

#### ForceClear
アクティブな遷移（再生中のシーケンスや、画面を覆ったまま残っているエフェクトを含む）を強制的にクリアし、入力をリセットします。保留中のレベル遷移と、次のレベルロード時に予定された自動フェードイン（`PrepareAutoReverseTransition` で準備したものを含む）もキャンセルします。緊急復旧に使用します。

```cpp
void ForceClear();
```

#### OpenLevelWithTransition
「フェードアウト → レベルオープン → フェードイン」のシーケンスを処理します。レベル遷移をまたいで状態が維持されます。サブシステムの Blueprint ノードとしても呼び出せます。`Preset` が null の場合は `GetDefaultFadePreset()` を使用し、再生可能なプリセットがない場合は遷移なしでレベルを開いてエラーをログ出力します。フェードアウトを完了前に停止（`StopTransition` / `ForceClear`）または置き換え（`StartTransition`）た場合はレベル遷移がキャンセルされ、レベルは開かれず、次のレベルロード時のフェードインも再生されません。

```cpp
void OpenLevelWithTransition(const UObject* WorldContextObject, FName LevelName, UTransitionPreset* Preset, float Duration = 1.0f);
```

#### PrepareAutoReverseTransition
次のレベルロード時に自動リバース遷移を行うようサブシステムを準備します。即座に遷移を開始することはありません。サブシステムの Blueprint ノードとしても呼び出せます。

```cpp
void PrepareAutoReverseTransition(UTransitionPreset* Preset, float Duration = 1.0f);
```

#### AddProgressThreshold
進捗の閾値（0.0〜1.0 にクランプ）を登録します。イージング適用済みの進捗がこの値を超えると、`OnProgressThresholdReached` が1回だけブロードキャストされます。登録した閾値は新しい遷移ごとに再び有効になり、`ClearProgressThresholds` を呼ぶまで登録されたままです。

```cpp
void AddProgressThreshold(float Threshold);
```

#### ClearProgressThresholds
登録されているすべての進捗閾値を削除します。

```cpp
void ClearProgressThresholds();
```

#### シーケンス再生 (Sequence Playback)
`UTransitionSequence` データアセットのエントリを連続再生します。`PlaySequence` はシーケンスを検証し（null または空の場合は警告のみで何もしません）、再生中のトランジションやシーケンスがあれば先に停止します。レベル遷移が保留中の場合は無視されます。プリセットが null のエントリや、マテリアルかエフェクトクラスが未設定のエントリは警告を出してスキップされます。`StopSequence` は `OnSequenceCompleted` を発火せずに再生をキャンセルします。

```cpp
void PlaySequence(UTransitionSequence* Sequence);
void StopSequence();
bool IsSequencePlaying() const;
int32 GetCurrentSequenceStep() const; // 再生中でない場合は -1
```

#### GetDefaultFadePreset
デフォルトの `DA_FadeToBlack` プリセットを返します。必要に応じて遅延ロードされます。

```cpp
UTransitionPreset* GetDefaultFadePreset();
```

#### GetTransitionManager（静的ヘルパー）
指定されたワールドコンテキストから `UTransitionManagerSubsystem` を取得します。失敗した場合は `nullptr` を返します。`UTransitionBlueprintLibrary` で利用可能です。

```cpp
static UTransitionManagerSubsystem* GetTransitionManager(const UObject* WorldContextObject);
```

### デリゲート (Delegates / Event Dispatchers)

サブシステムには以下のイベントディスパッチャーが用意されています。

*   **OnTransitionStarted**: 遷移が開始されたときに呼び出されます。
*   **OnTransitionCompleted**: 遷移が完了したときに呼び出されます。
*   **OnTransitionHoldStarted**: 遷移が最大進行度（1.0）でホールド（一時停止）されたときに呼び出されます（`bHoldAtMax` が true の場合）。
*   **OnTransitionProgressChanged** `(float Progress)`: 遷移がアクティブな間、毎ティックでイージング適用済みの進捗値（0.0〜1.0）をブロードキャストします。`GetCurrentProgress()` のポーリングが不要になります。
*   **OnProgressThresholdReached** `(float Threshold)`: `AddProgressThreshold` で登録した閾値をイージング適用済みの進捗が超えた際に1回だけ発火します。閾値は新しい遷移開始時に自動的に再び有効になります。
*   **OnSequenceStepChanged** `(int32 StepIndex)`: シーケンスが新しいエントリに進むたび（最初のエントリを含む）にブロードキャストされます。
*   **OnSequenceCompleted**: シーケンス全体（ループがある場合は全ループ）の完了時にブロードキャストされます。`StopSequence` によるキャンセル時は発火しません。

※ `OnTransitionStop` という名前のデリゲートは存在しませんが、`StopTransition` を呼び出すと遷移は中断されます。

## 4. 高度な機能 (Advanced Features)

### 非同期ロード (Async Loading)
`AsyncLoadTransitionPresets` を使用すると、ソフトオブジェクト参照 (`TSoftObjectPtr`) のリストを使用してプリセットを非同期にロードし、シェーダーのプリロードを行うことができます。

```cpp
void AsyncLoadTransitionPresets(const TArray<TSoftObjectPtr<UTransitionPreset>>& SoftPresets, FTransitionPreloadCompleteDelegate OnComplete);
```

### オブジェクトプーリング (Object Pooling)
サブシステムは遷移エフェクトのインスタンスを再利用のためにプールし、メモリの肥大化を防ぐためにエフェクトクラスごとに上限が設けられています（デフォルト: **3 インスタンス**）。上限は **プロジェクト設定 > プラグイン > TransitionFX > `MaxPoolSizePerEffectClass`** で変更でき、0 に設定するとプーリングを無効化できます。制限を超えたインスタンスはガベージコレクションに委ねられます。

### プリロード (Preloading)
`PreloadTransitionPresets` は、ロード済みのプリセットを使用してシェーダーを事前にウォームアップし、遷移実行時のヒッチ（カクつき）を防ぎます。

```cpp
void PreloadTransitionPresets(const TArray<UTransitionPreset*>& Presets);
```

## 5. パラメータのオーバーライド (Parameter Overrides)

`FTransitionParameters` 構造体を使用することで、プリセットのマテリアルパラメータを実行時に動的に変更できます。

```cpp
FTransitionParameters Params;
Params.ScalarParams.Add(FName("SplitCount"), 12.0f);          // 例: Stripe Cascade のパラメータ
Params.VectorParams.Add(FName("FadeColor"), FLinearColor::Red);
// TextureParams も同様に設定可能（例: Texture Mask の "MaskTexture"）
```

この構造体は `StartTransition` や各種ブループリントノードに渡すことができます。

*   パラメータ名はマテリアルと一致している必要があります。マテリアルに存在しないパラメータのオーバーライドは無視され、警告がログ出力されます（`LogTransitionFX`）。
*   オーバーライドを渡さずにプリセットごとの既定色を指定するには、プリセットの `bOverrideTransitionColor` を有効にして `TransitionColor` を設定します。`FTransitionParameters` で渡した `FadeColor` がそれより優先されます。

## 6. 手動制御とロード画面のワークフロー (Manual Control & Loading Screen Workflow)

TransitionFXは、遷移を自動再生するだけでなく、手動でタイミングを制御する機能を提供しています。特に、レベルロード中や重い処理の間に画面を隠す「ロード画面」として使用する場合に便利です。

### 重要なパラメータと関数

*   **bHoldAtMax (StartTransition)**:
    このパラメータを `true` に設定すると、遷移エフェクトは進行度が **1.0 (画面が完全に覆われた状態)** に達した時点で自動的に一時停止（ホールド）します。この状態では入力はブロックされたままになります。

*   **ReleaseHold**:
    ホールド状態にある遷移を完了させるための関数です。ホールド中のフェードアウトが完了し（`OnTransitionCompleted` が発火）、エフェクトは画面を覆ったまま残ります。これだけではフェードインしません。

*   **ReverseTransition / InvertTransition**:
    ホールド中（または完了したフェードアウト）からフェードインします。`ReverseTransition` は同じエフェクトを逆再生（1 → 0）し、`InvertTransition` はマスクを反転して順方向に再生し直します。どちらもホールド中に直接呼べるので、先に `ReleaseHold` を呼ぶ必要はありません。

### 実装ワークフロー例

1.  **遷移を開始 (Start Transition)**
    *   `StartTransition` (またはブループリントノード) を呼び出し、`bHoldAtMax` を `true` に設定します。
    *   画面がエフェクトで覆われ、進行度が 1.0 で停止します（`OnTransitionHoldStarted` が発火）。

2.  **バックグラウンド処理 (Load Level / Spawn Actors)**
    *   画面が隠れている間に、レベルのロードやアクターのスポーンなど、プレイヤーに見せたくない処理を行います。

3.  **フェードイン (Fade Back In)**
    *   ロード処理が完了したら、`ReverseTransition`（または `InvertTransition`）を呼び出します。`ReleaseHold` を呼んでから同じフレーム内で Reverse のトランジション（`Mode: Reverse` の `Play Transition And Wait` など）を開始する方法でも構いません。

4.  **遷移完了 (Transition Finishes)**
    *   フェードインが完了するとエフェクトが取り除かれ、入力が元に戻ります。

## 7. デバッグとユーティリティ (Debugging & Utilities)

開発中やデバッグ時に役立つ機能を提供しています。

### 強制クリア (Force Clear)

*   **ブループリントノード**: `ForceClear`
    *   すべての遷移状態（保留中のレベル遷移を含む）を即座に破棄し、入力ブロックを解除します。遷移が予期せずスタックした場合の復旧に使用します。

*   **コンソールコマンド**: `TransitionFX.ForceClear`
    *   **重要**: QAテスターや開発者は、コンソールウィンドウ（`@` または `~` キー）でこのコマンドを入力することで、画面が真っ暗なままスタックする状態から即座に復旧できます。
