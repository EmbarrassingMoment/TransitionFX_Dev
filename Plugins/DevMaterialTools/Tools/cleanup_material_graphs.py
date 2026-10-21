"""
TransitionFX のマスターマテリアル / マテリアル関数から到達不能ノードを削除し、描画に効かない設定を正規化する。
  - 到達不能ノード: マテリアルはルート入力 (Emissive / Opacity 等)、関数は FunctionOutput から
    入力を辿って届かない式 (Named Reroute Usage は同名の Declaration へジャンプ)。
    delete_all の消し残りや付け替えで孤立した AspectRaito Declaration などが対象。
    関数の FunctionInput / FunctionOutput は呼び出し側のピンが変わるため削除せず報告のみ
  - ShadingModel: Unlit 以外 → Unlit (PostProcess / UI ドメインではエンジンが Unlit 扱いにするため見た目は不変)
  - BlendMode: PostProcess ドメインで Opaque 以外 → Opaque。BlendableOutputAlpha=false ではブレンドステートに
    使われないが、AlphaHoldout は MATERIALBLENDING_TRANSLUCENT 付きでコンパイルされ (シェーダータイプ +2、
    命令数も変わる)、Substrate 有効時は PostProcessMaterialShaders.usf が Emissive を 0 にする (黒画面)
  - TEXTURES の各テクスチャ: never_stream=True (PostProcess / UI マテリアルからの参照ではストリーミング量の
    見積もりが効かず、初回に低解像度ミップで描画されうる)

安全装置: 開始時に全マスターのシェーダー統計 (PS/VS 命令数、サンプラー、テクスチャサンプル、補間子) と
Scalar/Vector/Texture パラメータ名を記録し、変更後に再コンパイルして比較する。BlendMode を変えた
マテリアルは統計の差を許容して記録し (シェーダータイプ数は減少のみ)、パラメータ名だけ一致を求める。
1 つでも想定外の差があるマテリアルは保存しない。
統計を取るには -AllowCommandletRendering が必須 (無いと全項目 0 で比較が素通りになるため、その場合は中断)。

print はコマンドレットでは見えないため、結果は Saved/DevMaterialTools/cleanup_material_graphs.result.json に書く。
環境変数 TFX_DRY_RUN=1 なら比較まで行い保存しない。

実行例 (エディタは閉じておくこと。-script は絶対パス必須):
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/cleanup_material_graphs.py" ^
    -EnablePlugins=PythonScriptPlugin -AllowCommandletRendering
"""
import json
import os

import unreal

MAT_DIR = "/TransitionFX/Materials"
MF_DIR = "/TransitionFX/MaterialFunctions"
TEXTURES = ["/TransitionFX/Textures/T_Circle_SDF"]
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "cleanup_material_graphs.result.json")
DRY_RUN = os.environ.get("TFX_DRY_RUN", "") == "1"

ROOT_PROPS = ("MP_EMISSIVE_COLOR", "MP_OPACITY", "MP_OPACITY_MASK", "MP_BASE_COLOR", "MP_NORMAL", "MP_METALLIC",
              "MP_SPECULAR", "MP_ROUGHNESS", "MP_AMBIENT_OCCLUSION", "MP_WORLD_POSITION_OFFSET",
              "MP_SUBSURFACE_COLOR", "MP_REFRACTION")
STAT_KEYS = ("num_pixel_shader_instructions", "num_vertex_shader_instructions", "num_samplers",
             "num_pixel_texture_samples", "num_vertex_texture_samples", "num_virtual_texture_samples",
             "num_uv_scalars", "num_interpolator_scalars")
IO_CLASSES = ("MaterialExpressionFunctionInput", "MaterialExpressionFunctionOutput")

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
dev = unreal.DevMaterialTools


def list_assets(folder, prefix):
    out = []
    for path in sorted(lib.list_assets(folder, recursive=True, include_folder=False)):
        name = path.split("/")[-1].split(".")[0]
        if name.startswith(prefix):
            out.append(path.split(".")[0])
    return out


def unreachable(exprs, inputs_fn, owner, roots):
    """roots から入力を辿って届かない式。Usage は同名 Declaration へジャンプする (Declaration 名は一意である前提を検証)。"""
    decl_by_name, dup = {}, []
    for e in exprs:
        if isinstance(e, unreal.MaterialExpressionNamedRerouteDeclaration):
            n = str(e.get_editor_property("name"))
            if n in decl_by_name:
                dup.append(n)
            decl_by_name[n] = e
    if dup:
        raise RuntimeError(f"duplicate Named Reroute Declaration names: {dup}")
    seen, stack = set(), [r for r in roots if r]
    while stack:
        cur = stack.pop()
        key = cur.get_name()
        if key in seen:
            continue
        seen.add(key)
        stack.extend(i for i in inputs_fn(owner, cur) if i)
        if isinstance(cur, unreal.MaterialExpressionNamedRerouteUsage):
            d = decl_by_name.get(str(dev.get_named_reroute_usage_display_name(cur)))
            if d is None:
                raise RuntimeError(f"{key}: usage without a matching declaration")
            stack.append(d)
    return [e for e in exprs if e.get_name() not in seen
            and e.get_class().get_name() != "MaterialExpressionComment"]


def describe(e):
    cls = e.get_class().get_name().replace("MaterialExpression", "")
    for prop in ("parameter_name", "name", "input_name"):
        try:
            v = str(e.get_editor_property(prop))
        except Exception:
            continue
        if v and v != "None":
            return f"{cls}({v})"
    return cls


