# TransitionFX - Quick Start Guide

## Step 1: 初期化（Initialization）

TransitionFX の `UTransitionManagerSubsystem` は **GameInstance サブシステム**として動作するため、エンジン起動時に自動で初期化されます。コンソールコマンドの登録やデフォルトアセットのロードはすべて自動で行われるため、特別なセットアップは不要です。

唯一、**手動で行うべき初期化**は **シェーダーのプリロード**です。

---

### なぜプリロードが必要か？

Unreal Engine はシェーダーを**初回使用時にコンパイル**します。何もしないと、ゲーム中に初めて遷移が再生されたタイミングで一瞬カクつく（ヒッチ）が発生することがあります。`PreloadTransitionPresets` を呼ぶことで、このコンパイルをゲーム起動時に済ませておけます。

---

### 実装方法

#### Blueprint

`GameInstance` の `Init` イベント、または最初のレベルの `BeginPlay` でノードを呼び出してください。

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

プリセットを**ソフト参照**で管理している場合や、ローディング画面中にバックグラウンドでシェーダー準備をしたい場合は `AsyncLoadTransitionPresets` を使用してください。

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

### 補足:自動で行われること

| 処理 | タイミング |
|---|---|
| `TransitionFX.ForceClear` コンソールコマンドの登録 | サブシステム初期化時 |
| デフォルトフェードプリセット (`DA_FadeToBlack`) のロード | サブシステム初期化時 |
| デフォルトマスターマテリアル (`M_Transition_Master`) のロード | サブシステム初期化時 |
| レベル遷移後の自動フェードイン準備 | `PostLoadMapWithWorld` に自動バインド |

---

> **Tips**
> プリロードに渡すプリセットは「ゲーム序盤に必ず使う」ものに絞るのがおすすめです。すべてのプリセットを渡すと起動時の負荷が増えます。メモリが気になる場合は非同期プリロードを使い、必要なタイミングでロードする設計にしましょう。
