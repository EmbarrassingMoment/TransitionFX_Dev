"""
PostProcess マスター M_Transition_<Effect> から Widget 版 M_Widget_<Effect> / MI_Widget_<Effect> を生成し、
DA_Widget_<Effect> (存在すれば) の TransitionMaterial を付け直す。

手順 (2026-09-03 に MCP で Iris に対して手動実行し、動作確認済みのものを移植):
  1. 既存の M_Widget_/MI_Widget_ を削除 (冪等)
  2. M_Transition_<Effect> を Materials/Widget/M_Widget_<Effect> へ複製
  3. 最終段 (SceneTexture → ComponentMask → Lerp → Emissive) を特定
       - Opacity の供給元 = Lerp.Alpha に刺さっていた式 (Iris では MF_ApplyInvert.Result)
       - Emissive の供給元 = VectorParameter "FadeColor"
  4. Lerp / ComponentMask / SceneTexture を削除  ← ドメイン変更より先 (先に変えると一時的にコンパイル失敗ログが出る)
  5. Domain=UI, BlendMode=Translucent
  6. FadeColor → EmissiveColor, Alpha 供給元 → Opacity を接続、再コンパイル、保存
  7. MI_Widget_<Effect> (親 = M_Widget_<Effect>、上書き無し) を作成・保存
  8. DA_Widget_<Effect> があれば TransitionMaterial を MI に付け直して保存

想定外のグラフは中断し、結果 JSON の "aborted" に理由を書く。変則マスターは意図的に未対応:
  - SceneTexture/ComponentMask/Lerp/FadeColor が 1 個ずつでない
  - SceneTexture.UVs が接続されている (Pixelate のようにシーンを歪める効果はオーバーレイで再現不可)
  - Lerp の A/B が逆 (Slice)。Alpha の意味が反転するため対象外
print はコマンドレットでは見えないため、結果は OUT_FILE (Saved/DevMaterialTools/) に JSON で書く。

実行例 (エディタは閉じておくこと):
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_widget_material.py" -EnablePlugins=PythonScriptPlugin
効果名は環境変数 TFX_EFFECT で指定 (既定 Iris)。
"""
import json
import os

import unreal

EFFECT = os.environ.get("TFX_EFFECT", "Iris")
SRC = f"/TransitionFX/Materials/M_Transition_{EFFECT}"
DST_DIR = "/TransitionFX/Materials/Widget"
MI_DIR = f"{DST_DIR}/Instances"
DST_NAME = f"M_Widget_{EFFECT}"
MI_NAME = f"MI_Widget_{EFFECT}"
DST = f"{DST_DIR}/{DST_NAME}"
MI = f"{MI_DIR}/{MI_NAME}"
DA = f"/TransitionFX/Data/DA_Widget_{EFFECT}"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, f"build_widget_{EFFECT}.result.json")

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
log = {"effect": EFFECT, "checks": {}, "steps": []}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def step(msg):
    log["steps"].append(msg)


def input_source(mat, expr, input_name=None):
    """入力ピン input_name に刺さっている (expression, output_name) を返す。未接続なら (None, "")。
    FExpressionInput は Python から protected で読めないため MaterialEditingLibrary 経由で辿る。"""
    names = [str(n) for n in mel.get_material_expression_input_names(expr)]
    inputs = list(mel.get_inputs_for_material_expression(mat, expr))
    if input_name is None:
        idx = 0  # 単一入力ノード (ComponentMask 等) は名前が空のことがあるので先頭を使う
    else:
        idx = next((i for i, n in enumerate(names) if n.lower() == input_name.lower()), None)
        if idx is None:
            return None, ""
    src = inputs[idx] if idx < len(inputs) else None
    if src is None:
        return None, ""
    res = mel.get_input_node_output_name_for_material_expression(expr, src)
    # Python では (bool, out_name) のタプルで返る
    out_name = str(res[1]) if isinstance(res, tuple) and res[0] else ""
    return src, out_name


def short(obj):
    return obj.get_name() if obj else None


