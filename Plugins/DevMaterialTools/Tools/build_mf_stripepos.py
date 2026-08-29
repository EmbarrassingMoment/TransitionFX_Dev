"""
MF_StripePos を Named Reroute 注釈付きで in-place 再構築する。
(sandbox の build_mf_stripetransition.py / MF_StripeUvPos 移植版)

  getUvPos(uv):
    targetPos = [x, 1-x, y, 1-y][Direction]   (Step+Lerp でブランチレス)
    threshold = 1 / SplitCount
    return saturate(mod(tp, th) + floor(tp/th) * Delay)

既存アセットを delete+create せず全ノード削除→再構築することで、
M_Transition_StripeCascade / MI からの参照とアセット設定を保持する。
入力名 (UV/Direction/SplitCount/Delay) は既存 MF と同一。
ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage。

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_mf_stripepos.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

FN_PATH = "/TransitionFX/MaterialFunctions/MF_StripePos"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools

C_CO = unreal.LinearColor(1.0, 0.6, 0.1, 1.0)   # パターン座標 = 橙
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑

results = []
def _connect(frm, out_name, to, in_name):
    ok = mel.connect_material_expressions(frm, out_name, to, in_name)
    if not ok:
        print(f"[TEST] FAILED connect: {frm.get_class().get_name()}('{out_name}') -> "
              f"{to.get_class().get_name()}('{in_name}')")
    results.append(ok)
    return ok

def V4s(x):
    v = unreal.Vector4f()
    v.set_editor_properties({"x": x, "y": 0.0, "z": 0.0, "w": 0.0})
    return v

fn = lib.load_asset(FN_PATH)
assert fn, f"{FN_PATH} not found"

n_cleared_comments = dmt.clear_comments_in_function(fn)
mel.delete_all_material_expressions_in_function(fn)
print(f"[TEST] cleared existing graph (comments removed: {n_cleared_comments})")

fn.set_editor_property("description",
    "StripeCascade: per-stripe local position plus stripe index * Delay. "
    "Direction 0:LtoR 1:RtoL 2:BtoT 3:TtoB. "
    "Output range [0, 1/SplitCount + Delay*(SplitCount-1)] saturated to <=1.")
fn.set_editor_property("expose_to_library", True)

def expr(cls, x, y):
    return mel.create_material_expression_in_function(fn, cls, x, y)

def make_input(name, x, y, sort, input_type, default=None):
    n_ = expr(unreal.MaterialExpressionFunctionInput, x, y)
    n_.set_editor_property("input_name", name)
    n_.set_editor_property("input_type", input_type)
    n_.set_editor_property("sort_priority", sort)
    if default is not None:
        n_.set_editor_property("preview_value", V4s(default))
        n_.set_editor_property("use_preview_value_as_default", True)
    return n_

decl_map = {}
def make_decl(name, src, x, y, color):
    dcl = expr(unreal.MaterialExpressionNamedRerouteDeclaration, x, y)
    dcl.set_editor_property("name", name)
    dcl.set_editor_property("node_color", color)
    _connect(src, "", dcl, "")
    decl_map[name] = dcl
    return dcl

def use(name, x, y):
    return dmt.create_named_reroute_usage_in_function(fn, decl_map[name], x, y)

uv_in    = make_input("UV", -2100, -100, 0, unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2)
dir_in   = make_input("Direction", -2100, 100, 1, unreal.FunctionInputType.FUNCTION_INPUT_SCALAR, 0.0)
split_in = make_input("SplitCount", -2100, 250, 2, unreal.FunctionInputType.FUNCTION_INPUT_SCALAR, 8.0)
delay_in = make_input("Delay", -2100, 400, 3, unreal.FunctionInputType.FUNCTION_INPUT_SCALAR, 0.05)

# targetPos: axis = lerp(uv.x, uv.y, step(1.5, Direction)) を fmod(Direction,2) で反転
mask_x = expr(unreal.MaterialExpressionComponentMask, -1900, -150)
mask_x.set_editor_property("r", True); mask_x.set_editor_property("g", False)
mask_x.set_editor_property("b", False); mask_x.set_editor_property("a", False)
_connect(uv_in, "", mask_x, "")
mask_y = expr(unreal.MaterialExpressionComponentMask, -1900, -50)
mask_y.set_editor_property("r", False); mask_y.set_editor_property("g", True)
mask_y.set_editor_property("b", False); mask_y.set_editor_property("a", False)
_connect(uv_in, "", mask_y, "")
is_y = expr(unreal.MaterialExpressionStep, -1900, 80)
is_y.set_editor_property("const_y", 1.5)
_connect(dir_in, "", is_y, "X")
axis = expr(unreal.MaterialExpressionLinearInterpolate, -1750, -80)
_connect(mask_x, "", axis, "A"); _connect(mask_y, "", axis, "B"); _connect(is_y, "", axis, "Alpha")

two_c = expr(unreal.MaterialExpressionConstant, -1900, 220)
two_c.set_editor_property("r", 2.0)
inv_sel = expr(unreal.MaterialExpressionFmod, -1750, 150)
_connect(dir_in, "", inv_sel, "A"); _connect(two_c, "", inv_sel, "B")
axis_inv = expr(unreal.MaterialExpressionOneMinus, -1600, -20)
_connect(axis, "", axis_inv, "")
tp = expr(unreal.MaterialExpressionLinearInterpolate, -1450, -80)
_connect(axis, "", tp, "A"); _connect(axis_inv, "", tp, "B"); _connect(inv_sel, "", tp, "Alpha")
make_decl("targetPos", tp, -1300, -80, C_CO)

# threshold = 1 / SplitCount
th = expr(unreal.MaterialExpressionDivide, -1450, 250)
th.set_editor_property("const_a", 1.0)
_connect(split_in, "", th, "B")
make_decl("threshold", th, -1300, 250, C_SC)

# remainder = mod(targetPos, threshold)
tp_u1 = use("targetPos", -1100, -100)
th_u1 = use("threshold", -1100, 0)
rem = expr(unreal.MaterialExpressionFmod, -950, -50)
_connect(tp_u1, "", rem, "A"); _connect(th_u1, "", rem, "B")
make_decl("remainder", rem, -800, -50, C_SC)

# count = floor(targetPos / threshold)
tp_u2 = use("targetPos", -1100, 150)
th_u2 = use("threshold", -1100, 250)
div_c = expr(unreal.MaterialExpressionDivide, -950, 200)
_connect(tp_u2, "", div_c, "A"); _connect(th_u2, "", div_c, "B")
flr = expr(unreal.MaterialExpressionFloor, -800, 200)
_connect(div_c, "", flr, "")
make_decl("count", flr, -650, 200, C_SC)

# Result = saturate(remainder + count * Delay)
cnt_u = use("count", -500, 200)
mul_d = expr(unreal.MaterialExpressionMultiply, -350, 250)
_connect(cnt_u, "", mul_d, "A"); _connect(delay_in, "", mul_d, "B")
rem_u = use("remainder", -350, 50)
add_r = expr(unreal.MaterialExpressionAdd, -200, 100)
_connect(rem_u, "", add_r, "A"); _connect(mul_d, "", add_r, "B")
sat_r = expr(unreal.MaterialExpressionSaturate, -50, 100)
_connect(add_r, "", sat_r, "")
out_p = expr(unreal.MaterialExpressionFunctionOutput, 100, 100)
out_p.set_editor_property("output_name", "Result")
_connect(sat_r, "", out_p, "")

mel.update_material_function(fn)
saved = lib.save_asset(FN_PATH)

# ---- 検証 ----
decls, usage_count = [], {}
for e in mel.get_material_function_expressions(fn):
    if isinstance(e, unreal.MaterialExpressionNamedRerouteDeclaration):
        decls.append(str(e.get_editor_property("name")))
    elif isinstance(e, unreal.MaterialExpressionNamedRerouteUsage):
        nm = dmt.get_named_reroute_usage_display_name(e)
        usage_count[nm] = usage_count.get(nm, 0) + 1

expected_decls = sorted(["targetPos", "threshold", "remainder", "count"])
expected_usages = {"targetPos": 2, "threshold": 2, "remainder": 1, "count": 1}
print(f"[TEST] decls: {sorted(decls)}")
print(f"[TEST] usages: {dict(sorted(usage_count.items()))}")
print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] Saved: {saved}")
passed = (all(results) and sorted(decls) == expected_decls
          and usage_count == expected_usages and saved)
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
