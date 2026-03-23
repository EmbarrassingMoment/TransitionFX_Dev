# TransitionFX Quick Start Guide

---

## 目次

1. [プリセットの作成](#step-1-プリセットの作成)
2. [初期化](#step-2-初期化)
3. [BPからの呼び出し方](#step-3-bpからの呼び出し方)
4. [ロード画面への応用](#step-4-ロード画面への応用)
5. [デバッグ方法](#step-5-デバッグ方法)
6. [イベント（デリゲート）の活用](#step-6-イベントデリゲートの活用)

---

## Step 1: プリセットの作成

TransitionFX では、遷移エフェクトの設定を **DataAsset（Transition Preset）** として管理します。
まず最初にこのアセットを作成します。

### 作成手順

1. コンテンツブラウザで右クリック → `Miscellaneous` → `Data Asset` を選択
2. クラス一覧から `TransitionPreset` を選択
3. アセット名を設定（例: `DA_FadeToBlack`、`DA_Iris` など）

<!-- IMAGE: quickstart_create_data_asset.png - Content Browser で Data Asset を作成する手順のスクリーンショット -->

### 設定項目

**基本設定（必須）**

| プロパティ | 説明 | 推奨値 |
| :--- | :--- | :--- |
| `Effect Class` | 使用するエフェクトのクラス | `PostProcessTransitionEffect` |
| `Transition Material` | 遷移に使用するマテリアル | `M_Transition_Master`、`M_Transition_Iris`、`M_Transition_Diamond` など |
| `Default Duration` | 遷移の長さ（秒） | `1.0` |

**動作設定**

| プロパティ | 説明 | 推奨値 |
| :--- | :--- | :--- |
| `Easing Type` | 進行度に適用するイージング関数 | `Linear`（シンプルなフェードなら十分） |
| `Progress Curve` | カスタムイージング用の FloatCurve（`Custom` 選択時のみ表示） | 任意 |
| `bAutoBlockInput` | 遷移中にプレイヤー入力を自動でブロックする | `True` 推奨 |
| `bTickWhenPaused` | ポーズ中でも遷移を動作させる | ポーズ画面を使う場合は `True` |
| `Priority` | PostProcess の描画優先度 | `1000`（デフォルト） |

**オーディオ設定（任意）**

| プロパティ | 説明 |
| :--- | :--- |
| `Transition Sound` | 遷移開始時に再生する Sound アセット |
| `Sound Volume` | 音量（デフォルト: `1.0`） |
| `Sound Pitch` | ピッチ（デフォルト: `1.0`） |

![TransitionPreset の詳細パネルのスクリーンショット](images/quickstart_preset_settings.png)

### 設定例：シンプルなフェードアウト

```
DA_FadeToBlack
├─ Effect Class        : PostProcessTransitionEffect
├─ Transition Material : M_Transition_Master
├─ Default Duration    : 1.0
├─ Easing Type         : EaseInOutSine
├─ bAutoBlockInput     : True
└─ bTickWhenPaused     : False
```

> **💡 Tip**
> マテリアルには `Progress` という名前のスカラーパラメータが必須です。
> このパラメータが存在しない場合、遷移はアニメーションしません（警告ログが出力されます）。

---

## Step 2: 初期化

`UTransitionManagerSubsystem` は **GameInstance サブシステム**として動作するため、エンジン起動時に自動で初期化されます。コンソールコマンドの登録やデフォルトアセットのロードはすべて自動で行われるため、特別なセットアップは不要です。

唯一、**手動で行うべき初期化はシェーダーのプリロード**です。

### なぜプリロードが必要か

Unreal Engine はシェーダーを**初回使用時にコンパイル**します。何もしないと、ゲーム中に初めて遷移が再生されたタイミングで一瞬カクつく（ヒッチ）が発生することがあります。`PreloadTransitionPresets` を呼ぶことで、このコンパイルをゲーム起動時に済ませておけます。

### 実装方法

**Blueprint**

`GameInstance` の `Init` イベント、または最初のレベルの `BeginPlay` でノードを呼び出します。

![GameInstance Init での PreloadTransitionPresets ノードの Blueprint スクリーンショット](images/quickstart_preload_bp.png)

```
Event Init (GameInstance)
└─ Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Preload Transition Presets
        └─ Presets: [DA_FadeToBlack, DA_Iris, ...]
```

**C++**

```cpp
void UMyGameInstance::Init()
{
    Super::Init();

    UTransitionManagerSubsystem* TransitionSystem = GetSubsystem<UTransitionManagerSubsystem>();
    if (TransitionSystem)
    {
        TArray<UTransitionPreset*> Presets = { FadePreset, IrisPreset };
        TransitionSystem->PreloadTransitionPresets(Presets);
    }
}
```

### メモリを節約したい場合：非同期プリロード

プリセットをソフト参照で管理している場合や、ローディング画面中にバックグラウンドでシェーダーを準備したい場合は `AsyncLoadTransitionPresets` を使用します。

```
ローディング画面 BeginPlay
└─ Async Load Transition Presets
    ├─ Soft Presets: [DA_FadeToBlack (Soft), DA_Iris (Soft), ...]
    └─ On Complete → ゲーム本編へ遷移（Open Level など）
```

```cpp
TArray<TSoftObjectPtr<UTransitionPreset>> SoftPresets = { SoftFadePreset, SoftIrisPreset };

TransitionSystem->AsyncLoadTransitionPresets(SoftPresets,
    FTransitionPreloadCompleteDelegate::CreateLambda([]()
    {
        // シェーダー準備完了 → ゲーム本編へ
    })
);
```

### 自動で行われること（対応不要）

| 処理 | タイミング |
| :--- | :--- |
| `TransitionFX.ForceClear` コンソールコマンドの登録 | サブシステム初期化時 |
| デフォルトフェードプリセット `DA_FadeToBlack` のロード | サブシステム初期化時 |
| デフォルトマスターマテリアル `M_Transition_Master` のロード | サブシステム初期化時 |
| レベル遷移後の自動フェードイン準備 | `PostLoadMapWithWorld` に自動バインド |

> **💡 Tip**
> プリロードに渡すプリセットは「ゲーム序盤に必ず使う」ものに絞るのがおすすめです。
> すべてのプリセットを渡すと起動時の負荷が増えます。メモリが気になる場合は非同期プリロードを使い、必要なタイミングでロードする設計にしましょう。

---

## Step 3: BPからの呼び出し方

基本的な使い方は **`Play Transition And Wait`** ノードを呼ぶだけです。
フェードアウト・フェードインはどちらも同じプリセットを使い、`Mode` ピンで切り替えます。

### 基本パターン：フェードアウト → 処理 → フェードイン

![フェードアウト → 処理 → フェードインの Blueprint グラフ全体](images/quickstart_fadeout_fadein_bp.png)

最もよく使う構成です。画面を暗転させてから処理を行い、フェードインで戻します。

```
[任意のイベント]
    │
    ▼
Play Transition And Wait
├─ Preset    : DA_FadeToBlack
├─ Mode      : Forward（フェードアウト）
├─ Play Speed: 1.0
└─ bInvert   : False
    │
    ▼（Completed）
【任意の処理：アクターのスポーン、UI切り替えなど】
    │
    ▼
Play Transition And Wait
├─ Preset    : DA_FadeToBlack
├─ Mode      : Reverse（フェードイン）
├─ Play Speed: 1.0
└─ bInvert   : False
    │
    ▼（Completed）
【完了後の処理】
```

### 秒数で再生時間を指定したい場合

`Play Speed` ではなく具体的な秒数で指定したい場合は **`Play Transition And Wait With Duration`** を使います。

```
Play Transition And Wait With Duration
├─ Preset   : DA_FadeToBlack
├─ Mode     : Forward
└─ Duration : 2.0
```

### ランダムなエフェクトを使いたい場合

複数のプリセットからランダムに再生したい場合は **`Play Random Transition And Wait`** を使います。

```
Play Random Transition And Wait
├─ Presets   : [DA_Iris, DA_Diamond, DA_Spiral]
├─ Mode      : Forward
└─ Play Speed: 1.0
```

### レベル遷移をシームレスに行いたい場合

![Open Level With Transition And Wait ノード](images/quickstart_open_level_bp.png)

フェードアウト → レベルオープン → フェードインを 1 ノードで完結させたい場合は **`Open Level With Transition And Wait`** を使います。

```
Open Level With Transition And Wait
├─ Level Name: MainLevel
├─ Preset    : DA_FadeToBlack
└─ Duration  : 1.0
    │
    ▼（Completed）
【呼び出し元での後処理があれば記述】
```

> **⚠️ 注意**
> `Completed` ピンは**フェードアウトが完了し `OpenLevel` が呼ばれた直後**に発火します。
> 新レベルのロード完了を待つものではありません。新レベル側でのフェードインは `PostLoadMapWithWorld` を通じて自動的に開始されます。

### プリセット不要で素早くフェードしたい場合

DataAsset を作らずに手軽にフェードしたい場合は **`Quick Fade To Black`** / **`Quick Fade From Black`** が使えます。

```
Quick Fade To Black   ── Duration: 1.0
Quick Fade From Black ── Duration: 1.0
```

> **💡 Tip**
> `Quick Fade` は内部的にデフォルトの `DA_FadeToBlack` を使用します。
> 黒フェード以外のエフェクトが必要な場合は、専用の DataAsset を作成して `Play Transition And Wait` を使ってください。

### マテリアルパラメータを動的に変えたい場合

`Override Params` ピンに `FTransitionParameters` 構造体を渡すことで、プリセットのマテリアルパラメータを実行時に上書きできます。

```
[Make TransitionParameters]
├─ Scalar Params : { "Intensity" → 2.0 }
└─ Vector Params : { "Color" → LinearColor(Red) }
    │
    ▼
Play Transition And Wait
└─ Override Params: ↑
```

---

## Step 4: ロード画面への応用

レベルのロードやアクターの大量スポーンなど、**プレイヤーに見せたくない重い処理**を画面を隠したまま行いたい場合のワークフローです。`bHoldAtMax` と `Release Hold` を組み合わせることで実現できます。

### 仕組み

通常の遷移は進行度が 1.0 に達すると自動的に完了しますが、`bHoldAtMax = true` にすることで**進行度 1.0（画面が完全に覆われた状態）で一時停止（ホールド）**させることができます。ホールド中は入力もブロックされたままです。

準備が整ったら `Release Hold` を呼ぶことで Forward 遷移が完了状態に移行します。その後、手動で `Play Transition And Wait`（Mode: Reverse）を呼び出すことでフェードインを行います。

### 実装ワークフロー

<!-- IMAGE: quickstart_hold_workflow_bp.png - bHoldAtMax + ReleaseHold を使ったロード画面パターンの Blueprint ワークフロー -->

```
[ロード開始イベント]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Start Transition
├─ Preset     : DA_FadeToBlack
├─ Mode       : Forward
├─ Play Speed : 1.0
└─ bHoldAtMax : True  ← ここがポイント
    │
    ▼（OnTransitionHoldStarted 発火）
【バックグラウンド処理】
├─ レベルのストリーミングロード
├─ アクターのスポーン
└─ データの初期化など
    │
    ▼（処理完了）
Release Hold
    │
    ▼
Play Transition And Wait
├─ Preset : DA_FadeToBlack
└─ Mode   : Reverse  ← フェードインは手動で呼ぶ
    │
    ▼（Completed）
```

### イベントでホールドを検知する

ホールド状態に入ったタイミングは `OnTransitionHoldStarted` デリゲートで取得できます。ポーリングではなくイベントドリブンで実装したい場合に使用します。

```
[BeginPlay などで事前にバインド]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Bind Event to OnTransitionHoldStarted
        └─ [バックグラウンド処理を開始するカスタムイベント]
```

### C++ での実装例

```cpp
// 1. バインドは StartTransition より前に行う
TransitionSystem->OnTransitionHoldStarted.AddDynamic(this, &UMyClass::OnHoldStarted);

// 2. ホールド付きで遷移を開始
TransitionSystem->StartTransition(FadePreset, ETransitionMode::Forward, 1.0f, false, true);

// 3. ホールド後にバックグラウンド処理 → ReleaseHold → フェードイン
void UMyClass::OnHoldStarted()
{
    // バックグラウンド処理...

    TransitionSystem->ReleaseHold();
    TransitionSystem->StartTransition(FadePreset, ETransitionMode::Reverse, 1.0f, false, false);
}
```

> **⚠️ 注意**
> `Release Hold` を呼び忘れると画面が永遠に暗転したままになります。
> 万が一スタックした場合は、コンソール（`@` または `~` キー）で `TransitionFX.ForceClear` を実行することで強制復旧できます。

---

## Step 5: デバッグ方法

### スタックの症状と原因

| 症状 | 主な原因 |
| :--- | :--- |
| 画面が真っ暗なまま何も起きない | `bHoldAtMax = true` で `Release Hold` を呼び忘れた |
| 画面が暗転したまま入力を受け付けない | `bAutoBlockInput = true` の状態で遷移が中断された |
| 遷移が途中で止まって進まない | `Stop Transition` が意図せず呼ばれた、または `Play Speed` が極端に小さい |

### 復旧方法 1：コンソールコマンド（開発・QA 向け）

ゲーム実行中にコンソールウィンドウ（`@` または `~` キー）を開き、以下のコマンドを入力します。

<!-- IMAGE: quickstart_forceclear_console.png - コンソールで TransitionFX.ForceClear コマンドを実行している画面 -->

```
TransitionFX.ForceClear
```

これにより以下がすべてリセットされます。

- 現在の遷移エフェクトの破棄
- 入力ブロック（CinematicMode）の解除
- 再生中のサウンドの停止
- すべての遷移フラグのリセット（`bIsTransitionActive`、`bIsHolding` など）

> **💡 Tip**
> QA テスターには必ずこのコマンドを周知しておきましょう。
> 画面が暗転してスタックした際に自力で復旧できるため、バグレポートの質が上がります。

### 復旧方法 2：`Force Clear` BPノードをデバッグ UI に組み込む

コンソールを使えない環境（コンシューマ向けビルドやコンソール機でのデバッグなど）では、`Force Clear` ノードをデバッグ用の UI・入力に仕込んでおくと便利です。

**特定のキー入力でリセットする**

```
Event [Cheat Key / Debug Input]
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Force Clear
```

**デバッグ用ウィジェットのボタンに割り当てる**

```
[Button: "Force Clear Transition"] OnClicked
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    │
    ▼
Force Clear
```

> **⚠️ 注意**
> `Force Clear` は遷移の完了を待たず即座に状態を破棄します。`OnTransitionCompleted` は発火しません。
> ゲームロジックが `OnTransitionCompleted` の発火を前提としている場合、`Force Clear` 後に手動で後続処理を呼ぶ必要があります。

### `Force Clear` と `Stop Transition` の違い

| | `Stop Transition` | `Force Clear` |
| :--- | :--- | :--- |
| エフェクトの後処理 | 正常にクリーンアップする | 即座に破棄する |
| 入力ブロックの解除 | `bAutoBlockInput` の設定に従う | 強制的に解除する |
| サウンドの停止 | 停止する | 停止する |
| `OnTransitionCompleted` の発火 | しない | しない |
| 主な用途 | 通常の停止処理 | 緊急リセット・デバッグ |

---

## Step 6: イベント（デリゲート）の活用

`TransitionManagerSubsystem` には 3 つのデリゲートが用意されており、遷移のタイミングに合わせてゲームロジックを実行できます。

| デリゲート | 発火タイミング |
| :--- | :--- |
| `OnTransitionStarted` | `StartTransition` が呼ばれ、遷移が開始した直後 |
| `OnTransitionCompleted` | 遷移の進行度が完了値（Forward: `1.0` / Reverse: `0.0`）に達したとき |
| `OnTransitionHoldStarted` | `bHoldAtMax = true` の状態で進行度が `1.0` に達し、ホールドに入ったとき |

### バインド方法

デリゲートは `GameInstance` の `Init` や、レベルの `BeginPlay` など**遷移が起きる前**にバインドしておきます。

**Blueprint**

```
Event BeginPlay（または GameInstance Init）
    │
    ▼
Get Game Instance Subsystem (TransitionManagerSubsystem)
    ├─ Bind Event to OnTransitionStarted     → [カスタムイベント: On Started]
    ├─ Bind Event to OnTransitionCompleted   → [カスタムイベント: On Completed]
    └─ Bind Event to OnTransitionHoldStarted → [カスタムイベント: On Hold]
```

**C++**

```cpp
TransitionSystem->OnTransitionStarted.AddDynamic(this, &UMyClass::OnTransitionStarted);
TransitionSystem->OnTransitionCompleted.AddDynamic(this, &UMyClass::OnTransitionCompleted);
TransitionSystem->OnTransitionHoldStarted.AddDynamic(this, &UMyClass::OnTransitionHoldStarted);
```

### ユースケース例

**OnTransitionStarted：遷移開始と同時に HUD を非表示にする**

遷移アニメーション中に HUD が残って見えてしまうのを防ぐ用途です。

```
OnTransitionStarted
    │
    ▼
Set Visibility (HUD Widget)
└─ Visibility: Hidden
```

**OnTransitionCompleted：フェードアウト完了後にレベルの状態を切り替える**

`Play Transition And Wait`（Mode: Forward）の `Completed` ピンと同等ですが、デリゲートを使うことで呼び出し元と処理を分離できます。

```
OnTransitionCompleted
    │
    ▼
Branch（現在のゲームステートが "FadingOut" か判定）
├─ True  → Spawn New Level Actors / Switch Game State
│               │
│               ▼
│           Play Transition And Wait (Mode: Reverse)
└─ False → 何もしない
```

**OnTransitionHoldStarted：ホールド中にロード進捗 UI を表示する**

`bHoldAtMax = true` で画面が覆われたタイミングでローディングスピナーを出す例です。

```
OnTransitionHoldStarted
    │
    ▼
Set Visibility (Loading Spinner)  ── Visible
    │
    ▼
[非同期ロード処理を開始]
    │
    ▼（ロード完了）
Set Visibility (Loading Spinner)  ── Hidden
    │
    ▼
Release Hold
    │
    ▼
Play Transition And Wait (Mode: Reverse)
```

### 注意点

> **⚠️ `OnTransitionCompleted` は Forward・Reverse どちらの完了でも発火します。**
> 複数の遷移が前後して実行される場合、どちらの完了に反応しているかをゲームステートなどで区別する必要があります。

> **⚠️ バインドしたデリゲートは使い終わったら必ず `Remove` してください。**
> 特にレベル Blueprint でバインドした場合、レベルが再ロードされるたびに重複バインドが発生する可能性があります。`GameInstance` 側でバインドする設計にするか、`Remove Dynamic` で明示的に解除することを推奨します。