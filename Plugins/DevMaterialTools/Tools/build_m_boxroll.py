"""
M_Transition_BoxRoll を新規作成する (sandbox の build_m_boxroll_pp.py 移植版 / BoxRoll 最終弾)。
右から転がり込んだ箱が下段から積み上がり画面を埋めるトランジション。

sandbox 版からの TransitionFX 規約適合:
  - Time + Hold のループ駆動 → C++ が設定する Progress パラメータ (0..1, イージング済み) 駆動:
    t = Progress * lastLand (mod ループと Hold は不要)
  - BoxColor/BGColor → FadeColor / SceneTexture(PostProcessInput0)
  - mask = 1 - smoothstep(0, aa, d) → smoothstep(aa, 0, d) (Min>Max の反転指定) とし、
    SmoothStep → MF_ApplyInvert(|Invert - Mask|) → lerp の共通チェーンに載せる
  - uv はポストプロセスの TexCoord (ScreenPosition と等価) / R = ViewSize
  - domain=PostProcess, BL_SceneColorAfterTonemapping, 出力 EmissiveColor
座標系は Shadertoy 流: uv = (x*aspect, 1-y)。y-up、1単位 = 画面高さ。
ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage
(複数回使うパラメータ Rows/Speed/DTCol/DTRow も Declaration 化)。

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_m_boxroll.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MAT_PATH = "/TransitionFX/Materials"
MAT_NAME = "M_Transition_BoxRoll"
MF_PATH = "/TransitionFX/MaterialFunctions"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools
tools = unreal.AssetToolsHelpers.get_asset_tools()

mf_row = lib.load_asset(f"{MF_PATH}/MF_RowDist")
mf_inv = lib.load_asset(f"{MF_PATH}/MF_ApplyInvert")
assert mf_row and mf_inv, "dependency MFs missing"

C_UV = unreal.LinearColor(0.2, 0.6, 1.0, 1.0)   # 画面座標系 = 青
C_CO = unreal.LinearColor(1.0, 0.6, 0.1, 1.0)   # パターン座標 = 橙
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑
C_CL = unreal.LinearColor(0.7, 0.4, 1.0, 1.0)   # 色 = 紫
C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)   # マスク = 黄
C_ID = unreal.LinearColor(0.3, 0.9, 0.9, 1.0)   # 行インデックス = 青緑
C_PR = unreal.LinearColor(0.65, 0.65, 0.65, 1.0)  # パラメータ = 灰

results = []
def _connect(frm, out_name, to, in_name):
    ok = mel.connect_material_expressions(frm, out_name, to, in_name)
    if not ok:
        print(f"[TEST] FAILED connect: {frm.get_class().get_name()}('{out_name}') -> "
              f"{to.get_class().get_name()}('{in_name}')")
    results.append(ok)
    return ok

full = f"{MAT_PATH}/{MAT_NAME}"
if lib.does_asset_exist(full):
    lib.delete_asset(full)
mat = tools.create_asset(MAT_NAME, MAT_PATH, unreal.Material,
                         unreal.MaterialFactoryNew())
mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
_bl = getattr(unreal.BlendableLocation, "BL_SCENE_COLOR_AFTER_TONEMAPPING", None)
if _bl is None:
    _bl = unreal.BlendableLocation.BL_AFTER_TONEMAPPING
mat.set_editor_property("blendable_location", _bl)

def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)

decl_map = {}
def make_decl(name, src, x, y, color):
    dcl = expr(unreal.MaterialExpressionNamedRerouteDeclaration, x, y)
    dcl.set_editor_property("name", name)
    dcl.set_editor_property("node_color", color)
    _connect(src, "", dcl, "")
    decl_map[name] = dcl
    return dcl

usages = []
def use(name, x, y):
    u = dmt.create_named_reroute_usage(mat, decl_map[name], x, y)
    usages.append(u)
    return u

def sparam(name, default, x, y):
    p = expr(unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", default)
    return p

# ---- パラメータ (Progress/Invert/FadeColor は C++ ランタイム契約) ----
prog_p  = sparam("Progress", 0.0, -1900, 40)
rows_p  = sparam("Rows",  6.0, -3300, -900)
speed_p = sparam("Speed", 3.5, -3300, -780)
dtc_p   = sparam("DTCol", 0.2, -3300, -660)
dtr_p   = sparam("DTRow", 0.5, -3300, -540)
inv_p   = sparam("Invert", 0.0, 340, -160)
fade_p = expr(unreal.MaterialExpressionVectorParameter, 590, -560)
fade_p.set_editor_property("parameter_name", "FadeColor")
fade_p.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))

make_decl("Rows",  rows_p,  -3130, -900, C_PR)
make_decl("Speed", speed_p, -3130, -780, C_PR)
make_decl("DTCol", dtc_p,   -3130, -660, C_PR)
make_decl("DTRow", dtr_p,   -3130, -540, C_PR)

# ---- view globals: aspect = R.x / R.y, Px = 1 / R.y (iResolution 相当) ----
vs = expr(unreal.MaterialExpressionViewSize, -3300, -140)
vs_x = expr(unreal.MaterialExpressionComponentMask, -3150, -180)
vs_x.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(vs, "", vs_x, "")
vs_y = expr(unreal.MaterialExpressionComponentMask, -3150, -60)
vs_y.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
_connect(vs, "", vs_y, "")

div_asp = expr(unreal.MaterialExpressionDivide, -3000, -160)
_connect(vs_x, "", div_asp, "A")
_connect(vs_y, "", div_asp, "B")
make_decl("aspect", div_asp, -2860, -160, C_UV)

div_px = expr(unreal.MaterialExpressionDivide, -3000, -20)
div_px.set_editor_property("const_a", 1.0)
_connect(vs_y, "", div_px, "B")
make_decl("Px", div_px, -2860, -20, C_UV)

# ---- grid globals: S, HalfDiag, Cols, StartX ----
div_s = expr(unreal.MaterialExpressionDivide, -3000, -900)   # S = 1 / Rows
div_s.set_editor_property("const_a", 1.0)
_connect(use("Rows", -3070, -840), "", div_s, "B")
make_decl("S", div_s, -2850, -900, C_SC)

u_S_hd = use("S", -2700, -1000)
mul_hd = expr(unreal.MaterialExpressionMultiply, -2550, -1000)  # 0.5*S*sqrt(2)
mul_hd.set_editor_property("const_b", 0.70710678)
_connect(u_S_hd, "", mul_hd, "A")
make_decl("HalfDiag", mul_hd, -2400, -1000, C_SC)

u_S_cols = use("S", -2700, -860)
div_cols = expr(unreal.MaterialExpressionDivide, -2550, -880)   # aspect / S
_connect(use("aspect", -2700, -940), "", div_cols, "A")
_connect(u_S_cols, "", div_cols, "B")
ceil_c = expr(unreal.MaterialExpressionCeil, -2410, -880)
_connect(div_cols, "", ceil_c, "")
make_decl("Cols", ceil_c, -2270, -880, C_SC)

u_S_sx = use("S", -2700, -740)
add_sx = expr(unreal.MaterialExpressionAdd, -2550, -760)        # aspect + S
_connect(use("aspect", -2700, -700), "", add_sx, "A")
_connect(u_S_sx, "", add_sx, "B")
make_decl("StartX", add_sx, -2410, -760, C_SC)

# ---- uv: Shadertoy 流座標 (y-up, x in [0, aspect]) ----
tc = expr(unreal.MaterialExpressionTextureCoordinate, -3000, -560)
mask_u = expr(unreal.MaterialExpressionComponentMask, -2850, -600)
mask_u.set_editor_properties({"r": True, "g": False, "b": False, "a": False})
_connect(tc, "", mask_u, "")
mul_ua = expr(unreal.MaterialExpressionMultiply, -2700, -600)   # x * aspect
_connect(mask_u, "", mul_ua, "A")
_connect(use("aspect", -2850, -660), "", mul_ua, "B")

mask_v = expr(unreal.MaterialExpressionComponentMask, -2850, -480)
mask_v.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
_connect(tc, "", mask_v, "")
om_v = expr(unreal.MaterialExpressionOneMinus, -2700, -480)     # 1 - y (y-up)
_connect(mask_v, "", om_v, "")

app_uv = expr(unreal.MaterialExpressionAppendVector, -2550, -540)
_connect(mul_ua, "", app_uv, "A")
_connect(om_v, "", app_uv, "B")
make_decl("uv", app_uv, -2410, -540, C_CO)

# ---- lastLand = (Rows-1)*DTRow + (Cols-1)*DTCol + (StartX-(Cols-0.5)*S)/Speed
sub_r1 = expr(unreal.MaterialExpressionSubtract, -3000, -260)   # Rows - 1
sub_r1.set_editor_property("const_b", 1.0)
_connect(use("Rows", -3130, -240), "", sub_r1, "A")
mul_r = expr(unreal.MaterialExpressionMultiply, -2850, -260)    # * DTRow
_connect(sub_r1, "", mul_r, "A")
_connect(use("DTRow", -3000, -180), "", mul_r, "B")

u_cols1 = use("Cols", -3000, 100)
sub_c1 = expr(unreal.MaterialExpressionSubtract, -2850, 100)    # Cols - 1
sub_c1.set_editor_property("const_b", 1.0)
_connect(u_cols1, "", sub_c1, "A")
mul_c = expr(unreal.MaterialExpressionMultiply, -2700, 100)     # * DTCol
_connect(sub_c1, "", mul_c, "A")
_connect(use("DTCol", -2850, 180), "", mul_c, "B")

u_cols2 = use("Cols", -3000, 240)
sub_ch = expr(unreal.MaterialExpressionSubtract, -2850, 240)    # Cols - 0.5
sub_ch.set_editor_property("const_b", 0.5)
_connect(u_cols2, "", sub_ch, "A")
u_S_ll = use("S", -2850, 320)
mul_chs = expr(unreal.MaterialExpressionMultiply, -2700, 260)   # (Cols-0.5)*S
_connect(sub_ch, "", mul_chs, "A")
_connect(u_S_ll, "", mul_chs, "B")

u_sx_ll = use("StartX", -2700, 380)
sub_sxll = expr(unreal.MaterialExpressionSubtract, -2550, 300)  # StartX - (Cols-0.5)*S
_connect(u_sx_ll, "", sub_sxll, "A")
_connect(mul_chs, "", sub_sxll, "B")
div_sp2 = expr(unreal.MaterialExpressionDivide, -2410, 300)     # / Speed
_connect(sub_sxll, "", div_sp2, "A")
_connect(use("Speed", -2550, 440), "", div_sp2, "B")

add_rc = expr(unreal.MaterialExpressionAdd, -2550, 40)
_connect(mul_r, "", add_rc, "A")
_connect(mul_c, "", add_rc, "B")
add_ll = expr(unreal.MaterialExpressionAdd, -2270, 160)
_connect(add_rc, "", add_ll, "A")
_connect(div_sp2, "", add_ll, "B")
make_decl("lastLand", add_ll, -2130, 160, C_SC)

# ---- t = Progress * lastLand (Time+mod ループの代わりに Progress 駆動) ----
u_ll = use("lastLand", -1900, 160)
mul_t = expr(unreal.MaterialExpressionMultiply, -1700, 100)
_connect(prog_p, "", mul_t, "A")
_connect(u_ll, "", mul_t, "B")
make_decl("t", mul_t, -1560, 100, C_SC)

# ---- rowHere = floor(uv.y / S) ----
u_uv_rh = use("uv", -1990, -420)
mask_uvy = expr(unreal.MaterialExpressionComponentMask, -1850, -420)
mask_uvy.set_editor_properties({"r": False, "g": True, "b": False, "a": False})
_connect(u_uv_rh, "", mask_uvy, "")
u_S_rh = use("S", -1850, -340)
div_rh = expr(unreal.MaterialExpressionDivide, -1700, -400)
_connect(mask_uvy, "", div_rh, "A")
_connect(u_S_rh, "", div_rh, "B")
floor_rh = expr(unreal.MaterialExpressionFloor, -1560, -400)
_connect(div_rh, "", floor_rh, "")
make_decl("rowHere", floor_rh, -1420, -400, C_ID)

# ---- 3x MF_RowDist (rowHere, rowHere-1, rowHere+1) ----
def rowdist_call(x, y, j_node):
    call = expr(unreal.MaterialExpressionMaterialFunctionCall, x, y)
    call.set_editor_property("material_function", mf_row)
    _connect(use("uv", x - 160, y - 40), "", call, "UV")
    _connect(j_node, "", call, "J")
    _connect(use("t", x - 160, y + 40), "", call, "T")
    _connect(use("S", x - 160, y + 120), "", call, "S")
    _connect(use("Rows", x - 320, y + 160), "", call, "Rows")
    _connect(use("Cols", x - 160, y + 200), "", call, "Cols")
    _connect(use("DTCol", x - 320, y + 240), "", call, "DTCol")
    _connect(use("DTRow", x - 320, y + 320), "", call, "DTRow")
    _connect(use("Speed", x - 320, y + 400), "", call, "Speed")
    _connect(use("StartX", x - 160, y + 280), "", call, "StartX")
    _connect(use("HalfDiag", x - 160, y + 360), "", call, "HalfDiag")
    _connect(use("Px", x - 320, y + 480), "", call, "Px")
    return call

u_rh1 = use("rowHere", -1250, -680)
call_here = rowdist_call(-950, -640, u_rh1)

u_rh2 = use("rowHere", -1250, -120)
sub_j = expr(unreal.MaterialExpressionSubtract, -1130, -120)    # rowHere - 1
sub_j.set_editor_property("const_b", 1.0)
_connect(u_rh2, "", sub_j, "A")
call_below = rowdist_call(-950, -80, sub_j)

u_rh3 = use("rowHere", -1250, 440)
add_j = expr(unreal.MaterialExpressionAdd, -1130, 440)          # rowHere + 1
add_j.set_editor_property("const_b", 1.0)
_connect(u_rh3, "", add_j, "A")
call_above = rowdist_call(-950, 480, add_j)

min1 = expr(unreal.MaterialExpressionMin, -670, -400)
_connect(call_here, "", min1, "A")
_connect(call_below, "", min1, "B")
min2 = expr(unreal.MaterialExpressionMin, -530, -300)
_connect(min1, "", min2, "A")
_connect(call_above, "", min2, "B")
make_decl("d", min2, -390, -300, C_FX)

# ---- aa = 1.5 * Px (解析的 AA 幅) ----
mul_aa = expr(unreal.MaterialExpressionMultiply, -670, -120)
mul_aa.set_editor_property("const_b", 1.5)
_connect(use("Px", -810, -120), "", mul_aa, "A")
make_decl("aa", mul_aa, -530, -120, C_FX)

# ---- mask = smoothstep(aa, 0, d): Min>Max の反転指定で d<=0 (箱の内側) が 1 ----
u_d = use("d", -250, -300)
u_aa = use("aa", -250, -200)
ss = expr(unreal.MaterialExpressionSmoothStep, -100, -280)
ss.set_editor_property("const_max", 0.0)
_connect(u_aa, "", ss, "Min")
_connect(u_d, "", ss, "Value")

# ---- alpha = ApplyInvert(mask, Invert) = |Invert - mask| ----
inv_call = expr(unreal.MaterialExpressionMaterialFunctionCall, 100, -260)
inv_call.set_editor_property("material_function", mf_inv)
_connect(ss, "", inv_call, "Mask")
_connect(inv_p, "", inv_call, "Invert")
make_decl("alpha", inv_call, 300, -260, C_FX)

# ---- col = SceneTexture(PostProcessInput0).rgb ----
st = expr(unreal.MaterialExpressionSceneTexture, 590, -760)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
rgb = expr(unreal.MaterialExpressionComponentMask, 770, -760)
rgb.set_editor_properties({"r": True, "g": True, "b": True, "a": False})
_connect(st, "", rgb, "")
make_decl("col", rgb, 920, -760, C_CL)

# ---- EmissiveColor = lerp(col, FadeColor.rgb, alpha) ----
fade_rgb = expr(unreal.MaterialExpressionComponentMask, 770, -560)
fade_rgb.set_editor_properties({"r": True, "g": True, "b": True, "a": False})
_connect(fade_p, "", fade_rgb, "")
col_u = use("col", 1090, -660)
al_u = use("alpha", 1090, -460)
lerp_c = expr(unreal.MaterialExpressionLinearInterpolate, 1240, -610)
_connect(col_u, "", lerp_c, "A")
_connect(fade_rgb, "", lerp_c, "B")
_connect(al_u, "", lerp_c, "Alpha")
ok_out = mel.connect_material_property(lerp_c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
results.append(ok_out)

# ---- コメント ----
COMMENTS = [
    ("view globals (iResolution 相当): aspect = R.x/R.y, Px = 1/R.y。解像度に自動追従",
     -3350, -240, 660, 360, C_UV),
    ("grid globals: S = 1/Rows, HalfDiag = 0.5*S*sqrt(2), Cols = ceil(aspect/S), "
     "StartX = aspect + S (画面外スポーン)",
     -3050, -1100, 920, 480, C_SC),
    ("uv = (x*aspect, 1-y): Shadertoy 流 y-up 座標。1単位 = 画面高さ",
     -3050, -660, 780, 300, C_CO),
    ("lastLand = (Rows-1)*DTRow + (Cols-1)*DTCol + (StartX-(Cols-0.5)*S)/Speed: "
     "最後の箱 (左上) が着地する時刻。t = Progress * lastLand で 0..1 の Progress を"
     "シミュレーション時間へ写像 (Time ループの代わり)",
     -3050, -20, 1620, 620, C_SC),
    ("転がる箱は隣の行にはみ出すため rowHere-1..rowHere+1 の3行を評価 (ループ展開): "
     "d = min of 3x MF_RowDist",
     -1300, -740, 940, 1620, C_ID),
    ("mask = smoothstep(1.5*Px, 0, d) (解析的 AA)。alpha = ApplyInvert(mask) = |Invert - mask|",
     -310, -400, 760, 340, C_FX),
    ("out = lerp(scene.rgb, FadeColor.rgb, alpha) -> emissive",
     540, -860, 900, 480, C_CL),
]
comment_ok = []
for text, x, y, w, h, color in COMMENTS:
    c = dmt.create_comment_in_material(mat, text, x, y, w, h, color)
    comment_ok.append(c is not None)
print(f"[TEST] Comments created: {sum(comment_ok)}/{len(comment_ok)}")

# ---- 検証 ----
decls, usage_count, class_count = [], {}, {}
for e in mel.get_material_expressions(mat):
    cn = e.get_class().get_name()
    class_count[cn] = class_count.get(cn, 0) + 1
    if isinstance(e, unreal.MaterialExpressionNamedRerouteDeclaration):
        decls.append(str(e.get_editor_property("name")))
    elif isinstance(e, unreal.MaterialExpressionNamedRerouteUsage):
        nm = dmt.get_named_reroute_usage_display_name(e)
        usage_count[nm] = usage_count.get(nm, 0) + 1
print(f"[TEST] node classes: {dict(sorted(class_count.items()))}")

expected_decls = sorted(["Rows", "Speed", "DTCol", "DTRow", "aspect", "Px", "S",
                         "HalfDiag", "Cols", "StartX", "uv", "lastLand", "t",
                         "rowHere", "d", "aa", "alpha", "col"])
expected_usages = {"Rows": 5, "Speed": 4, "DTCol": 4, "DTRow": 4, "aspect": 3,
                   "Px": 4, "S": 8, "HalfDiag": 3, "Cols": 5, "StartX": 4,
                   "uv": 4, "lastLand": 1, "t": 3, "rowHere": 3, "d": 1,
                   "aa": 1, "alpha": 1, "col": 1}
n_exprs = len(mel.get_material_expressions(mat))
expected_exprs = 128  # 7 params + 3 sources + 41 ops + 18 decls + 59 usages
usages_ok = all(u is not None for u in usages) and len(usages) == 59
domain_ok = mat.get_editor_property("material_domain") == unreal.MaterialDomain.MD_POST_PROCESS
print(f"[TEST] Declarations ({len(decls)}): {sorted(decls)}")
print(f"[TEST] Usage counts: {dict(sorted(usage_count.items()))}")
print(f"[TEST] expression count: {n_exprs} (expected {expected_exprs})")
print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")
print(f"[TEST] blendable_location={mat.get_editor_property('blendable_location')}")

mel.recompile_material(mat)
saved = lib.save_asset(full)
print(f"[TEST] Saved: {saved}")

passed = (all(results) and sorted(decls) == expected_decls
          and usage_count == expected_usages and usages_ok and all(comment_ok)
          and saved and domain_ok and n_exprs == expected_exprs)
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
