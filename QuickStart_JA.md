# TransitionFX - Quick Start Guide

## Step 1: プリセットの作成（Create a Preset）

TransitionFX では、遷移エフェクトの設定を **DataAsset（Transition Preset）** として管理します。まず最初にこのアセットを作成します。

---

### 作成手順

1. コンテンツブラウザで右クリック → `Miscellaneous` → `Data Asset` を選択
2. クラス一覧から `TransitionPreset` を選択
3. アセット名を設定（例: `DA_FadeToBlack`、`DA_Iris` など）

---

### 設定項目

#### 基本設定（必須）

| プロパティ | 説明 | 推奨値 |
|---|---|---|
| **Effect Class** | 使用するエフェクトのクラス | `PostProcessTransitionEffect` |
| **Transition Material** | 遷移に使用するマテリアル | `M_Transition_Master`、`M_Iris`、`M_Diamond` など |
| **Default Duration** | 遷移の長さ（秒） | `1.0` |

#### 動作設定

| プロパティ | 説明 | 推奨値 |
|---|---|---|
| **Easing Type** | 進行度に適用するイージング関数 | `Linear`（シンプルなフェードなら十分） |
| **Progress Curve** | カスタムイージング用のFloatCurve（`Custom` 選択時のみ表示） | 任意 |
| **bAutoBlockInput** | 遷移中にプレイヤー入力を自動でブロックする | `True` 推奨 |
| **bTickWhenPaused** | ポーズ中でも遷移を動作させる | ポーズ画面を使う場合は `True` |
| **Priority** | PostProcess の描画優先度 | `1000`（デフォルト） |

#### オーディオ設定（任意）

| プロパティ | 説明 |
|---|---|
| **Transition Sound** | 遷移開始時に再生するSoundアセット |
| **Sound Volume** | 音量（デフォルト: `1.0`） |
| **Sound Pitch** | ピッチ（デフォルト: `1.0`） |

---

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

> **💡 Tips**
> マテリアルには `Progress` という名前のスカラーパラメータが必須です。このパラメータが存在しない場合、遷移はアニメーションしません（警告ログが出力されます）。

---

## Step 2: 初期化（Initialization）

TransitionFX の `UTransitionManagerSubsystem` は **GameInstance サブシステム**として動作するため、エンジン起動時に自動で初期化されます。コンソールコマンドの登録やデフォルトアセットのロードはすべて自動で行われるため、特別なセットアップは不要です。

唯一、**手動で行うべき初期化**は **シェーダーのプリロード**です。

---

### なぜプリロードが必要か？

Unreal Engine はシェーダーを**初回使用時にコンパイル**します。何もしないと、ゲーム中に初めて遷移が再生されたタイミングで一瞬カクつく（ヒッチ）が発生することがあります。`PreloadTransitionPresets` を呼ぶことで、このコンパイルをゲーム起動時に済ませておけます。

---

### 実装方法

#### Blueprint

`GameInstance` の `Init` イベント、または最初のレベルの `BeginPlay` でノードを呼び出します。

```
Event Init (GameInstance)
└─ Get Game Instance Subsystem (TransitionManagerSubsystem)
    └─ Preload Transition Presets
        └─ Presets: [DA_FadeToBlack, DA_Iris, ...]  ← よく使うプリセットを配列で渡す
```

#### C++

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

---

### メモリを節約したい場合：非同期プリロード

プリセットを**ソフト参照**で管理している場合や、ローディング画面中にバックグラウンドでシェーダー準備をしたい場合は `AsyncLoadTransitionPresets` を使用します。

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

---

### 自動で行われること（対応不要）

| 処理 | タイミング |
|---|---|
| `TransitionFX.ForceClear` コンソールコマンドの登録 | サブシステム初期化時 |
| デフォルトフェードプリセット (`DA_FadeToBlack`) のロード | サブシステム初期化時 |
| デフォルトマスターマテリアル (`M_Transition_Master`) のロード | サブシステム初期化時 |
| レベル遷移後の自動フェードイン準備 | `PostLoadMapWithWorld` に自動バインド |

---

> **💡 Tips**
> プリロードに渡すプリセットは「ゲーム序盤に必ず使う」ものに絞るのがおすすめです。すべてのプリセットを渡すと起動時の負荷が増えます。メモリが気になる場合は非同期プリロードを使い、必要なタイミングでロードする設計がおすすめです。
