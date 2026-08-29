"""
MF_RowDist を新規作成する (sandbox の build_mf_rowdist.py 移植版 / BoxRoll 第4弾)。

    float rowDist(vec2 uv, float j, float t)
    {
        // 飛行中の位置 x_i(t) = startX - SPEED*(t - j*DT_ROW) + i*SPEED*DT_COL を i について逆算
        float iFly  = floor((uv.x - gStartX + SPEED * (t - j * DT_ROW))
                            / (SPEED * DT_COL) + 0.5);
        float iLand = floor(uv.x / gS);
        return min(boxDist(uv, iFly, j, t), boxDist(uv, iLand, j, t));
    }

- 入力は MF_BoxDist から I を除いた12個 (iFly/iLand は内部で算出)
- MF_BoxDist を2回 MaterialFunctionCall (グローバル群はそのまま転送)
- ルール: GLSL ローカル変数 iFly / iLand = Named Reroute Declaration / 再利用 = Usage

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_mf_rowdist.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MF_PATH = "/TransitionFX/MaterialFunctions"
MF_NAME = "MF_RowDist"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools
tools = unreal.AssetToolsHelpers.get_asset_tools()

mf_boxdist = lib.load_asset(f"{MF_PATH}/MF_BoxDist")
assert mf_boxdist, "MF_BoxDist missing"

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
    "Rolling-box transition: SDF of row J at time T. Only two boxes can touch a "
    "pixel in this row: the nearest flying one (column index inverse-solved from "
    "the flight motion) and the landed one in the pixel's own cell. "
    "Returns min of the two MF_BoxDist calls.")
fn.set_editor_property("expose_to_library", True)

def expr(cls, x, y):
    return mel.create_material_expression_in_function(fn, cls, x, y)

C_ID = unreal.LinearColor(0.3, 0.9, 0.9, 1.0)  # 列インデックス = 青緑
C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)  # SDF / マスク = 黄

usages = []
def use(d, x, y):
    u = dmt.create_named_reroute_usage_in_function(fn, d, x, y)
    usages.append(u)
    return u

def f_in(name, ftype, prio, x, y, pv):
    e = expr(unreal.MaterialExpressionFunctionInput, x, y)
    e.set_editor_property("input_name", name)
    e.set_editor_property("input_type", ftype)
    e.set_editor_property("sort_priority", prio)
    e.set_editor_property("preview_value", pv)
    return e

V2 = unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2
SC = unreal.FunctionInputType.FUNCTION_INPUT_SCALAR

uv_in  = f_in("UV",       V2, 0,  -2000, -700, vec4(0.8, 0.1, 0.0, 0.0))
j_in   = f_in("J",        SC, 1,  -2000, -590, vec4(0.0, 0.0, 0.0, 0.0))
t_in   = f_in("T",        SC, 2,  -2000, -480, vec4(1.0, 0.0, 0.0, 0.0))
s_in   = f_in("S",        SC, 3,  -2000, -370, vec4(0.1666667, 0.0, 0.0, 0.0))
rows_in= f_in("Rows",     SC, 4,  -2000, -260, vec4(6.0, 0.0, 0.0, 0.0))
cols_in= f_in("Cols",     SC, 5,  -2000, -150, vec4(11.0, 0.0, 0.0, 0.0))
dtc_in = f_in("DTCol",    SC, 6,  -2000,  -40, vec4(0.2, 0.0, 0.0, 0.0))
dtr_in = f_in("DTRow",    SC, 7,  -2000,   70, vec4(0.5, 0.0, 0.0, 0.0))
spd_in = f_in("Speed",    SC, 8,  -2000,  180, vec4(3.5, 0.0, 0.0, 0.0))
sx_in  = f_in("StartX",   SC, 9,  -2000,  290, vec4(1.9444444, 0.0, 0.0, 0.0))
hd_in  = f_in("HalfDiag", SC, 10, -2000,  400, vec4(0.1178511, 0.0, 0.0, 0.0))
px_in  = f_in("Px",       SC, 11, -2000,  510, vec4(0.0009259, 0.0, 0.0, 0.0))

# --- uv.x -------------------------------------------------------------------
ux = expr(unreal.MaterialExpressionComponentMask, -1750, -700)
ux.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(uv_in, "", ux, "")

# --- iFly = floor((uv.x - StartX + Speed*(T - J*DTRow)) / (Speed*DTCol) + 0.5)
mul_jdtr = expr(unreal.MaterialExpressionMultiply, -1750, -380)
_connect(j_in, "", mul_jdtr, "A")
_connect(dtr_in, "", mul_jdtr, "B")

sub_tj = expr(unreal.MaterialExpressionSubtract, -1600, -400)
_connect(t_in, "", sub_tj, "A")
_connect(mul_jdtr, "", sub_tj, "B")

mul_sp = expr(unreal.MaterialExpressionMultiply, -1450, -420)
_connect(spd_in, "", mul_sp, "A")
_connect(sub_tj, "", mul_sp, "B")

sub_ux = expr(unreal.MaterialExpressionSubtract, -1600, -560)
_connect(ux, "", sub_ux, "A")
_connect(sx_in, "", sub_ux, "B")

add_num = expr(unreal.MaterialExpressionAdd, -1300, -500)
_connect(sub_ux, "", add_num, "A")
_connect(mul_sp, "", add_num, "B")

mul_den = expr(unreal.MaterialExpressionMultiply, -1450, -260)
_connect(spd_in, "", mul_den, "A")
_connect(dtc_in, "", mul_den, "B")

div_fly = expr(unreal.MaterialExpressionDivide, -1150, -450)
_connect(add_num, "", div_fly, "A")
_connect(mul_den, "", div_fly, "B")

add_half = expr(unreal.MaterialExpressionAdd, -1000, -450)
add_half.set_editor_property("const_b", 0.5)
_connect(div_fly, "", add_half, "A")

floor_fly = expr(unreal.MaterialExpressionFloor, -860, -450)
_connect(add_half, "", floor_fly, "")

d_iFly = expr(unreal.MaterialExpressionNamedRerouteDeclaration, -720, -450)
d_iFly.set_editor_property("name", "iFly")
d_iFly.set_editor_property("node_color", C_ID)
_connect(floor_fly, "", d_iFly, "")

# --- iLand = floor(uv.x / S) ------------------------------------------------
div_land = expr(unreal.MaterialExpressionDivide, -1150, -120)
_connect(ux, "", div_land, "A")
_connect(s_in, "", div_land, "B")

floor_land = expr(unreal.MaterialExpressionFloor, -1000, -120)
_connect(div_land, "", floor_land, "")

d_iLand = expr(unreal.MaterialExpressionNamedRerouteDeclaration, -860, -120)
d_iLand.set_editor_property("name", "iLand")
d_iLand.set_editor_property("node_color", C_ID)
_connect(floor_land, "", d_iLand, "")

# --- min(boxDist(iFly), boxDist(iLand)) -------------------------------------
def boxdist_call(x, y, i_node):
    call = expr(unreal.MaterialExpressionMaterialFunctionCall, x, y)
    call.set_editor_property("material_function", mf_boxdist)
    _connect(uv_in, "", call, "UV")
    _connect(i_node, "", call, "I")
    _connect(j_in, "", call, "J")
    _connect(t_in, "", call, "T")
    _connect(s_in, "", call, "S")
    _connect(rows_in, "", call, "Rows")
    _connect(cols_in, "", call, "Cols")
    _connect(dtc_in, "", call, "DTCol")
    _connect(dtr_in, "", call, "DTRow")
    _connect(spd_in, "", call, "Speed")
    _connect(sx_in, "", call, "StartX")
    _connect(hd_in, "", call, "HalfDiag")
    _connect(px_in, "", call, "Px")
    return call

u_iFly = use(d_iFly, -560, -520)
call_fly = boxdist_call(-380, -560, u_iFly)

u_iLand = use(d_iLand, -560, -40)
call_land = boxdist_call(-380, 0, u_iLand)

min_d = expr(unreal.MaterialExpressionMin, -120, -280)
_connect(call_fly, "", min_d, "A")
_connect(call_land, "", min_d, "B")

out = expr(unreal.MaterialExpressionFunctionOutput, 20, -280)
out.set_editor_property("output_name", "Result")
_connect(min_d, "", out, "")

# --- comments ---------------------------------------------------------------
COMMENTS = [
    ("iFly: inverse-solve the flying column index from the flight motion "
     "x_i(t) = StartX - Speed*(t - J*DTRow) + i*Speed*DTCol "
     "=> i = floor((uv.x - StartX + Speed*(t - J*DTRow)) / (Speed*DTCol) + 0.5). "
     "Only the nearest flying box can touch this pixel.",
     -1800, -660, 1220, 460, C_ID),
    ("iLand: the landed box in this pixel's own cell = floor(uv.x / S).",
     -1200, -190, 610, 220, C_ID),
    ("Row SDF = min(boxDist(iFly), boxDist(iLand)). "
     "MF_BoxDist returns 1e9 for out-of-range / not-launched indices, "
     "so bogus inverse-solved columns are harmless.",
     -640, -640, 780, 880, C_FX),
]
comment_ok = []
for text, x, y, w, h, color in COMMENTS:
    c = dmt.create_comment_in_function(fn, text, x, y, w, h, color)
    comment_ok.append(c is not None)

mel.update_material_function(fn)
saved = lib.save_asset(full)

# ---- 検証 ----
counts = {}
for e in mel.get_material_function_expressions(fn):
    cn = e.get_class().get_name()
    counts[cn] = counts.get(cn, 0) + 1
print(f"[TEST] {MF_NAME} nodes: {dict(sorted(counts.items()))}")
expected = {
    "MaterialExpressionFunctionInput": 12,
    "MaterialExpressionComponentMask": 1,
    "MaterialExpressionMultiply": 3,
    "MaterialExpressionSubtract": 2,
    "MaterialExpressionAdd": 2,
    "MaterialExpressionDivide": 2,
    "MaterialExpressionFloor": 2,
    "MaterialExpressionNamedRerouteDeclaration": 2,
    "MaterialExpressionNamedRerouteUsage": 2,
    "MaterialExpressionMaterialFunctionCall": 2,
    "MaterialExpressionMin": 1,
    "MaterialExpressionFunctionOutput": 1,
}
nodes_ok = counts == expected
usages_ok = all(u is not None for u in usages) and len(usages) == 2

print(f"[TEST] Comments created: {sum(comment_ok)}/{len(comment_ok)}")
print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] usages_ok={usages_ok} nodes_ok={nodes_ok}")
print(f"[TEST] Saved: {saved}")
passed = all(results) and usages_ok and nodes_ok and all(comment_ok) and saved
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
