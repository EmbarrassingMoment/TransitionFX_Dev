"""
既存 Stripe Cascade アセット群の構成ダンプ(上書き前の規約確認用)。
  M_Transition_StripeCascade / MF_StripePos / MF_ApplyInvert / MI / DA

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/dump_stripecascade.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary

def dump_expr(e, indent="  "):
    cls = e.get_class().get_name()
    x = e.get_editor_property("material_expression_editor_x")
    y = e.get_editor_property("material_expression_editor_y")
    extra = ""
    for prop in ("parameter_name", "name", "input_name", "output_name", "default_value",
                 "const_a", "const_b", "const_y", "r", "scene_texture_id", "material_function",
                 "input_type", "sort_priority", "preview_value", "use_preview_value_as_default",
                 "desc"):
        try:
            v = e.get_editor_property(prop)
            if v is not None and v != "":
                extra += f" {prop}={v}"
        except Exception:
            pass
    print(f"{indent}({x:6},{y:6}) {cls}{extra}")

def dump_material(path):
    mat = lib.load_asset(path)
    if not mat:
        print(f"[DUMP] NOT FOUND: {path}")
        return
    print(f"[DUMP] ===== {path} =====")
    try:
        print(f"[DUMP] domain={mat.get_editor_property('material_domain')}")
        print(f"[DUMP] blendable_location={mat.get_editor_property('blendable_location')}")
        print(f"[DUMP] blendable_priority={mat.get_editor_property('blendable_priority')}")
        print(f"[DUMP] output_alpha={mat.get_editor_property('blendable_output_alpha')}")
    except Exception as ex:
        print(f"[DUMP] prop error: {ex}")
    for e in mel.get_material_expressions(mat):
        dump_expr(e)

def dump_function(path):
    fn = lib.load_asset(path)
    if not fn:
        print(f"[DUMP] NOT FOUND: {path}")
        return
    print(f"[DUMP] ===== {path} =====")
    try:
        print(f"[DUMP] description={fn.get_editor_property('description')!r}")
        print(f"[DUMP] expose_to_library={fn.get_editor_property('expose_to_library')}")
    except Exception:
        pass
    for e in mel.get_material_function_expressions(fn):
        dump_expr(e)

def dump_instance(path):
    mi = lib.load_asset(path)
    if not mi:
        print(f"[DUMP] NOT FOUND: {path}")
        return
    print(f"[DUMP] ===== {path} =====")
    print(f"[DUMP] parent={mi.get_editor_property('parent')}")
    for spv in mi.get_editor_property("scalar_parameter_values"):
        print(f"[DUMP]   scalar {spv.get_editor_property('parameter_info').get_editor_property('name')} = {spv.get_editor_property('parameter_value')}")
    for vpv in mi.get_editor_property("vector_parameter_values"):
        print(f"[DUMP]   vector {vpv.get_editor_property('parameter_info').get_editor_property('name')} = {vpv.get_editor_property('parameter_value')}")

def dump_preset(path):
    da = lib.load_asset(path)
    if not da:
        print(f"[DUMP] NOT FOUND: {path}")
        return
    print(f"[DUMP] ===== {path} =====")
    for prop in ("effect_class", "transition_material", "default_duration", "easing_type",
                 "material_params", "b_override_transition_color", "transition_color"):
        try:
            print(f"[DUMP]   {prop} = {da.get_editor_property(prop)}")
        except Exception as ex:
            print(f"[DUMP]   {prop}: <n/a>")

dump_material("/TransitionFX/Materials/M_Transition_StripeCascade")
dump_function("/TransitionFX/MaterialFunctions/MF_StripePos")
dump_function("/TransitionFX/MaterialFunctions/MF_ApplyInvert")
dump_function("/TransitionFX/MaterialFunctions/MF_CorrectAspectRatio")
dump_instance("/TransitionFX/Materials/Instances/MI_Transition_StripeCascade")
dump_preset("/TransitionFX/Data/DA_StripeCascade")
print("[DUMP] done")
