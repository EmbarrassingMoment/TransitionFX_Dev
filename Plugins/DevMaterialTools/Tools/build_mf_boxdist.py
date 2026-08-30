"""
MF_BoxDist を新規作成する (sandbox の build_mf_boxdist.py 桁落ちバグ修正済み版の移植 / BoxRoll 第3弾)。

    float boxDist(vec2 uv, float i, float j, float t)
    {
        if (i < 0.0 || i > gCols - 1.0 || j < 0.0 || j > ROWS - 1.0) return 1e9;
        float targetX = (i + 0.5) * gS;
        float t0  = j * DT_ROW + i * DT_COL;
        float dur = (gStartX - targetX) / SPEED;
        float p   = clamp((t - t0) / dur, 0.0, 1.0);
        if (p <= 0.0) return 1e9;
        float x   = mix(gStartX, targetX, p);
        float ang = (targetX - x) / gS * PI2;
        float yc  = j * gS + gHalfDiag * cos(mod(ang, PI2) - PI4);
        vec2 q = rotate(uv - vec2(x, yc), -ang);
        return sdBox(q, vec2(0.5 * gS + gPx));
    }

- グローバル (gS/gCols/gStartX 等) はスカラー入力化
- early return 2箇所は step マスク積 → sd*valid + 1e9*(1-valid) でブランチレス化
  (lerp(1e9, sd, valid) は fp32 桁落ちで sd が 0 に消えるため不可)
- dur は max(dur, 1e-4) でゼロ割りガード (RowDist の iFly 逆算対策、NaN は select を汚染する)
- mod は既存の MF_GLSLMod (入力 A/B、Vector2 型)。スカラー ang はスプラットされ
  出力が float2 になるので直後に ComponentMask(R) でスカラーへ戻す
- rotate は MF_Rotate2D、sdBox は MF_SdBox2D を呼ぶ
- ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_mf_boxdist.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MF_PATH = "/TransitionFX/MaterialFunctions"
MF_NAME = "MF_BoxDist"
PI2 = 1.5707963
PI4 = 0.78539816
TWO_PI = 6.2831853

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools
tools = unreal.AssetToolsHelpers.get_asset_tools()

mf_mod = lib.load_asset(f"{MF_PATH}/MF_GLSLMod")
mf_rot = lib.load_asset(f"{MF_PATH}/MF_Rotate2D")
mf_box = lib.load_asset(f"{MF_PATH}/MF_SdBox2D")
assert mf_mod and mf_rot and mf_box, "dependency MFs missing"

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
    "Rolling-box transition: SDF of box (I, J) at time T, or 1e9 when the box is "
    "out of range / not launched yet (branchless select: sd*valid + 1e9*(1-valid); "
    "NOT lerp(1e9, sd, valid), which cancels sd to 0 in fp32). "
    "Flight duration is clamped to 1e-4 to avoid div-by-zero NaN when RowDist's "
    "inverse-solved column index lands on TargetX == StartX. "
    "Calls MF_GLSLMod / MF_Rotate2D / MF_SdBox2D.")
fn.set_editor_property("expose_to_library", True)

def expr(cls, x, y):
    return mel.create_material_expression_in_function(fn, cls, x, y)

C_CO = unreal.LinearColor(1.0, 0.6, 0.1, 1.0)   # パターン座標 = 橙
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑
C_MK = unreal.LinearColor(1.0, 0.35, 0.35, 1.0) # 有効マスク = 赤
C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)   # SDF / マスク = 黄

def decl(name, x, y, color):
    d = expr(unreal.MaterialExpressionNamedRerouteDeclaration, x, y)
    d.set_editor_property("name", name)
    d.set_editor_property("node_color", color)
    return d

usages = []
def use(d, x, y):
    u = dmt.create_named_reroute_usage_in_function(fn, d, x, y)
    usages.append(u)
    return u

# --- inputs (GLSL args + globals) -------------------------------------------
def f_in(name, ftype, prio, x, y, pv):
    e = expr(unreal.MaterialExpressionFunctionInput, x, y)
    e.set_editor_property("input_name", name)
    e.set_editor_property("input_type", ftype)
    e.set_editor_property("sort_priority", prio)
    e.set_editor_property("preview_value", pv)
    return e

V2 = unreal.FunctionInputType.FUNCTION_INPUT_VECTOR2
SC = unreal.FunctionInputType.FUNCTION_INPUT_SCALAR

uv_in  = f_in("UV",       V2, 0,  -2450, -900, vec4(0.8, 0.1, 0.0, 0.0))
i_in   = f_in("I",        SC, 1,  -2450, -780, vec4(0.0, 0.0, 0.0, 0.0))
j_in   = f_in("J",        SC, 2,  -2450, -660, vec4(0.0, 0.0, 0.0, 0.0))
t_in   = f_in("T",        SC, 3,  -2450, -540, vec4(1.0, 0.0, 0.0, 0.0))
s_in   = f_in("S",        SC, 4,  -2450, -420, vec4(0.1666667, 0.0, 0.0, 0.0))
rows_in= f_in("Rows",     SC, 5,  -2450, -300, vec4(6.0, 0.0, 0.0, 0.0))
cols_in= f_in("Cols",     SC, 6,  -2450, -180, vec4(11.0, 0.0, 0.0, 0.0))
dtc_in = f_in("DTCol",    SC, 7,  -2450,  -60, vec4(0.2, 0.0, 0.0, 0.0))
dtr_in = f_in("DTRow",    SC, 8,  -2450,   60, vec4(0.5, 0.0, 0.0, 0.0))
spd_in = f_in("Speed",    SC, 9,  -2450,  180, vec4(3.5, 0.0, 0.0, 0.0))
sx_in  = f_in("StartX",   SC, 10, -2450,  300, vec4(1.9444444, 0.0, 0.0, 0.0))
hd_in  = f_in("HalfDiag", SC, 11, -2450,  420, vec4(0.1178511, 0.0, 0.0, 0.0))
px_in  = f_in("Px",       SC, 12, -2450,  540, vec4(0.0009259, 0.0, 0.0, 0.0))

# --- bounds check: 0 <= I <= Cols-1 && 0 <= J <= Rows-1 ---------------------
cols1 = expr(unreal.MaterialExpressionSubtract, -2100, -950)
cols1.set_editor_property("const_b", 1.0)
_connect(cols_in, "", cols1, "A")

rows1 = expr(unreal.MaterialExpressionSubtract, -2100, -700)
rows1.set_editor_property("const_b", 1.0)
_connect(rows_in, "", rows1, "A")

step_i0 = expr(unreal.MaterialExpressionStep, -1950, -1050)   # I >= 0
step_i0.set_editor_property("const_y", 0.0)
_connect(i_in, "", step_i0, "X")

step_ic = expr(unreal.MaterialExpressionStep, -1950, -925)    # Cols-1 >= I
_connect(cols1, "", step_ic, "X")
_connect(i_in, "", step_ic, "Y")

step_j0 = expr(unreal.MaterialExpressionStep, -1950, -800)    # J >= 0
step_j0.set_editor_property("const_y", 0.0)
_connect(j_in, "", step_j0, "X")

step_jr = expr(unreal.MaterialExpressionStep, -1950, -675)    # Rows-1 >= J
_connect(rows1, "", step_jr, "X")
_connect(j_in, "", step_jr, "Y")

m_i = expr(unreal.MaterialExpressionMultiply, -1800, -1000)
_connect(step_i0, "", m_i, "A")
_connect(step_ic, "", m_i, "B")

m_j = expr(unreal.MaterialExpressionMultiply, -1800, -750)
_connect(step_j0, "", m_j, "A")
_connect(step_jr, "", m_j, "B")

m_ij = expr(unreal.MaterialExpressionMultiply, -1650, -900)
_connect(m_i, "", m_ij, "A")
_connect(m_j, "", m_ij, "B")

# --- targetX = (I + 0.5) * S ------------------------------------------------
add_i = expr(unreal.MaterialExpressionAdd, -2100, -550)
add_i.set_editor_property("const_b", 0.5)
_connect(i_in, "", add_i, "A")

mul_tx = expr(unreal.MaterialExpressionMultiply, -1950, -550)
_connect(add_i, "", mul_tx, "A")
_connect(s_in, "", mul_tx, "B")

d_targetX = decl("targetX", -1800, -550, C_CO)
_connect(mul_tx, "", d_targetX, "")

# --- t0 = J * DTRow + I * DTCol ---------------------------------------------
mul_jr = expr(unreal.MaterialExpressionMultiply, -2100, -380)
_connect(j_in, "", mul_jr, "A")
_connect(dtr_in, "", mul_jr, "B")

mul_ic = expr(unreal.MaterialExpressionMultiply, -2100, -260)
_connect(i_in, "", mul_ic, "A")
_connect(dtc_in, "", mul_ic, "B")

add_t0 = expr(unreal.MaterialExpressionAdd, -1950, -320)
_connect(mul_jr, "", add_t0, "A")
_connect(mul_ic, "", add_t0, "B")

d_t0 = decl("t0", -1800, -320, C_SC)
_connect(add_t0, "", d_t0, "")

# --- dur = max((StartX - targetX) / Speed, 1e-4)  (NaN guard) ---------------
u_tx1 = use(d_targetX, -1650, -500)
sub_sx = expr(unreal.MaterialExpressionSubtract, -1500, -560)
_connect(sx_in, "", sub_sx, "A")
_connect(u_tx1, "", sub_sx, "B")

div_dur = expr(unreal.MaterialExpressionDivide, -1350, -560)
_connect(sub_sx, "", div_dur, "A")
_connect(spd_in, "", div_dur, "B")

max_dur = expr(unreal.MaterialExpressionMax, -1200, -560)
max_dur.set_editor_property("const_b", 1.0e-4)
_connect(div_dur, "", max_dur, "A")

d_dur = decl("dur", -1050, -560, C_SC)
_connect(max_dur, "", d_dur, "")

# --- p = clamp((T - t0) / dur, 0, 1) ----------------------------------------
u_t0 = use(d_t0, -1650, -320)
sub_t = expr(unreal.MaterialExpressionSubtract, -1500, -350)
_connect(t_in, "", sub_t, "A")
_connect(u_t0, "", sub_t, "B")

u_dur = use(d_dur, -1500, -230)
div_p = expr(unreal.MaterialExpressionDivide, -1350, -320)
_connect(sub_t, "", div_p, "A")
_connect(u_dur, "", div_p, "B")

clamp_p = expr(unreal.MaterialExpressionClamp, -1200, -320)
clamp_p.set_editor_property("min_default", 0.0)
clamp_p.set_editor_property("max_default", 1.0)
_connect(div_p, "", clamp_p, "")

d_p = decl("p", -1050, -320, C_SC)
_connect(clamp_p, "", d_p, "")

# --- valid = boundsOK * (p > 0) ---------------------------------------------
u_p1 = use(d_p, -900, -800)
step_np = expr(unreal.MaterialExpressionStep, -760, -800)     # 0 >= p (not launched)
step_np.set_editor_property("const_x", 0.0)
_connect(u_p1, "", step_np, "Y")

launched = expr(unreal.MaterialExpressionOneMinus, -620, -800)
_connect(step_np, "", launched, "")

m_valid = expr(unreal.MaterialExpressionMultiply, -480, -850)
_connect(m_ij, "", m_valid, "A")
_connect(launched, "", m_valid, "B")

d_valid = decl("valid", -340, -850, C_MK)
_connect(m_valid, "", d_valid, "")

# --- x = mix(StartX, targetX, p) --------------------------------------------
u_tx2 = use(d_targetX, -900, -120)
u_p2 = use(d_p, -900, -40)
lerp_x = expr(unreal.MaterialExpressionLinearInterpolate, -740, -100)
_connect(sx_in, "", lerp_x, "A")
_connect(u_tx2, "", lerp_x, "B")
_connect(u_p2, "", lerp_x, "Alpha")

d_x = decl("x", -590, -100, C_CO)
_connect(lerp_x, "", d_x, "")

# --- ang = (targetX - x) / S * PI2 ------------------------------------------
u_tx3 = use(d_targetX, -440, 60)
u_x1 = use(d_x, -440, 140)
sub_ax = expr(unreal.MaterialExpressionSubtract, -290, 100)
_connect(u_tx3, "", sub_ax, "A")
_connect(u_x1, "", sub_ax, "B")

div_as = expr(unreal.MaterialExpressionDivide, -150, 100)
_connect(sub_ax, "", div_as, "A")
_connect(s_in, "", div_as, "B")

c_pi2 = expr(unreal.MaterialExpressionConstant, -150, 220)
c_pi2.set_editor_property("r", PI2)

mul_pi2 = expr(unreal.MaterialExpressionMultiply, -10, 100)
_connect(div_as, "", mul_pi2, "A")
_connect(c_pi2, "", mul_pi2, "B")

d_ang = decl("ang", 130, 100, C_SC)
_connect(mul_pi2, "", d_ang, "")

# --- yc = J * S + HalfDiag * cos(mod(ang, PI2) - PI4) -----------------------
# MF_GLSLMod は Vector2 入力 (A/B)。スカラー ang はスプラットされ出力が
# float2 になるため、ComponentMask(R) でスカラーへ戻してから cos 系へ流す。
u_ang1 = use(d_ang, 280, 280)
mod_call = expr(unreal.MaterialExpressionMaterialFunctionCall, 430, 280)
mod_call.set_editor_property("material_function", mf_mod)
_connect(u_ang1, "", mod_call, "A")
_connect(c_pi2, "", mod_call, "B")

mod_r = expr(unreal.MaterialExpressionComponentMask, 570, 280)
mod_r.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(mod_call, "", mod_r, "")

sub_pi4 = expr(unreal.MaterialExpressionSubtract, 700, 280)
sub_pi4.set_editor_property("const_b", PI4)
_connect(mod_r, "", sub_pi4, "A")

cos_yc = expr(unreal.MaterialExpressionCosine, 840, 280)
cos_yc.set_editor_property("period", TWO_PI)
_connect(sub_pi4, "", cos_yc, "")

mul_hd = expr(unreal.MaterialExpressionMultiply, 980, 280)
_connect(hd_in, "", mul_hd, "A")
_connect(cos_yc, "", mul_hd, "B")

mul_js = expr(unreal.MaterialExpressionMultiply, 840, 430)
_connect(j_in, "", mul_js, "A")
_connect(s_in, "", mul_js, "B")

add_yc = expr(unreal.MaterialExpressionAdd, 1130, 340)
_connect(mul_js, "", add_yc, "A")
_connect(mul_hd, "", add_yc, "B")

d_yc = decl("yc", 1270, 340, C_CO)
_connect(add_yc, "", d_yc, "")

# --- q = rotate(UV - vec2(x, yc), -ang) -------------------------------------
u_x2 = use(d_x, 1270, 520)
u_yc1 = use(d_yc, 1270, 600)
app_xy = expr(unreal.MaterialExpressionAppendVector, 1430, 560)
_connect(u_x2, "", app_xy, "A")
_connect(u_yc1, "", app_xy, "B")

sub_uv = expr(unreal.MaterialExpressionSubtract, 1580, 520)
_connect(uv_in, "", sub_uv, "A")
_connect(app_xy, "", sub_uv, "B")

u_ang2 = use(d_ang, 1430, 700)
neg_ang = expr(unreal.MaterialExpressionMultiply, 1580, 700)
neg_ang.set_editor_property("const_b", -1.0)
_connect(u_ang2, "", neg_ang, "A")

rot_call = expr(unreal.MaterialExpressionMaterialFunctionCall, 1730, 560)
rot_call.set_editor_property("material_function", mf_rot)
_connect(sub_uv, "", rot_call, "P")
_connect(neg_ang, "", rot_call, "A")

d_q = decl("q", 1890, 560, C_CO)
_connect(rot_call, "", d_q, "")

# --- sdBox(q, vec2(0.5 * S + Px)) -------------------------------------------
mul_hs = expr(unreal.MaterialExpressionMultiply, 1580, 860)
mul_hs.set_editor_property("const_b", 0.5)
_connect(s_in, "", mul_hs, "A")

add_px = expr(unreal.MaterialExpressionAdd, 1730, 860)
_connect(mul_hs, "", add_px, "A")
_connect(px_in, "", add_px, "B")

app_b = expr(unreal.MaterialExpressionAppendVector, 1880, 860)
_connect(add_px, "", app_b, "A")
_connect(add_px, "", app_b, "B")

u_q1 = use(d_q, 2040, 640)
sd_call = expr(unreal.MaterialExpressionMaterialFunctionCall, 2190, 680)
sd_call.set_editor_property("material_function", mf_box)
_connect(u_q1, "", sd_call, "P")
_connect(app_b, "", sd_call, "B")

# --- Result = sd * valid + 1e9 * (1 - valid)  (exact select) ----------------
# lerp(1e9, sd, valid) は使えない: GPU の lerp は a + s*(b - a) で、a=1e9 だと
# float32 の桁落ちで sd (〜数単位) が丸め消失し、valid=1 でも 0 が返る。
# valid は正確に 0/1 なので乗算セレクトなら厳密。
u_valid1 = use(d_valid, 2190, 840)
mul_sd = expr(unreal.MaterialExpressionMultiply, 2350, 700)
_connect(sd_call, "", mul_sd, "A")
_connect(u_valid1, "", mul_sd, "B")

u_valid2 = use(d_valid, 2190, 980)
om_valid = expr(unreal.MaterialExpressionOneMinus, 2350, 980)
_connect(u_valid2, "", om_valid, "")
mul_big = expr(unreal.MaterialExpressionMultiply, 2500, 980)
mul_big.set_editor_property("const_b", 1.0e9)
_connect(om_valid, "", mul_big, "A")

add_out = expr(unreal.MaterialExpressionAdd, 2650, 780)
_connect(mul_sd, "", add_out, "A")
_connect(mul_big, "", add_out, "B")

out = expr(unreal.MaterialExpressionFunctionOutput, 2800, 780)
out.set_editor_property("output_name", "Result")
_connect(add_out, "", out, "")

# --- comments ---------------------------------------------------------------
COMMENTS = [
    ("valid = (0 <= I <= Cols-1) * (0 <= J <= Rows-1) * (p > 0). "
     "Replaces both GLSL early returns (branchless).",
     -2150, -1150, 1780, 560, C_MK),
    ("targetX = (I + 0.5) * S, t0 = J*DTRow + I*DTCol, "
     "dur = max((StartX - targetX) / Speed, 1e-4), p = clamp((T - t0) / dur, 0, 1). "
     "The max() guard is not in the GLSL: RowDist inverse-solves the column index, "
     "so targetX == StartX can occur and NaN would leak through the final select.",
     -2150, -660, 1250, 500, C_SC),
    ("x = mix(StartX, targetX, p): flight position. "
     "ang = (targetX - x) / S * PI/2: rolled angle (one quarter turn per cell).",
     -950, -180, 1230, 440, C_CO),
    ("yc = J*S + HalfDiag * cos(mod(ang, PI/2) - PI/4): center height bobs on the "
     "rolling square's half-diagonal. mod() must be GLSL-style (ang <= 0 here) -> "
     "MF_GLSLMod(A, B). Its inputs are Vector2, so the scalar ang splats to float2 "
     "and ComponentMask(R) restores a scalar.",
     230, 200, 1190, 330, C_CO),
    ("q = rotate(UV - (x, yc), -ang); sdBox(q, 0.5*S + Px). "
     "Result = sd*valid + 1e9*(1-valid): exact select, NOT lerp(1e9, sd, valid). "
     "GPU lerp is a + s*(b-a); with a=1e9 the fp32 ulp is 64, so sd cancels to 0 "
     "even at valid=1 (rounds through 1e9 + (sd - 1e9) = 0).",
     1230, 460, 1740, 660, C_FX),
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
    "MaterialExpressionFunctionInput": 13,
    "MaterialExpressionSubtract": 7,
    "MaterialExpressionStep": 5,
    "MaterialExpressionMultiply": 14,
    "MaterialExpressionAdd": 5,
    "MaterialExpressionDivide": 3,
    "MaterialExpressionMax": 1,
    "MaterialExpressionClamp": 1,
    "MaterialExpressionOneMinus": 2,
    "MaterialExpressionLinearInterpolate": 1,
    "MaterialExpressionCosine": 1,
    "MaterialExpressionConstant": 1,
    "MaterialExpressionComponentMask": 1,
    "MaterialExpressionAppendVector": 2,
    "MaterialExpressionMaterialFunctionCall": 3,
    "MaterialExpressionNamedRerouteDeclaration": 9,
    "MaterialExpressionNamedRerouteUsage": 15,
    "MaterialExpressionFunctionOutput": 1,
}
nodes_ok = counts == expected
usages_ok = all(u is not None for u in usages) and len(usages) == 15

print(f"[TEST] Comments created: {sum(comment_ok)}/{len(comment_ok)}")
print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] usages_ok={usages_ok} nodes_ok={nodes_ok}")
print(f"[TEST] Saved: {saved}")
passed = all(results) and usages_ok and nodes_ok and all(comment_ok) and saved
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
