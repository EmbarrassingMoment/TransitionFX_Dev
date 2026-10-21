"""
UE 5.5 で読めない版 (UE 5.8 で保存) を git 履歴の 5.5 保存版に戻したあと、その後に入った名前の修正だけを
UE 5.5 でやり直す (v1.6.1 ホットフィックス用)。
  - MF_CorrectAspectRatio / MF_GetMaxScreenRadius: FunctionOutput "AspectRaito" -> "AspectRatio" (1.4.0, 64fb2b3)
    旧版から戻すので FunctionOutput の GUID は変わらず、マスター側の呼び出しノードの接続は保たれる
  - M_Transition_Pixelate: ScalarParameter "MaxResoluution" -> "MaxResolution" (1.4.0, 9de6586)
DA_LinearWipe の内部オブジェクト名の修正は fix_da_linearwipe_name.py をそのまま使う。

print はコマンドレットで見えないため、結果は Saved/DevMaterialTools/ue55_reapply_renames.result.json に書く。
"""
import json
import os

import unreal

OUTPUT_RENAMES = {
    "/TransitionFX/MaterialFunctions/MF_CorrectAspectRatio": ("AspectRaito", "AspectRatio"),
    "/TransitionFX/MaterialFunctions/MF_GetMaxScreenRadius": ("AspectRaito", "AspectRatio"),
}
PARAM_RENAMES = {
    "/TransitionFX/Materials/M_Transition_Pixelate": ("MaxResoluution", "MaxResolution"),
}
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "ue55_reapply_renames.result.json")

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
dmt = unreal.DevMaterialTools
log = {"checks": {}, "steps": []}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def run():
    for path, (old, new) in OUTPUT_RENAMES.items():
        fn = lib.load_asset(path)
        if not check(f"{path} loads", fn is not None):
            continue
        outputs = [e for e in dmt.get_material_function_expressions(fn) if isinstance(e, unreal.MaterialExpressionFunctionOutput)]
        renamed = 0
        for e in outputs:
            if str(e.get_editor_property("output_name")) == old:
                e.set_editor_property("output_name", new)
                renamed += 1
        names = sorted(str(e.get_editor_property("output_name")) for e in outputs)
        log["steps"].append(f"{path}: renamed {renamed}, outputs now {names}")
        check(f"{path} has {new}", new in names and old not in names)
        mel.update_material_function(fn)
        check(f"{path} saved", lib.save_asset(path, only_if_is_dirty=False))

    for path, (old, new) in PARAM_RENAMES.items():
        mat = lib.load_asset(path)
        if not check(f"{path} loads", mat is not None):
            continue
        renamed = 0
        for e in dmt.get_material_expressions(mat):
            if isinstance(e, unreal.MaterialExpressionScalarParameter) and str(e.get_editor_property("parameter_name")) == old:
                e.set_editor_property("parameter_name", new)
                renamed += 1
        mel.recompile_material(mat)
        names = sorted(str(n) for n in mel.get_scalar_parameter_names(mat))
        log["steps"].append(f"{path}: renamed {renamed}, scalar params now {names}")
        check(f"{path} has {new}", new in names and old not in names)
        check(f"{path} saved", lib.save_asset(path, only_if_is_dirty=False))


try:
    run()
except Exception as ex:  # noqa: BLE001
    import traceback
    log["exception"] = traceback.format_exc()

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
