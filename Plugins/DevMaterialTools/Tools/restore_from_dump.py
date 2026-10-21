"""
dump_plugin_assets.py の出力 (正解データ) に合わせて、MaterialInstance / TransitionPreset の値と
マテリアルの設定を作り直したアセットへ書き込む (v1.6.1 ホットフィックス: UE 5.5 での作り直し用)。
  - MaterialInstanceConstant: 無ければ作成。親と Scalar/Vector/Texture の上書きを正解データと同じにする
    (正解データに無い上書きは消す)
  - TransitionPreset:         無ければ作成 (オブジェクト名 = パッケージ名)。全プロパティを正解データの値にする
  - TransitionSequence:       エントリ、ループ設定を正解データの値にする (アセットは既にあること)
  - Material:                 ドメイン、ブレンドモード、シェーディングモデル等の設定を正解データの値にする
グラフ (マテリアル / マテリアル関数の中身) はビルダーが作る。ここでは扱わない。

環境変数:
  TFX_REF_DUMP       正解データの JSON (UE 5.8 で dump_plugin_assets.py を実行したもの)
  TFX_RESTORE_ASSETS 対象のパッケージパス (カンマ区切り)。省略時は正解データにある MI / DA / マテリアル全部
結果は Saved/DevMaterialTools/restore_from_dump.result.json に書く。
"""
import json
import os

import unreal

REF = os.environ["TFX_REF_DUMP"]
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "restore_from_dump.result.json")

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = {"assets": {}, "skipped_params": {}}


def to_value(current, ref):
    """Converts a dumped value back to an editor value, using the current value's type as a guide."""
    if ref is None:
        return None
    if isinstance(current, unreal.EnumBase):
        return getattr(type(current), ref)
    if isinstance(ref, list) and len(ref) == 4 and all(isinstance(v, (int, float)) for v in ref):
        return unreal.LinearColor(*ref)
    if isinstance(ref, str) and ref.startswith("/"):
        if ref.startswith("/Script/"):
            return unreal.load_class(None, ref)
        return unreal.load_object(None, ref)
    return ref


def norm(value):
    """Same normalization as dump_plugin_assets.py, for comparing read-back values."""
    if isinstance(value, float):
        return round(value, 5)
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, unreal.LinearColor):
        return [round(value.r, 5), round(value.g, 5), round(value.b, 5), round(value.a, 5)]
    return value


def split(package):
    return package.rsplit("/", 1)[1], package.rsplit("/", 1)[0]


def restore_instance(package, ref, entry):
    mi = lib.load_asset(package) if lib.does_asset_exist(package) else None
    if mi is None:
        name, folder = split(package)
        mi = tools.create_asset(name, folder, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        entry["created"] = True
    mel.set_material_instance_parent(mi, unreal.load_object(None, ref["parent"]))
    mel.clear_all_material_instance_parameters(mi)
    # The setters' return values are unreliable (UE 5.5 returns False even on success), so read the overrides back instead
    for name, value in ref["scalar_parameter_values"].items():
        mel.set_material_instance_scalar_parameter_value(mi, name, value)
    for name, value in ref["vector_parameter_values"].items():
        mel.set_material_instance_vector_parameter_value(mi, name, unreal.LinearColor(*value))
    for name, value in ref["texture_parameter_values"].items():
        mel.set_material_instance_texture_parameter_value(mi, name, unreal.load_object(None, value) if value else None)
    mel.update_material_instance(mi)
    skipped = []
    for arr in ("scalar_parameter_values", "vector_parameter_values", "texture_parameter_values"):
        actual = {str(v.get_editor_property("parameter_info").get_editor_property("name")): norm(v.get_editor_property("parameter_value"))
                  for v in mi.get_editor_property(arr) or []}
        if actual != ref[arr]:
            skipped.append({arr: {"expected": ref[arr], "actual": actual}})
    if skipped:
        log["skipped_params"][package] = skipped
    entry["saved"] = lib.save_asset(package, only_if_is_dirty=False)


def restore_preset(package, ref, entry):
    da = lib.load_asset(package) if lib.does_asset_exist(package) else None
    if da is None:
        name, folder = split(package)
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.TransitionPreset)
        da = tools.create_asset(name, folder, unreal.TransitionPreset, factory)
        entry["created"] = True
    for prop, value in ref["props"].items():
        da.set_editor_property(prop, to_value(da.get_editor_property(prop), value))
    entry["object_name"] = da.get_name()
    entry["saved"] = lib.save_asset(package, only_if_is_dirty=False)


def restore_sequence(package, ref, entry):
    seq = lib.load_asset(package)
    if seq is None:
        entry["error"] = "sequence missing (restore it first)"
        return
    entries = []
    for e in ref["entries"]:
        item = unreal.TransitionSequenceEntry()
        item.set_editor_property("preset", unreal.load_object(None, e["preset"]) if e["preset"] else None)
        item.set_editor_property("mode", getattr(unreal.TransitionMode, e["mode"]))
        for prop in ("duration_override", "invert", "delay_after"):
            item.set_editor_property(prop, e[prop])
        params = unreal.TransitionParameters()
        params.set_editor_property("scalar_params", e["override_params"]["scalar_params"])
        params.set_editor_property("vector_params", {k: unreal.LinearColor(*v) for k, v in e["override_params"]["vector_params"].items()})
        params.set_editor_property("texture_params", {k: unreal.load_object(None, v) for k, v in e["override_params"]["texture_params"].items()})
        item.set_editor_property("override_params", params)
        entries.append(item)
    seq.set_editor_property("entries", entries)
    seq.set_editor_property("loop", ref["loop"])
    seq.set_editor_property("loop_count", ref["loop_count"])
    entry["saved"] = lib.save_asset(package, only_if_is_dirty=False)


def restore_material(package, ref, entry):
    mat = lib.load_asset(package)
    if mat is None:
        entry["error"] = "material missing (build it first)"
        return
    changed = []
    for prop, value in ref["settings"].items():
        if isinstance(value, str) and value.startswith("<error"):
            continue
        current = mat.get_editor_property(prop)
        target = to_value(current, value)
        if current != target:
            mat.set_editor_property(prop, target)
            changed.append(prop)
    entry["changed_settings"] = changed
    if changed:
        mel.recompile_material(mat)
        entry["saved"] = lib.save_asset(package, only_if_is_dirty=False)


def main():
    ref_assets = json.load(open(REF, encoding="utf-8"))["assets"]
    wanted = [p.strip() for p in os.environ.get("TFX_RESTORE_ASSETS", "").split(",") if p.strip()] or sorted(ref_assets)
    for package in wanted:
        ref = ref_assets.get(package)
        entry = log["assets"].setdefault(package, {})
        if ref is None:
            entry["error"] = "not in reference dump"
            continue
        try:
            if ref["class"] == "MaterialInstanceConstant":
                restore_instance(package, ref, entry)
            elif ref["class"] == "TransitionPreset":
                restore_preset(package, ref, entry)
            elif ref["class"] == "TransitionSequence":
                restore_sequence(package, ref, entry)
            elif ref["class"] == "Material":
                restore_material(package, ref, entry)
            else:
                entry["skipped"] = ref["class"]
        except Exception:  # noqa: BLE001
            import traceback
            entry["error"] = traceback.format_exc()


try:
    main()
except Exception:  # noqa: BLE001
    import traceback
    log["exception"] = traceback.format_exc()
failed = [p for p, e in log["assets"].items() if "error" in e or e.get("saved") is False] + sorted(log["skipped_params"])
log["failed"] = failed
log["result"] = "PASS" if not failed and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
