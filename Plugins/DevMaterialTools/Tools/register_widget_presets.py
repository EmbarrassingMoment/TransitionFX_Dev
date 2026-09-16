"""
L_ShowCase の LevelScriptActor メンバ変数 PostProcess (TArray<TSoftObjectPtr<UTransitionPreset>>) に
未登録の DA_Widget_* を全て追加する。build_boxroll_assets.py のステップ 3 を汎用化したもの。

  - 配列が現在アルファベット順 (大文字小文字無視) なら各 DA をソート位置へ挿入、そうでなければ末尾へ
  - 既に登録済みのものはスキップ (冪等)
  - ソフト参照は未ロードだと None になり書き戻しで配列を壊すため、先に /TransitionFX/Data の全 DA をロードし、
    None 混入時は中断する
  - 変数は Instance Editable ではないため Blueprint の CDO に書いて compile_blueprint → save_map で永続化

結果は Saved/DevMaterialTools/register_widget_presets.result.json。
エディタは閉じておくこと (GUI エディタが同プロジェクトを開いていると umap の保存がロックで失敗する)。
"""
import json
import os

import unreal

DA_DIR = "/TransitionFX/Data"
LEVEL_PATH = "/Game/SampleLevel/L_ShowCase"
PRESET_VAR = "PostProcess"
PREFIX = "DA_Widget_"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "register_widget_presets.result.json")

lib = unreal.EditorAssetLibrary
log = {"checks": {}, "steps": []}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def find_lsa(world):
    lsas = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.LevelScriptActor)
    return next((a for a in lsas if "ShowCase" in a.get_class().get_name()), None)


def run():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    all_da_names = []
    for ad in registry.get_assets_by_path(DA_DIR, recursive=False):
        lib.load_asset(str(ad.package_name))
        all_da_names.append(str(ad.asset_name))
    widget_das = sorted(n for n in all_da_names if n.startswith(PREFIX))
    log["widget_presets_on_disk"] = widget_das
    if not check("widget presets found", len(widget_das) > 0):
        return

    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    check("level loaded", les.load_level(LEVEL_PATH))
    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    world = ues.get_editor_world()
    lsa = find_lsa(world)
    if not check("LSA found", lsa is not None):
        return

    presets = list(lsa.get_editor_property(PRESET_VAR))
    names = [p.get_name() if p else "None" for p in presets]
    log["before"] = names
    if not check("no unresolved soft refs", "None" not in names):
        return

    to_add = [n for n in widget_das if n not in names]
    log["to_add"] = to_add
    if not to_add:
        check("registered", True)
        log["note"] = "all widget presets already registered"
        return

    sorted_before = names == sorted(names, key=str.lower)
    for da_name in to_add:
        da = lib.load_asset(f"{DA_DIR}/{da_name}")
        cur_names = [p.get_name() for p in presets]
        if sorted_before:
            idx = len([n for n in cur_names if n.lower() < da_name.lower()])
        else:
            idx = len(presets)
        presets.insert(idx, da)
        log["steps"].append(f"insert {da_name} at {idx}")

    arr = unreal.Array(unreal.TransitionPreset)
    for p in presets:
        arr.append(p)
    bel = unreal.BlueprintEditorLibrary
    res = bel.get_blueprint_for_class(lsa.get_class())
    bp = res[0] if isinstance(res, tuple) else res
    if not check("level BP found", bp is not None):
        return
    bp.modify()
    cdo = unreal.get_default_object(lsa.get_class())
    cdo.set_editor_property(PRESET_VAR, arr)
    bel.compile_blueprint(bp)

    world = ues.get_editor_world()
    lsa2 = find_lsa(world)
    after = [p.get_name() if p else "None" for p in lsa2.get_editor_property(PRESET_VAR)]
    log["after"] = after
    check("registered", all(n in after for n in to_add) and len(after) == len(names) + len(to_add))
    check("level saved", unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL_PATH))


try:
    run()
except Exception as ex:  # noqa: BLE001
    log["exception"] = repr(ex)

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
