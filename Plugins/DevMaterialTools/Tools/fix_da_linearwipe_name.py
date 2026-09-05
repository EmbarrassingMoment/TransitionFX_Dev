"""
DA_LinearWipe.uasset の中のオブジェクト名 "DA_LinerWipe" (typo) を "DA_LinearWipe" に直す。

パッケージ名は既に DA_LinearWipe なので、同名パッケージへ直接 rename はできない。
  1. /TransitionFX/Data/DA_LinearWipe.DA_LinerWipe → /TransitionFX/Data/DA_LinearWipe_tmp へ rename
  2. 旧パッケージ DA_LinearWipe に残るリダイレクタを削除
  3. DA_LinearWipe_tmp → /TransitionFX/Data/DA_LinearWipe へ rename (パッケージ名 = オブジェクト名 = DA_LinearWipe)
  4. tmp に残るリダイレクタを削除、保存、検証

結果は Saved/DevMaterialTools/fix_da_linearwipe_name.result.json。エディタは閉じておくこと。
"""
import json
import os

import unreal

DATA_DIR = "/TransitionFX/Data"
OLD_OBJ = f"{DATA_DIR}/DA_LinearWipe.DA_LinerWipe"
FINAL = f"{DATA_DIR}/DA_LinearWipe"
TMP = f"{DATA_DIR}/DA_LinearWipe_tmp"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "fix_da_linearwipe_name.result.json")

lib = unreal.EditorAssetLibrary
log = {"checks": {}, "steps": []}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def delete_if_exists(path):
    if lib.does_asset_exist(path):
        log["steps"].append(f"delete {path}: {lib.delete_asset(path)}")


def run():
    old = lib.load_asset(OLD_OBJ)
    if old is None and lib.does_asset_exist(FINAL):
        obj = lib.load_asset(FINAL)
        if obj is not None and obj.get_name() == "DA_LinearWipe":
            log["note"] = "already fixed"
            check("object name is DA_LinearWipe", True)
            return
    if not check("typo object found", old is not None):
        return
    log["before"] = {"name": old.get_name(), "path": old.get_path_name(),
                     "material": str(old.get_editor_property("transition_material"))}

    check("rename to tmp", lib.rename_asset(OLD_OBJ, TMP))
    delete_if_exists(FINAL)  # 旧パッケージのリダイレクタ
    check("rename tmp to final", lib.rename_asset(TMP, FINAL))
    delete_if_exists(TMP)  # tmp のリダイレクタ

    obj = lib.load_asset(FINAL)
    if not check("final loads", obj is not None):
        return
    check("object name is DA_LinearWipe", obj.get_name() == "DA_LinearWipe")
    check("package is DA_LinearWipe", obj.get_outermost().get_name() == FINAL)
    log["after"] = {"name": obj.get_name(), "path": obj.get_path_name(),
                    "material": str(obj.get_editor_property("transition_material")),
                    "effect_class": str(obj.get_editor_property("effect_class")),
                    "default_duration": obj.get_editor_property("default_duration")}
    check("saved", lib.save_asset(FINAL))
    check("no tmp left", not lib.does_asset_exist(TMP))


try:
    run()
except Exception as ex:  # noqa: BLE001
    log["exception"] = repr(ex)

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
