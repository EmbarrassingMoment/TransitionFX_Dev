"""
PostProcess マスター M_Transition_<Effect> から Widget 版を生成する一括変換スクリプト。
  - Materials/Widget/M_Widget_<Effect>            (複製 → 最終段差し替え → UI/Translucent)
  - Materials/Widget/Instances/MI_Widget_<Effect>  (親 = M_Widget_<Effect>、PP 側 MI のパラメータ上書きを写す)
  - Data/DA_Widget_<Preset>                        (DA_<Preset> をミラー、EffectClass=WidgetTransitionEffect)

手順 (2026-09-03 に MCP で Iris に対して手動実行し、動作確認済みのものを移植):
  1. 既存の M_Widget_/MI_Widget_ を削除 (冪等)
  2. M_Transition_<Effect> を複製
  3. 最終段 (SceneTexture → ComponentMask → Lerp → Emissive) を特定
       - Opacity の供給元 = Lerp.Alpha に刺さっていた式 (Iris では MF_ApplyInvert.Result)
       - Emissive の供給元 = VectorParameter "FadeColor"
  4. Lerp / ComponentMask / SceneTexture を削除  ← ドメイン変更より先 (先に変えると一時的にコンパイル失敗ログが出る)
  5. Domain=UI, BlendMode=Translucent
  6. FadeColor → EmissiveColor, Alpha 供給元 → Opacity を接続、再コンパイル、保存
  7. MI を作成し、MI_Transition_<Effect> の Scalar/Vector/Texture 上書きをコピーして保存
  8. DA_Widget_<Preset> を作成 (既存なら TransitionMaterial の付け直しのみ)

想定外のグラフは中断し、結果 JSON の "aborted" に理由を書く。変則マスターは意図的に未対応:
  - SceneTexture/ComponentMask/Lerp/FadeColor が 1 個ずつでない
  - SceneTexture.UVs が接続されている (Pixelate のようにシーンを歪める効果はオーバーレイで再現不可)
  - Lerp の A/B が逆 (Slice)。Alpha の意味が反転するため対象外

print はコマンドレットでは見えないため、結果は Saved/DevMaterialTools/build_widget_batch.result.json に JSON で書く。

実行例 (エディタは閉じておくこと。-script は絶対パス必須):
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/build_widget_material.py" -EnablePlugins=PythonScriptPlugin
対象は環境変数 TFX_EFFECT (カンマ区切りのマテリアル名サフィックス)。未指定なら BATCH の第 1 弾 (Iris を除く) を全て処理。
"""
import json
import os

import unreal

# 第 1 弾: (マテリアル名サフィックス, [(ミラー元 DA, 作成する DA), ...])
# マテリアル名と DA 名は綴りが違うものがある (Checkerboard / DA_CheckerBoard) ので明示する。
BATCH = [
    ("Fade", [("DA_Fade", "DA_Widget_Fade"), ("DA_FadeToBlack", "DA_Widget_FadeToBlack")]),
    ("Iris", [("DA_Iris", "DA_Widget_Iris")]),
    # DA_LinearWipe.uasset の中身は "DA_LinerWipe" という綴り違いのオブジェクト名 (既存の typo) なのでオブジェクトパスで指定
    ("LinearWipe", [("DA_LinearWipe.DA_LinerWipe", "DA_Widget_LinearWipe")]),
    ("Dissolve", [("DA_Dissolve", "DA_Widget_Dissolve")]),
    ("RadialWipe", [("DA_RadialWipe", "DA_Widget_RadialWipe")]),
    ("Checkerboard", [("DA_CheckerBoard", "DA_Widget_CheckerBoard")]),
    ("Blinds", [("DA_Blinds", "DA_Widget_Blinds")]),
    ("TextureMask", [("DA_TextureMask", "DA_Widget_TextureMask")]),
]
DEFAULT_EFFECTS = [e for e, _ in BATCH if e != "Iris"]

MAT_DIR = "/TransitionFX/Materials"
MI_SRC_DIR = f"{MAT_DIR}/Instances"
DST_DIR = f"{MAT_DIR}/Widget"
MI_DIR = f"{DST_DIR}/Instances"
DA_DIR = "/TransitionFX/Data"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "build_widget_batch.result.json")

WIDGET_Z_ORDER = 10000
DA_MIRROR_PROPS = [
    "default_duration", "easing_type", "progress_curve", "auto_block_input", "tick_when_paused",
    "priority", "transition_sound", "sound_volume", "sound_pitch",
    "override_transition_color", "transition_color",
]

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()


class Ctx:
    def __init__(self, effect):
        self.log = {"effect": effect, "checks": {}, "steps": []}

    def check(self, label, ok):
        self.log["checks"][label] = bool(ok)
        return bool(ok)

    def step(self, msg):
        self.log["steps"].append(msg)


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
    out_name = str(res[1]) if isinstance(res, tuple) and res[0] else ""
    return src, out_name