def snapshot(mat):
    st = mel.get_statistics(mat)
    return {
        "stats": {k: st.get_editor_property(k) for k in STAT_KEYS},
        "shader_types": mel.get_num_shader_types(mat),
        "params": {kind: sorted(str(n) for n in getattr(mel, f"get_{kind}_parameter_names")(mat))
                   for kind in ("scalar", "vector", "texture")},
    }


def clean_function(path):
    fn = lib.load_asset(path)
    exprs = list(mel.get_material_function_expressions(fn))
    roots = [e for e in exprs if isinstance(e, unreal.MaterialExpressionFunctionOutput)]
    dead = unreachable(exprs, mel.get_inputs_for_material_function_expression, fn, roots)
    kept = [describe(e) for e in dead if e.get_class().get_name() in IO_CLASSES]
    removed = []
    for e in dead:
        if e.get_class().get_name() not in IO_CLASSES:
            removed.append(describe(e))
            mel.delete_material_expression_in_function(fn, e)
    if removed:
        mel.update_material_function(fn)
    return fn, {"removed": removed, "kept_io": kept}


def clean_material(path):
    mat = lib.load_asset(path)
    log = {"removed": [], "settings": []}
    exprs = list(mel.get_material_expressions(mat))
    roots = []
    for mp in ROOT_PROPS:
        prop = getattr(unreal.MaterialProperty, mp, None)
        node = mel.get_material_property_input_node(mat, prop) if prop is not None else None
        if node:
            roots.append(node)
    for e in unreachable(exprs, mel.get_inputs_for_material_expression, mat, roots):
        log["removed"].append(describe(e))
        mel.delete_material_expression(mat, e)
    if mat.get_editor_property("shading_model") != unreal.MaterialShadingModel.MSM_UNLIT:
        log["settings"].append(f"shading_model {mat.get_editor_property('shading_model')} -> MSM_UNLIT")
        mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    if (mat.get_editor_property("material_domain") == unreal.MaterialDomain.MD_POST_PROCESS
            and mat.get_editor_property("blend_mode") != unreal.BlendMode.BLEND_OPAQUE):
        log["settings"].append(f"blend_mode {mat.get_editor_property('blend_mode')} -> BLEND_OPAQUE")
        log["blend_mode_changed"] = True
        mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    if log["removed"] or log["settings"]:
        mel.recompile_material(mat)
    return mat, log


def main():
    result = {"dry_run": DRY_RUN, "materials": {}, "functions": {}, "textures": {}}
    mat_paths = list_assets(MAT_DIR, "M_")
    mats = {p: lib.load_asset(p) for p in mat_paths}
    before = {p: snapshot(m) for p, m in mats.items()}
    if all(not any(s["stats"].values()) for s in before.values()):
        result["aborted"] = "shader statistics are all zero (run with -AllowCommandletRendering)"
        return result

    changed_fns = []
    for path in list_assets(MF_DIR, "MF_"):
        fn, log = clean_function(path)
        if log["removed"] or log["kept_io"]:
            result["functions"][path] = log
        if log["removed"]:
            changed_fns.append(path)

    changed_mats = []
    for path in mat_paths:
        _, log = clean_material(path)
        if log["removed"] or log["settings"]:
            changed_mats.append(path)
            result["materials"][path] = log

    # 関数の変更は呼び出し側マテリアルにも波及するので、全マスターを比較する
    failed = []
    for path, mat in mats.items():
        after = snapshot(mat)
        b = before[path]
        diffs = []
        blend_changed = result["materials"].get(path, {}).get("blend_mode_changed", False)
        if after["stats"] != b["stats"] and not blend_changed:
            diffs.append({"stats_before": b["stats"], "stats_after": after["stats"]})
        if after["params"] != b["params"]:
            diffs.append({"params_before": b["params"], "params_after": after["params"]})
        if after["shader_types"] != b["shader_types"] and not (blend_changed and after["shader_types"] < b["shader_types"]):
            diffs.append({"shader_types_before": b["shader_types"], "shader_types_after": after["shader_types"]})
        entry = result["materials"].setdefault(path, {}) if (diffs or path in changed_mats) else None
        if entry is not None:
            entry["shader_types"] = [b["shader_types"], after["shader_types"]]
            entry["ps"] = [b["stats"]["num_pixel_shader_instructions"], after["stats"]["num_pixel_shader_instructions"]]
            if blend_changed:
                entry["stats_before"], entry["stats_after"] = b["stats"], after["stats"]
            if diffs:
                entry["diffs"] = diffs
                failed.append(path)

    for path in TEXTURES:
        tex = lib.load_asset(path)
        if not tex.get_editor_property("never_stream"):
            tex.set_editor_property("never_stream", True)
            result["textures"][path] = "never_stream False -> True"

    result["failed"] = failed
    saved = []
    if not DRY_RUN:
        for path in changed_fns:
            if lib.save_asset(path, only_if_is_dirty=False):
                saved.append(path)
        for path in changed_mats:
            if path not in failed and lib.save_asset(path, only_if_is_dirty=False):
                saved.append(path)
        for path in result["textures"]:
            if lib.save_asset(path, only_if_is_dirty=False):
                saved.append(path)
    result["saved"] = saved
    expected_saves = 0 if DRY_RUN else len(changed_fns) + len(changed_mats) - len(failed) + len(result["textures"])
    result["result"] = "PASS" if not failed and len(saved) == expected_saves else "FAIL"
    return result


try:
    RESULT = main()
except Exception as ex:
    import traceback
    RESULT = {"result": "FAIL", "exception": repr(ex), "traceback": traceback.format_exc()}
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(RESULT, f, ensure_ascii=False, indent=2, default=str)
print(f"[TEST] RESULT: {RESULT.get('result')} -> {OUT_FILE}")