def run():
    # ---- 1. 既存物の削除 (冪等) ----
    for path in (MI, DST):
        if lib.does_asset_exist(path):
            step(f"delete {path}: {lib.delete_asset(path)}")
    lib.make_directory(DST_DIR)
    lib.make_directory(MI_DIR)

    # ---- 2. 複製 ----
    if not check("source exists", lib.does_asset_exist(SRC)):
        return
    mat = lib.duplicate_asset(SRC, DST)
    if not check("duplicated", mat is not None):
        return
    exprs = list(mel.get_material_expressions(mat))
    log["expr_count_before"] = len(exprs)

    # ---- 3. 最終段の特定 ----
    scenes = [e for e in exprs if isinstance(e, unreal.MaterialExpressionSceneTexture)]
    lerps = [e for e in exprs if isinstance(e, unreal.MaterialExpressionLinearInterpolate)]
    masks = [e for e in exprs if isinstance(e, unreal.MaterialExpressionComponentMask)
             and input_source(mat, e)[0] in scenes]
    fades = [e for e in exprs if isinstance(e, unreal.MaterialExpressionVectorParameter)
             and str(e.get_editor_property("parameter_name")) == "FadeColor"]
    log["found"] = {"scene": [short(e) for e in scenes], "mask": [short(e) for e in masks],
                    "lerp": [short(e) for e in lerps], "fade": [short(e) for e in fades]}
    if not check("canonical final chain (1 scene, 1 mask, 1 lerp, 1 FadeColor)",
                 len(scenes) == 1 and len(masks) == 1 and len(lerps) == 1 and len(fades) == 1):
        log["aborted"] = "unexpected graph; convert this master by hand"
        return
    lerp = lerps[0]
    # 未対応パターンの検出 (変則マスターは意図的に変換しない):
    #  - SceneTexture.UVs が接続されている = シーンを歪める効果 (Pixelate 等)。オーバーレイでは再現不可能
    #  - Lerp.A が scene 側でない (Slice の A/B 逆転) = Alpha の意味が反転するので変換対象外
    scene_uv, _ = input_source(mat, scenes[0], "UVs")
    if not check("SceneTexture.UVs unconnected (no scene distortion)", scene_uv is None):
        log["aborted"] = "unsupported: effect samples the scene with modified UVs (cannot be an overlay)"
        return
    lerp_a, _ = input_source(mat, lerp, "A")
    if not check("Lerp.A is the scene mask (canonical A/B order)", lerp_a is masks[0]):
        log["aborted"] = "unsupported: Lerp A/B swapped (alpha semantics inverted)"
        return
    alpha_src, alpha_out = input_source(mat, lerp, "Alpha")
    if not check("Lerp.Alpha connected", alpha_src is not None):
        return
    log["opacity_source"] = {"expression": short(alpha_src), "class": alpha_src.get_class().get_name(), "output": alpha_out}

    # ---- 4. 旧最終段を削除 (ドメイン変更より先) ----
    for e in (lerp, masks[0], scenes[0]):
        mel.delete_material_expression(mat, e)
    step("deleted lerp/mask/scene")

    # ---- 5. ドメイン / ブレンド ----
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    # ---- 6. 再接続・再コンパイル・保存 ----
    check("connect Emissive", mel.connect_material_property(fades[0], "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
    check("connect Opacity", mel.connect_material_property(alpha_src, alpha_out, unreal.MaterialProperty.MP_OPACITY))
    mel.recompile_material(mat)
    check("domain UI", mat.get_editor_property("material_domain") == unreal.MaterialDomain.MD_UI)
    check("blend translucent", mat.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT)
    after = len(list(mel.get_material_expressions(mat)))
    log["expr_count_after"] = after
    check("expression count = before - 3", after == len(exprs) - 3)
    check("material saved", lib.save_asset(DST))

    # ---- 7. MI ----
    mi = tools.create_asset(MI_NAME, MI_DIR, unreal.MaterialInstanceConstant,
                            unreal.MaterialInstanceConstantFactoryNew())
    if not check("MI created", mi is not None):
        return
    mel.set_material_instance_parent(mi, mat)
    check("MI parent", mi.get_editor_property("parent") == mat)
    check("MI saved", lib.save_asset(MI))

    # ---- 8. DA の付け直し (存在する場合のみ) ----
    if lib.does_asset_exist(DA):
        da = lib.load_asset(DA)
        da.set_editor_property("transition_material", mi)
        check("DA material = MI", da.get_editor_property("transition_material") == mi)
        log["da"] = {
            "effect_class": str(da.get_editor_property("effect_class")),
            "default_duration": da.get_editor_property("default_duration"),
            "widget_z_order": da.get_editor_property("widget_z_order"),
        }
        check("DA saved", lib.save_asset(DA))
    else:
        step(f"{DA} not found; skipped")


try:
    run()
except Exception as ex:  # noqa: BLE001
    log["exception"] = repr(ex)

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log and "aborted" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
