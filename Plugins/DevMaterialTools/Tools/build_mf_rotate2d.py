"""
MF_Rotate2D を新規作成する (sandbox の build_mf_rotate2d.py 移植版 / BoxRoll 第1弾)。

    vec2 rotate(vec2 p, float a){
        float c = cos(a), s = sin(a);
        return vec2(c*p.x - s*p.y, s*p.x + c*p.y);
    }

角度はラジアン (Sine/Cosine は period=2π 設定)。
ルール: GLSL ローカル変数 c / s = Named Reroute Declaration / 再利用 = Usage。

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_mf_rotate2d.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MF_PATH = "/TransitionFX/MaterialFunctions"
MF_NAME = "MF_Rotate2D"
TWO_PI = 6.2831853

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools
tools = unreal.AssetToolsHelpers.get_asset_tools()

C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑
C_UV = unreal.LinearColor(0.2, 0.6, 1.0, 1.0)   # コメント枠 = 青

results = []
def _connect(frm, out_name, to, in_name):
    ok = mel.connect_material_expressions(frm, out_name, to, in_name)
    if not ok:
        print(f"[TEST] FAILED connect: {frm.get_class().get_name()}('{out_name}') -> "
              f"{to.get_class().get_name()}('{in_name}')")
    results.append(ok)
    return ok

def vec4(x, y, z, w):
    v = unreal.Vector4f()
    v.set_editor_properties({"x": x, "y": y, "z": z, "w": w})
    return v

full = f"{MF_PATH}/{MF_NAME}"
if lib.does_asset_exist(full):
    lib.delete_asset(full)
fn = tools.create_asset(MF_NAME, MF_PATH, unreal.MaterialFunction,
                        unreal.MaterialFunctionFactoryNew())
fn.set_editor_property("description",
    "2D rotation: rotate(P, A) = (c*P.x - s*P.y, s*P.x + c*P.y) "
    "with c = cos(A), s = sin(A). A is in radians (Sine/Cosine period = 2*pi). "
    "Positive A rotates counter-clockwise.")
fn.set_editor_property("expose_to_library", True)

def expr(cls, x, y):
    return mel.create_material_expression_in_function(fn, cls, x, y)

# --- inputs -----------------------------------------------------------------
p_in = expr(unreal.MaterialExpressionFunctionInput, -1150, -220)
p_in.set_editor_property("input_name", "P")
p_in.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2)
p_in.set_editor_property("sort_priority", 0)
p_in.set_editor_property("preview_value", vec4(0.0, 0.0, 0.0, 0.0))

a_in = expr(unreal.MaterialExpressionFunctionInput, -1150, 60)
a_in.set_editor_property("input_name", "A")
a_in.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_SCALAR)
a_in.set_editor_property("sort_priority", 1)
a_in.set_editor_property("preview_value", vec4(0.0, 0.0, 0.0, 0.0))

# --- P.x / P.y --------------------------------------------------------------
px = expr(unreal.MaterialExpressionComponentMask, -950, -260)
px.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(p_in, "", px, "")

py = expr(unreal.MaterialExpressionComponentMask, -950, -160)
py.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
_connect(p_in, "", py, "")

# --- c = cos(a), s = sin(a)  (Named Reroute, radians) -----------------------
cos_a = expr(unreal.MaterialExpressionCosine, -950, 20)
cos_a.set_editor_property("period", TWO_PI)
_connect(a_in, "", cos_a, "")

sin_a = expr(unreal.MaterialExpressionSine, -950, 160)
sin_a.set_editor_property("period", TWO_PI)
_connect(a_in, "", sin_a, "")

decl_c = expr(unreal.MaterialExpressionNamedRerouteDeclaration, -780, 20)
decl_c.set_editor_property("name", "c")
decl_c.set_editor_property("node_color", C_SC)
_connect(cos_a, "", decl_c, "")

decl_s = expr(unreal.MaterialExpressionNamedRerouteDeclaration, -780, 160)
decl_s.set_editor_property("name", "s")
decl_s.set_editor_property("node_color", C_SC)
_connect(sin_a, "", decl_s, "")

use_c1 = dmt.create_named_reroute_usage_in_function(fn, decl_c, -600, -300)
use_s1 = dmt.create_named_reroute_usage_in_function(fn, decl_s, -600, -180)
use_s2 = dmt.create_named_reroute_usage_in_function(fn, decl_s, -600, -20)
use_c2 = dmt.create_named_reroute_usage_in_function(fn, decl_c, -600, 100)
usages_ok = all(u is not None for u in (use_c1, use_s1, use_s2, use_c2))

# --- out.x = c*P.x - s*P.y --------------------------------------------------
mul_cx = expr(unreal.MaterialExpressionMultiply, -450, -300)
_connect(use_c1, "", mul_cx, "A")
_connect(px, "", mul_cx, "B")

mul_sy = expr(unreal.MaterialExpressionMultiply, -450, -180)
_connect(use_s1, "", mul_sy, "A")
_connect(py, "", mul_sy, "B")

sub_x = expr(unreal.MaterialExpressionSubtract, -300, -260)
_connect(mul_cx, "", sub_x, "A")
_connect(mul_sy, "", sub_x, "B")

# --- out.y = s*P.x + c*P.y --------------------------------------------------
mul_sx = expr(unreal.MaterialExpressionMultiply, -450, -20)
_connect(use_s2, "", mul_sx, "A")
_connect(px, "", mul_sx, "B")

mul_cy = expr(unreal.MaterialExpressionMultiply, -450, 100)
_connect(use_c2, "", mul_cy, "A")
_connect(py, "", mul_cy, "B")

add_y = expr(unreal.MaterialExpressionAdd, -300, 40)
_connect(mul_sx, "", add_y, "A")
_connect(mul_cy, "", add_y, "B")

# --- Result = float2(out.x, out.y) ------------------------------------------
app = expr(unreal.MaterialExpressionAppendVector, -160, -140)
_connect(sub_x, "", app, "A")
_connect(add_y, "", app, "B")

out = expr(unreal.MaterialExpressionFunctionOutput, -20, -140)
out.set_editor_property("output_name", "Result")
_connect(app, "", out, "")

c1 = dmt.create_comment_in_function(
    fn, "rotate(P, A): c = cos(A), s = sin(A) -> (c*P.x - s*P.y, s*P.x + c*P.y). "
        "A in radians (period = 2*pi). CCW for positive A.",
    -1200, -400, 1330, 700, C_UV)
comments_ok = c1 is not None

mel.update_material_function(fn)
saved = lib.save_asset(full)

# ---- 検証 ----
counts = {}
for e in mel.get_material_function_expressions(fn):
    cn = e.get_class().get_name()
    counts[cn] = counts.get(cn, 0) + 1
print(f"[TEST] {MF_NAME} nodes: {dict(sorted(counts.items()))}")
expected = {
    "MaterialExpressionFunctionInput": 2,
    "MaterialExpressionComponentMask": 2,
    "MaterialExpressionCosine": 1,
    "MaterialExpressionSine": 1,
    "MaterialExpressionNamedRerouteDeclaration": 2,
    "MaterialExpressionNamedRerouteUsage": 4,
    "MaterialExpressionMultiply": 4,
    "MaterialExpressionSubtract": 1,
    "MaterialExpressionAdd": 1,
    "MaterialExpressionAppendVector": 1,
    "MaterialExpressionFunctionOutput": 1,
}
nodes_ok = counts == expected

print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] usages_ok={usages_ok} nodes_ok={nodes_ok} comments_ok={comments_ok}")
print(f"[TEST] Saved: {saved}")
passed = all(results) and usages_ok and nodes_ok and comments_ok and saved
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
