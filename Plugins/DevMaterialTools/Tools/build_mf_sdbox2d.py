"""
MF_SdBox2D を新規作成する (sandbox の build_mf_sdbox2d.py 移植版 / BoxRoll 第2弾)。

    float sdBox(vec2 p, vec2 b){
        vec2 d = abs(p) - b;
        return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
    }

length(v) は UE に式ノードが無いので sqrt(dot(v, v)) で構築。
ルール: GLSL ローカル変数 d = Named Reroute Declaration / 再利用 = Usage。

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_mf_sdbox2d.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MF_PATH = "/TransitionFX/MaterialFunctions"
MF_NAME = "MF_SdBox2D"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools
tools = unreal.AssetToolsHelpers.get_asset_tools()

C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)   # SDF / マスク = 黄
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # コメント枠 = 緑

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
    "2D box SDF: d = abs(P) - B; returns length(max(d, 0)) + min(max(d.x, d.y), 0). "
    "P = point relative to box center, B = half extents. "
    "Negative inside, positive outside, exact Euclidean distance.")
fn.set_editor_property("expose_to_library", True)

def expr(cls, x, y):
    return mel.create_material_expression_in_function(fn, cls, x, y)

# --- inputs -----------------------------------------------------------------
p_in = expr(unreal.MaterialExpressionFunctionInput, -1250, -160)
p_in.set_editor_property("input_name", "P")
p_in.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2)
p_in.set_editor_property("sort_priority", 0)
p_in.set_editor_property("preview_value", vec4(0.0, 0.0, 0.0, 0.0))

b_in = expr(unreal.MaterialExpressionFunctionInput, -1250, 40)
b_in.set_editor_property("input_name", "B")
b_in.set_editor_property("input_type", unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2)
b_in.set_editor_property("sort_priority", 1)
b_in.set_editor_property("preview_value", vec4(0.5, 0.5, 0.0, 0.0))

# --- d = abs(P) - B  (Named Reroute) ----------------------------------------
abs_p = expr(unreal.MaterialExpressionAbs, -1080, -160)
_connect(p_in, "", abs_p, "")

sub_d = expr(unreal.MaterialExpressionSubtract, -940, -100)
_connect(abs_p, "", sub_d, "A")
_connect(b_in, "", sub_d, "B")

decl_d = expr(unreal.MaterialExpressionNamedRerouteDeclaration, -800, -100)
decl_d.set_editor_property("name", "d")
decl_d.set_editor_property("node_color", C_FX)
_connect(sub_d, "", decl_d, "")

use_d1 = dmt.create_named_reroute_usage_in_function(fn, decl_d, -620, -220)
use_d2 = dmt.create_named_reroute_usage_in_function(fn, decl_d, -620, -40)
use_d3 = dmt.create_named_reroute_usage_in_function(fn, decl_d, -620, 80)
usages_ok = all(u is not None for u in (use_d1, use_d2, use_d3))

# --- length(max(d, 0)) = sqrt(dot(m, m)) ------------------------------------
max0 = expr(unreal.MaterialExpressionMax, -470, -220)
max0.set_editor_property("const_b", 0.0)
_connect(use_d1, "", max0, "A")

dot_mm = expr(unreal.MaterialExpressionDotProduct, -330, -220)
_connect(max0, "", dot_mm, "A")
_connect(max0, "", dot_mm, "B")

sqrt_len = expr(unreal.MaterialExpressionSquareRoot, -200, -220)
_connect(dot_mm, "", sqrt_len, "")

# --- min(max(d.x, d.y), 0) --------------------------------------------------
dx = expr(unreal.MaterialExpressionComponentMask, -470, -40)
dx.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(use_d2, "", dx, "")

dy = expr(unreal.MaterialExpressionComponentMask, -470, 80)
dy.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
_connect(use_d3, "", dy, "")

max_xy = expr(unreal.MaterialExpressionMax, -330, 20)
_connect(dx, "", max_xy, "A")
_connect(dy, "", max_xy, "B")

min0 = expr(unreal.MaterialExpressionMin, -200, 20)
min0.set_editor_property("const_b", 0.0)
_connect(max_xy, "", min0, "A")

# --- Result = outside + inside ----------------------------------------------
add = expr(unreal.MaterialExpressionAdd, -60, -100)
_connect(sqrt_len, "", add, "A")
_connect(min0, "", add, "B")

out = expr(unreal.MaterialExpressionFunctionOutput, 70, -100)
out.set_editor_property("output_name", "Result")
_connect(add, "", out, "")

c1 = dmt.create_comment_in_function(
    fn, "sdBox(P, B): d = abs(P) - B -> length(max(d, 0)) + min(max(d.x, d.y), 0). "
        "B = half extents. length = sqrt(dot(v, v)).",
    -1300, -320, 1520, 560, C_SC)
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
    "MaterialExpressionAbs": 1,
    "MaterialExpressionSubtract": 1,
    "MaterialExpressionNamedRerouteDeclaration": 1,
    "MaterialExpressionNamedRerouteUsage": 3,
    "MaterialExpressionMax": 2,
    "MaterialExpressionMin": 1,
    "MaterialExpressionComponentMask": 2,
    "MaterialExpressionDotProduct": 1,
    "MaterialExpressionSquareRoot": 1,
    "MaterialExpressionAdd": 1,
    "MaterialExpressionFunctionOutput": 1,
}
nodes_ok = counts == expected

print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] usages_ok={usages_ok} nodes_ok={nodes_ok} comments_ok={comments_ok}")
print(f"[TEST] Saved: {saved}")
passed = all(results) and usages_ok and nodes_ok and comments_ok and saved
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
