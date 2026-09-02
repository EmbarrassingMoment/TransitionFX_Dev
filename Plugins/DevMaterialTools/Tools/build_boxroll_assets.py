"""
BoxRoll の MI / DA プリセット作成と L_ShowCase 登録 (BoxRoll 付帯アセット)。

  1. MI_Transition_BoxRoll  (親 = M_Transition_BoxRoll、オーバーライド無し)
  2. DA_BoxRoll             (EffectClass / EasingType は DA_StripeCascade の実値をミラー、
                             TransitionMaterial = MI。Duration は箱の転がり全体が見える 3.0s)
  3. L_ShowCase の LevelScriptActor メンバ変数 PostProcess
     (TArray<TSoftObjectPtr<UTransitionPreset>>; BeginPlay で
      AsyncLoadTransitionPresets(SoftPresets=...) に渡される) へ DA_BoxRoll を追加。
     - 配列が現在アルファベット順ならソート位置へ挿入、そうでなければ末尾へ
     - 既に登録済みならスキップ (冪等)
     - ソフト参照は未ロードだと Python 側で None になり書き戻しで配列を壊すため、
       先に /TransitionFX/Data の全 DA をロードし、None 混入時は中断する
     - 変数は Instance Editable ではない (インスタンスへの set は
       "cannot be edited on instances" で失敗) ため、Blueprint の CDO に書いて
       compile_blueprint → レベル保存で永続化する

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_boxroll_assets.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MAT_PATH = "/TransitionFX/Materials/M_Transition_BoxRoll"
MI_DIR = "/TransitionFX/Materials/Instances"
MI_NAME = "MI_Transition_BoxRoll"
DA_DIR = "/TransitionFX/Data"
DA_NAME = "DA_BoxRoll"
REF_DA_PATH = "/TransitionFX/Data/DA_StripeCascade"
LEVEL_PATH = "/Game/SampleLevel/L_ShowCase"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
checks = []

def check(label, ok):
    print(f"[TEST] {label}: {'OK' if ok else 'NG'}")
    checks.append(bool(ok))
    return ok

# ---- 参照値のダンプ (DA_StripeCascade をミラー元にする) ----
ref_da = lib.load_asset(REF_DA_PATH)
assert ref_da, f"{REF_DA_PATH} not found"
ref_effect_class = ref_da.get_editor_property("effect_class")
ref_easing = ref_da.get_editor_property("easing_type")
ref_material = ref_da.get_editor_property("transition_material")
print(f"[TEST] ref effect_class={ref_effect_class}")
print(f"[TEST] ref easing_type={ref_easing} duration={ref_da.get_editor_property('default_duration')}")
print(f"[TEST] ref transition_material={ref_material.get_path_name() if ref_material else None}")
check("ref effect_class present", ref_effect_class is not None)

# ---- 1. MI_Transition_BoxRoll ----
parent_mat = lib.load_asset(MAT_PATH)
assert parent_mat, f"{MAT_PATH} not found"
mi_full = f"{MI_DIR}/{MI_NAME}"
if lib.does_asset_exist(mi_full):
    lib.delete_asset(mi_full)
mi = tools.create_asset(MI_NAME, MI_DIR, unreal.MaterialInstanceConstant,
                        unreal.MaterialInstanceConstantFactoryNew())
mel.set_material_instance_parent(mi, parent_mat)
check("MI created", mi is not None)
check("MI parent", mi.get_editor_property("parent") == parent_mat)
check("MI saved", lib.save_asset(mi_full))

# ---- 2. DA_BoxRoll ----
da_full = f"{DA_DIR}/{DA_NAME}"
if lib.does_asset_exist(da_full):
    lib.delete_asset(da_full)
factory = unreal.DataAssetFactory()
factory.set_editor_property("data_asset_class", unreal.TransitionPreset)
da = tools.create_asset(DA_NAME, DA_DIR, unreal.TransitionPreset, factory)
check("DA created", da is not None)
da.set_editor_property("effect_class", ref_effect_class)
da.set_editor_property("transition_material", mi)
da.set_editor_property("default_duration", 3.0)
da.set_editor_property("easing_type", ref_easing)
check("DA saved", lib.save_asset(da_full))

# ---- 3. L_ShowCase の PostProcess 配列 (プリセットリスト) へ登録 ----
PRESET_VAR = "PostProcess"

# ソフト参照が None 化しないよう先に全 DA をロードしておく
for ad in unreal.AssetRegistryHelpers.get_asset_registry().get_assets_by_path(DA_DIR, recursive=False):
    lib.load_asset(str(ad.package_name))

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
check("level loaded", les.load_level(LEVEL_PATH))

world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
lsas = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LevelScriptActor)
print(f"[TEST] level script actors: {[a.get_class().get_name() for a in lsas]}")
lsa = next((a for a in lsas if "ShowCase" in a.get_class().get_name()), None)
if check("LSA found", lsa is not None):
    presets = list(lsa.get_editor_property(PRESET_VAR))
    names = [p.get_name() if p else "None" for p in presets]
    print(f"[TEST] {PRESET_VAR} before ({len(presets)}): {names}")
    if "None" in names:
        check("no unresolved soft refs (abort: would corrupt array)", False)
    elif DA_NAME in names:
        print("[TEST] already registered; skipping")
        check("registered", True)
    else:
        da_loaded = lib.load_asset(da_full)
        if names == sorted(names, key=str.lower):
            idx = len([n for n in names if n.lower() < DA_NAME.lower()])
        else:
            idx = len(presets)
        presets.insert(idx, da_loaded)
        arr = unreal.Array(unreal.TransitionPreset)
        for p in presets:
            arr.append(p)
        bel = unreal.BlueprintEditorLibrary
        res = bel.get_blueprint_for_class(lsa.get_class())
        bp = res[0] if isinstance(res, tuple) else res
        check("level BP found", bp is not None)
        bp.modify()
        cdo = unreal.get_default_object(lsa.get_class())
        cdo.set_editor_property(PRESET_VAR, arr)
        bel.compile_blueprint(bp)
        # コンパイルで LSA が作り直される可能性があるため取り直して検証
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
        lsas = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LevelScriptActor)
        lsa2 = next((a for a in lsas if "ShowCase" in a.get_class().get_name()), None)
        after = [p.get_name() if p else "None" for p in lsa2.get_editor_property(PRESET_VAR)]
        print(f"[TEST] {PRESET_VAR} after ({len(after)}): {after}")
        check("registered", DA_NAME in after and len(after) == len(names) + 1)
        # save_current_level() はコマンドレットでは False を返す (2026-08-29 確認) ため save_map を使う
        check("level saved", unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL_PATH))

print("[TEST] RESULT: " + ("PASS" if all(checks) else "FAIL"))
