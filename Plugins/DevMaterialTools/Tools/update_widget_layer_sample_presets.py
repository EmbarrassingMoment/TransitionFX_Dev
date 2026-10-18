"""
既存の /Game/Widget/WBP_WidgetLayerSample のプリセット一覧だけを最新化する。
build_widget_layer_sample.py は WBP とレベルを削除して作り直すため、ウィジェットレイヤー版を
追加しただけのときはこちらを使う (レイアウト・レベル・デザイナでの変更は触らない)。

  - WidgetPresets      = /TransitionFX/Data の DA_Widget_* 全部 (アルファベット順)
  - PostProcessPresets = 同名の DA_* (無ければ None)。WidgetPresets とインデックス対応
  - CDO に書いて compile_blueprint → save_asset

結果は Saved/DevMaterialTools/update_widget_layer_sample_presets.result.json (print はコマンドレットでは見えない)。

実行 (エディタは閉じておく。-script は絶対パス必須):
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/update_widget_layer_sample_presets.py" -EnablePlugins=PythonScriptPlugin
"""
import json
import os

import unreal

WBP_PATH = "/Game/Widget/WBP_WidgetLayerSample"
DA_DIR = "/TransitionFX/Data"
WIDGET_PREFIX = "DA_Widget_"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "update_widget_layer_sample_presets.result.json")

lib = unreal.EditorAssetLibrary
log = {"checks": {}}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def preset_names(cdo, prop):
    return [p.get_name() if p else "None" for p in cdo.get_editor_property(prop)]


def collect_presets():
    """DA_Widget_* (sorted) and their PostProcess counterparts (loaded objects, None when missing)."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    names = sorted(str(ad.asset_name) for ad in registry.get_assets_by_path(DA_DIR, recursive=False))
    widget_presets, pp_presets, pairs = [], [], []
    for wn in (n for n in names if n.startswith(WIDGET_PREFIX)):
        pp_name = "DA_" + wn[len(WIDGET_PREFIX):]
        w = lib.load_asset(f"{DA_DIR}/{wn}")
        pp = lib.load_asset(f"{DA_DIR}/{pp_name}") if pp_name in names else None
        widget_presets.append(w)
        pp_presets.append(pp)
        pairs.append([wn, pp_name if pp else None])
    log["preset_pairs"] = pairs
    return widget_presets, pp_presets


def run():
    wbp = lib.load_asset(WBP_PATH)
    if not check("WBP loaded", wbp is not None):
        return
    widget_presets, pp_presets = collect_presets()
    if not check("widget presets found", len(widget_presets) > 0 and None not in widget_presets):
        return

    cdo = unreal.get_default_object(wbp.generated_class())
    log["before_widget_presets"] = preset_names(cdo, "widget_presets")

    w_arr = unreal.Array(unreal.TransitionPreset)
    for p in widget_presets:
        w_arr.append(p)
    pp_arr = unreal.Array(unreal.TransitionPreset)
    for p in pp_presets:
        pp_arr.append(p)
    wbp.modify()
    cdo.set_editor_property("widget_presets", w_arr)
    cdo.set_editor_property("post_process_presets", pp_arr)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)

    cdo = unreal.get_default_object(wbp.generated_class())
    saved_w = preset_names(cdo, "widget_presets")
    saved_pp = preset_names(cdo, "post_process_presets")
    log["after_widget_presets"] = saved_w
    log["after_post_process_presets"] = saved_pp
    check("CDO preset lists", saved_w == [p.get_name() for p in widget_presets]
          and len(saved_pp) == len(pp_presets))
    status = wbp.get_editor_property("status")
    log["wbp_status"] = str(status)
    check("WBP compiles", status == unreal.BlueprintStatus.BS_UP_TO_DATE)
    check("WBP saved", lib.save_asset(WBP_PATH, only_if_is_dirty=False))


try:
    run()
except Exception as ex:  # noqa: BLE001
    log["exception"] = repr(ex)

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
