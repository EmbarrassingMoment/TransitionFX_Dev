"""
M_Transition_<Effect> の MI と DA プリセットを作成する (既にあれば in-place で付け直す。冪等)。

  1. Materials/Instances/MI_Transition_<Effect>  (親 = M_Transition_<Effect>、オーバーライド無し)
  2. Data/DA_<Effect>                            (EffectClass / EasingType は DA_Iris の実値をミラー、
                                                  TransitionMaterial = MI、Duration 1.5s)

対象は環境変数 TFX_EFFECTS (カンマ区切りのエフェクト名。例: TFX_EFFECTS=SpiralTiles)。
未指定なら DEFAULT_EFFECTS の全て。

プラグインのコンテンツなので UE 5.5 のワークスペースで実行すること。先にマスターのビルダー
(build_m_rings.py / build_m_spiraltiles.py) を実行しておく。
L_ShowCase への登録はサンプルプロジェクト (UE 5.8) 側で register_widget_presets.py を
TFX_PRESETS=DA_OrbitRings,DA_RippleRings,DA_SpiralTiles のように指定して実行する。

実行例 (UE 5.5 ワークスペース):
  UnrealEditor-Cmd.exe T55W.uproject -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/build_effect_assets.py" -EnablePlugins=PythonScriptPlugin
"""
import os

import unreal

DEFAULT_EFFECTS = ["RippleRings", "OrbitRings", "SpiralTiles"]
EFFECTS = [n.strip() for n in os.environ.get("TFX_EFFECTS", "").split(",") if n.strip()] or DEFAULT_EFFECTS
MAT_DIR = "/TransitionFX/Materials"
MI_DIR = "/TransitionFX/Materials/Instances"
DA_DIR = "/TransitionFX/Data"
REF_DA_PATH = "/TransitionFX/Data/DA_Iris"
DURATION = 1.5

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
checks = []


def check(label, ok):
    print(f"[TEST] {label}: {'OK' if ok else 'NG'}")
    checks.append(bool(ok))
    return ok


ref_da = lib.load_asset(REF_DA_PATH)
assert ref_da, f"{REF_DA_PATH} not found"
ref_effect_class = ref_da.get_editor_property("effect_class")
ref_easing = ref_da.get_editor_property("easing_type")
print(f"[TEST] ref effect_class={ref_effect_class} easing_type={ref_easing}")
check("ref effect_class present", ref_effect_class is not None)

for effect in EFFECTS:
    parent = lib.load_asset(f"{MAT_DIR}/M_Transition_{effect}")
    if not check(f"{effect}: master found", parent is not None):
        continue

    mi_name = f"MI_Transition_{effect}"
    mi_path = f"{MI_DIR}/{mi_name}"
    mi = lib.load_asset(mi_path) if lib.does_asset_exist(mi_path) else tools.create_asset(
        mi_name, MI_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    if not check(f"{effect}: MI present", mi is not None):
        continue
    mel.set_material_instance_parent(mi, parent)
    check(f"{effect}: MI parent", mi.get_editor_property("parent") == parent)
    check(f"{effect}: MI saved", lib.save_asset(mi_path))

    da_name = f"DA_{effect}"
    da_path = f"{DA_DIR}/{da_name}"
    if lib.does_asset_exist(da_path):
        da = lib.load_asset(da_path)
    else:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.TransitionPreset)
        da = tools.create_asset(da_name, DA_DIR, unreal.TransitionPreset, factory)
    if not check(f"{effect}: DA present", da is not None):
        continue
    da.set_editor_property("effect_class", ref_effect_class)
    da.set_editor_property("transition_material", mi)
    da.set_editor_property("default_duration", DURATION)
    da.set_editor_property("easing_type", ref_easing)
    check(f"{effect}: DA material", da.get_editor_property("transition_material") == mi)
    check(f"{effect}: DA saved", lib.save_asset(da_path))

print("[TEST] RESULT: " + ("PASS" if all(checks) else "FAIL"))
