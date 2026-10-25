"""
/TransitionFX のアセット内容を JSON に書き出す (エンジンのバージョンをまたいで比較するため)。
  - Material:          ドメイン等の設定、パラメータ名と既定値、式の数、(-AllowCommandletRendering 時) シェーダー統計
  - MaterialFunction:  入力 (名前・型・順序・既定値)、出力名、説明、式の数
  - MaterialInstance:  親、Scalar/Vector/Texture の上書き値
  - TransitionPreset / TransitionSequence: 全 UPROPERTY の値
  - Texture2D:         主要な設定
オブジェクト参照はパス名、色は丸めた数値にして、メモリアドレスなど実行ごとに変わる値を含めない。
UE 5.5 (作業環境) と UE 5.8 (サンプルプロジェクト) の両方で動く。

print はコマンドレットで見えないため、出力先は環境変数 TFX_DUMP_OUT (JSON パス) で受け取る。
アセットの列挙はアセットレジストリ経由なので、読めないパッケージ (新しいエンジンで保存されたもの) は出てこない。

実行例:
  set TFX_DUMP_OUT=D:/tfx55/dump_58_before.json
  UnrealEditor-Cmd.exe <uproject> -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/dump_plugin_assets.py" -EnablePlugins=PythonScriptPlugin -AllowCommandletRendering
"""
import json
import os
import re
import traceback

import unreal