def short(obj):
    return obj.get_name() if obj else None


def effect_class_name(da):
    cls = da.get_editor_property("effect_class")
    return cls.get_name() if cls else None


def convert_material(c, effect):
    """M_Transition_<effect> → M_Widget_<effect>。成功時は UMaterial、未対応/失敗時は None。"""
    src = f"{MAT_DIR}/M_Transition_{effect}"
    dst = f"{DST_DIR}/M_Widget_{effect}"
    mi_path = f"{MI_DIR}/MI_Widget_{effect}"
    for path in (mi_path, dst):
        if lib.does_asset_exist(path):
            c.step(f"delete {path}: {lib.delete_asset(path)}")
    lib.make_directory(DST_DIR)
    lib.make_directory(MI_DIR)

    if not c.check("source exists", lib.does_asset_exist(src)):
        return None
    mat = lib.duplicate_asset(src, dst)
    if not c.check("duplicated", mat is not None):
        return None
    exprs = list(mel.get_material_expressions(mat))
    c.log["expr_count_before"] = len(exprs)

    scenes = [e for e in exprs if isinstance(e, unreal.MaterialExpressionSceneTexture)]
    masks = [e for e in exprs if isinstance(e, unreal.MaterialExpressionComponentMask)
             and input_source(mat, e)[0] in scenes]
    fades = [e for e in exprs if isinstance(e, unreal.MaterialExpressionVectorParameter)
             and str(e.get_editor_property("parameter_name")) == "FadeColor"]
    # 最終段の Lerp は EmissiveColor に刺さっているもの (SDF 内部に別の Lerp を持つマスターがあるため数では判定しない)
    emissive_node = mel.get_material_property_input_node(mat, unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lerps = [emissive_node] if isinstance(emissive_node, unreal.MaterialExpressionLinearInterpolate) else []
    c.log["found"] = {"scene": [short(e) for e in scenes], "mask": [short(e) for e in masks],
                      "emissive_node": short(emissive_node), "fade": [short(e) for e in fades]}
    if not c.check("canonical final chain (1 scene, 1 mask, Lerp on Emissive, 1 FadeColor)",
                   len(scenes) == 1 and len(masks) == 1 and len(lerps) == 1 and len(fades) == 1):
        c.log["aborted"] = "unsupported: non-canonical final chain"
        return None
    lerp = lerps[0]
    scene_uv, _ = input_source(mat, scenes[0], "UVs")
    if not c.check("SceneTexture.UVs unconnected (no scene distortion)", scene_uv is None):
        c.log["aborted"] = "unsupported: effect samples the scene with modified UVs (cannot be an overlay)"
        return None
    lerp_a, _ = input_source(mat, lerp, "A")
    if not c.check("Lerp.A is the scene mask (canonical A/B order)", lerp_a is masks[0]):
        c.log["aborted"] = "unsupported: Lerp A/B swapped (alpha semantics inverted)"
        return None
    alpha_src, alpha_out = input_source(mat, lerp, "Alpha")
    if not c.check("Lerp.Alpha connected", alpha_src is not None):
        return None
    c.log["opacity_source"] = {"expression": short(alpha_src), "class": alpha_src.get_class().get_name(), "output": alpha_out}

    for e in (lerp, masks[0], scenes[0]):
        mel.delete_material_expression(mat, e)
    c.step("deleted lerp/mask/scene")

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    c.check("connect Emissive", mel.connect_material_property(fades[0], "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
    c.check("connect Opacity", mel.connect_material_property(alpha_src, alpha_out, unreal.MaterialProperty.MP_OPACITY))
    mel.recompile_material(mat)
    c.check("domain UI", mat.get_editor_property("material_domain") == unreal.MaterialDomain.MD_UI)
    c.check("blend translucent", mat.get_editor_property("blend_mode") == unreal.BlendMode.BLEND_TRANSLUCENT)
    after = len(list(mel.get_material_expressions(mat)))
    c.log["expr_count_after"] = after
    c.check("expression count = before - 3", after == len(exprs) - 3)
    c.check("material saved", lib.save_asset(dst))
    return mat


def copy_instance_overrides(c, src_mi, dst_mi):
    """MI_Transition_<effect> の Scalar/Vector/Texture 上書きを Widget 側 MI に写す。"""
    counts = {"scalar": 0, "vector": 0, "texture": 0}
    for spv in src_mi.get_editor_property("scalar_parameter_values"):
        name = spv.get_editor_property("parameter_info").get_editor_property("name")
        mel.set_material_instance_scalar_parameter_value(dst_mi, name, spv.get_editor_property("parameter_value"))
        counts["scalar"] += 1
    for vpv in src_mi.get_editor_property("vector_parameter_values"):
        name = vpv.get_editor_property("parameter_info").get_editor_property("name")
        mel.set_material_instance_vector_parameter_value(dst_mi, name, vpv.get_editor_property("parameter_value"))
        counts["vector"] += 1
    for tpv in src_mi.get_editor_property("texture_parameter_values"):
        name = tpv.get_editor_property("parameter_info").get_editor_property("name")
        tex = tpv.get_editor_property("parameter_value")
        if tex:
            mel.set_material_instance_texture_parameter_value(dst_mi, name, tex)
            counts["texture"] += 1
    c.log["copied_overrides"] = counts


def create_instance(c, effect, mat):
    mi_name = f"MI_Widget_{effect}"
    mi = tools.create_asset(mi_name, MI_DIR, unreal.MaterialInstanceConstant,
                            unreal.MaterialInstanceConstantFactoryNew())
    if not c.check("MI created", mi is not None):
        return None
    mel.set_material_instance_parent(mi, mat)
    c.check("MI parent", mi.get_editor_property("parent") == mat)
    src_mi_path = f"{MI_SRC_DIR}/MI_Transition_{effect}"
    if lib.does_asset_exist(src_mi_path):
        copy_instance_overrides(c, lib.load_asset(src_mi_path), mi)
        mel.update_material_instance(mi)
    else:
        c.step(f"{src_mi_path} not found; no overrides copied")
    c.check("MI saved", lib.save_asset(f"{MI_DIR}/{mi_name}"))
    return mi


def create_or_update_preset(c, src_name, dst_name, mi):
    src_path = f"{DA_DIR}/{src_name}"
    dst_path = f"{DA_DIR}/{dst_name}"
    if lib.does_asset_exist(dst_path):
        da = lib.load_asset(dst_path)
        da.set_editor_property("transition_material", mi)
        da.set_editor_property("effect_class", unreal.WidgetTransitionEffect)
        da.set_editor_property("widget_z_order", WIDGET_Z_ORDER)
        c.check(f"{dst_name} material = MI", da.get_editor_property("transition_material") == mi)
        c.check(f"{dst_name} effect class", effect_class_name(da) == "WidgetTransitionEffect")
        c.check(f"{dst_name} saved", lib.save_asset(dst_path))
        c.step(f"{dst_name}: existed, re-pointed material")
        return
    if not c.check(f"{src_name} exists", lib.does_asset_exist(src_path)):
        return
    src = lib.load_asset(src_path)
    factory = unreal.DataAssetFactory()
    factory.set_editor_property("data_asset_class", unreal.TransitionPreset)
    da = tools.create_asset(dst_name, DA_DIR, unreal.TransitionPreset, factory)
    if not c.check(f"{dst_name} created", da is not None):
        return
    for prop in DA_MIRROR_PROPS:
        try:
            da.set_editor_property(prop, src.get_editor_property(prop))
        except Exception as ex:  # noqa: BLE001
            c.step(f"{dst_name}: could not mirror {prop}: {ex}")
    da.set_editor_property("effect_class", unreal.WidgetTransitionEffect)
    da.set_editor_property("transition_material", mi)
    da.set_editor_property("widget_z_order", WIDGET_Z_ORDER)
    # クラスオブジェクトの == は Python ラッパー同士で False になることがあるため名前で比較する
    c.check(f"{dst_name} effect class", effect_class_name(da) == "WidgetTransitionEffect")
    c.check(f"{dst_name} saved", lib.save_asset(dst_path))
    c.log.setdefault("presets", {})[dst_name] = {
        "mirrored_from": src_name,
        "default_duration": da.get_editor_property("default_duration"),
        "easing_type": str(da.get_editor_property("easing_type")),
    }


def process(effect, presets):
    c = Ctx(effect)
    try:
        mat = convert_material(c, effect)
        if mat is not None:
            mi = create_instance(c, effect, mat)
            if mi is not None:
                for src_name, dst_name in presets:
                    create_or_update_preset(c, src_name, dst_name, mi)
    except Exception as ex:  # noqa: BLE001
        c.log["exception"] = repr(ex)
    ok = c.log["checks"] and all(c.log["checks"].values()) and "exception" not in c.log and "aborted" not in c.log
    c.log["result"] = "PASS" if ok else ("UNSUPPORTED" if "aborted" in c.log else "FAIL")
    return c.log


def main():
    wanted = [s.strip() for s in os.environ.get("TFX_EFFECT", "").split(",") if s.strip()] or DEFAULT_EFFECTS
    table = {e: p for e, p in BATCH}
    results = {}
    for effect in wanted:
        presets = table.get(effect, [(f"DA_{effect}", f"DA_Widget_{effect}")])
        results[effect] = process(effect, presets)
    summary = {e: r["result"] for e, r in results.items()}
    os.makedirs(OUT_DIR, exist_ok=True)
    with open(OUT_FILE, "w", encoding="utf-8") as f:
        json.dump({"summary": summary, "results": results}, f, ensure_ascii=False, indent=2)
    print(f"[TEST] RESULT: {summary} -> {OUT_FILE}")


main()