OUT = os.environ.get("TFX_DUMP_OUT", os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools", "plugin_assets.json"))
ROOT = "/TransitionFX"

lib = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
dmt = unreal.DevMaterialTools  # get_material_(function_)expressions: MaterialEditingLibrary has them only from UE 5.6

PRESET_PROPS = ("effect_class", "transition_material", "override_transition_color", "transition_color",
                "default_duration", "easing_type", "progress_curve", "auto_block_input", "tick_when_paused",
                "priority", "widget_z_order", "transition_sound", "sound_volume", "sound_pitch")
MATERIAL_PROPS = ("material_domain", "blend_mode", "shading_model", "blendable_location", "blendable_priority",
                  "blendable_output_alpha", "two_sided", "use_material_attributes", "is_blendable",
                  "enable_stencil_test", "used_with_ui")
STAT_KEYS = ("num_pixel_shader_instructions", "num_vertex_shader_instructions", "num_samplers",
             "num_pixel_texture_samples", "num_vertex_texture_samples", "num_uv_scalars", "num_interpolator_scalars")
TEXTURE_PROPS = ("compression_settings", "srgb", "mip_gen_settings", "lod_group", "never_stream", "filter",
                 "address_x", "address_y")


def norm(value):
    """Turns an editor property value into stable JSON data."""
    if value is None or isinstance(value, (bool, int, str)):
        return value
    if isinstance(value, float):
        return round(value, 5)
    if isinstance(value, unreal.Object):
        return value.get_path_name()
    if isinstance(value, unreal.LinearColor):
        return [round(value.r, 5), round(value.g, 5), round(value.b, 5), round(value.a, 5)]
    if isinstance(value, unreal.Name):
        return str(value)
    if isinstance(value, (list, tuple, unreal.Array)):
        return [norm(v) for v in value]
    if isinstance(value, unreal.Map):
        return {str(k): norm(v) for k, v in value.items()}
    if isinstance(value, unreal.EnumBase):
        return value.name
    # Structs and anything else: their text form without memory addresses
    return re.sub(r"\(0x[0-9A-Fa-f]+\)\s*", "", str(value))


def get(obj, prop):
    try:
        return norm(obj.get_editor_property(prop))
    except Exception as ex:
        return f"<error: {ex}>"


def dump_material(mat):
    d = {"class": "Material", "settings": {p: get(mat, p) for p in MATERIAL_PROPS}}
    d["params"] = {}
    for kind in ("scalar", "vector", "texture", "static_switch"):
        names = sorted(str(n) for n in getattr(mel, f"get_{kind}_parameter_names")(mat))
        values = {}
        for n in names:
            try:
                getter = getattr(mel, f"get_material_default_{kind}_parameter_value")
                values[n] = norm(getter(mat, n))
            except Exception as ex:
                values[n] = f"<error: {ex}>"
        d["params"][kind] = values
    d["num_expressions"] = len([e for e in dmt.get_material_expressions(mat)
                                if e.get_class().get_name() != "MaterialExpressionComment"])
    roots = {}
    for mp in ("MP_EMISSIVE_COLOR", "MP_OPACITY", "MP_OPACITY_MASK", "MP_BASE_COLOR"):
        node = mel.get_material_property_input_node(mat, getattr(unreal.MaterialProperty, mp))
        roots[mp] = node.get_class().get_name() if node else None
    d["root_inputs"] = roots
    try:
        stats = mel.get_statistics(mat)
        d["stats"] = {k: stats.get_editor_property(k) for k in STAT_KEYS}
    except Exception as ex:
        d["stats"] = f"<error: {ex}>"
    if hasattr(mel, "get_num_shader_types"):  # not in UE 5.5
        d["num_shader_types"] = mel.get_num_shader_types(mat)
    return d


def dump_function(fn):
    exprs = [e for e in dmt.get_material_function_expressions(fn) if e.get_class().get_name() != "MaterialExpressionComment"]
    inputs, outputs = [], []
    for e in exprs:
        if isinstance(e, unreal.MaterialExpressionFunctionInput):
            inputs.append({"name": get(e, "input_name"), "type": get(e, "input_type"), "sort": get(e, "sort_priority"),
                           "preview": get(e, "preview_value"), "use_preview": get(e, "use_preview_value_as_default")})
        elif isinstance(e, unreal.MaterialExpressionFunctionOutput):
            outputs.append({"name": get(e, "output_name"), "sort": get(e, "sort_priority")})
    return {"class": "MaterialFunction", "description": get(fn, "description"), "expose_to_library": get(fn, "expose_to_library"),
            "inputs": sorted(inputs, key=lambda x: (str(x["sort"]), str(x["name"]))),
            "outputs": sorted(outputs, key=lambda x: (str(x["sort"]), str(x["name"]))), "num_expressions": len(exprs)}


def dump_instance(mi):
    d = {"class": mi.get_class().get_name(), "parent": get(mi, "parent")}
    for arr in ("scalar_parameter_values", "vector_parameter_values", "texture_parameter_values"):
        values = {}
        for v in mi.get_editor_property(arr) or []:
            values[str(v.get_editor_property("parameter_info").get_editor_property("name"))] = norm(v.get_editor_property("parameter_value"))
        d[arr] = values
    d["base_property_overrides"] = get(mi, "base_property_overrides")
    return d


def dump_preset(da):
    return {"class": "TransitionPreset", "object_name": da.get_name(), "props": {p: get(da, p) for p in PRESET_PROPS}}


def dump_sequence(seq):
    entries = []
    for e in seq.get_editor_property("entries"):
        params = e.get_editor_property("override_params")
        entries.append({"preset": get(e, "preset"), "mode": get(e, "mode"), "duration_override": get(e, "duration_override"),
                        "invert": get(e, "invert"), "delay_after": get(e, "delay_after"),
                        "override_params": {k: get(params, k) for k in ("scalar_params", "vector_params", "texture_params")}})
    return {"class": "TransitionSequence", "loop": get(seq, "loop"), "loop_count": get(seq, "loop_count"), "entries": entries}


def dump_texture(tex):
    d = {"class": "Texture2D", "settings": {p: get(tex, p) for p in TEXTURE_PROPS}}
    try:
        d["size"] = [tex.blueprint_get_size_x(), tex.blueprint_get_size_y()]
    except Exception:
        pass
    return d


def main():
    result = {"engine": unreal.SystemLibrary.get_engine_version(), "assets": {}, "errors": {}}
    # コマンドレットのアセットレジストリはプラグインコンテンツを走査済みとは限らない (1.7.0 でエディタモジュールの強制走査を外した)
    unreal.AssetRegistryHelpers.get_asset_registry().scan_paths_synchronous([ROOT], True)
    for path in sorted(lib.list_assets(ROOT, recursive=True, include_folder=False)):
        package = path.split(".")[0]
        try:
            obj = lib.load_asset(package)
            if isinstance(obj, unreal.Material):
                d = dump_material(obj)
            elif isinstance(obj, unreal.MaterialFunction):
                d = dump_function(obj)
            elif isinstance(obj, unreal.MaterialInstance):
                d = dump_instance(obj)
            elif isinstance(obj, unreal.TransitionPreset):
                d = dump_preset(obj)
            elif isinstance(obj, unreal.TransitionSequence):
                d = dump_sequence(obj)
            elif isinstance(obj, unreal.Texture2D):
                d = dump_texture(obj)
            else:
                d = {"class": obj.get_class().get_name() if obj else None}
            result["assets"][package] = d
        except Exception:
            result["errors"][package] = traceback.format_exc()
    os.makedirs(os.path.dirname(OUT), exist_ok=True)
    with open(OUT, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False, indent=1, sort_keys=True)


main()
