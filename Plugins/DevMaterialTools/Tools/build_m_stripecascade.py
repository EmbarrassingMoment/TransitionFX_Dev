"""
M_Transition_StripeCascade を Named Reroute 注釈付きで in-place 再構築する。
(sandbox の build_m_stripetransition.py 移植版)

TransitionFX 規約への適合:
  - Time/cycle 駆動 → C++ が設定する Progress パラメータ (0..1, イージング済み) 駆動
  - フェードイン/アウトの2相 → 単相。反転は MF_ApplyInvert(|Invert - Mask|) に委譲
  - 出力 = lerp(SceneTexture.rgb, FadeColor.rgb, alpha) -> EmissiveColor
    (全マスター共通の Lerp→Emissive チェーン / Progress・Invert・FadeColor 三点セット)
  - uv はポストプロセスの TexCoord をそのまま使用 (sandbox の p/toUV 往復は不要)
  - AA は Smoothness 定数の代わりに進行軸方向の解析的 1px (= 1/ViewSize.axis)。
    fwidth(pos) はストライプ境界の微分スパイクが出るため解析値を使う
  - パラメータ名は既存 MI 互換: Direction / SplitCount / StripeDelay

fix(getFixTransition 相当) は呼び出しが1回だけなので MF 化せずインライン:
  accel = 1 + StripeDelay * SplitCount * max(1, SplitCount - 1)
  fix   = Progress / SplitCount * accel
ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage。

実行例:
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="Plugins/DevMaterialTools/Tools/build_m_stripecascade.py" -EnablePlugins=PythonScriptPlugin
"""
import unreal

MAT_PATH = "/TransitionFX/Materials/M_Transition_StripeCascade"
MF_POS_PATH = "/TransitionFX/MaterialFunctions/MF_StripePos"
MF_INV_PATH = "/TransitionFX/MaterialFunctions/MF_ApplyInvert"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools

C_UV = unreal.LinearColor(0.2, 0.6, 1.0, 1.0)   # 画面座標系 = 青
C_CO = unreal.LinearColor(1.0, 0.6, 0.1, 1.0)   # パターン座標 = 橙
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑
C_CL = unreal.LinearColor(0.7, 0.4, 1.0, 1.0)   # 色 = 紫
C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)   # マスク = 黄

fn_pos = lib.load_asset(MF_POS_PATH)
fn_inv = lib.load_asset(MF_INV_PATH)
mat = lib.load_asset(MAT_PATH)
assert fn_pos and fn_inv and mat, "required assets not found"

results = []
def _connect(frm, out_name, to, in_name):
    ok = mel.connect_material_expressions(frm, out_name, to, in_name)
    if not ok:
        print(f"[TEST] FAILED connect: {frm.get_class().get_name()}('{out_name}') -> "
              f"{to.get_class().get_name()}('{in_name}')")
    results.append(ok)
    return ok

n_cleared_comments = dmt.clear_comments_in_material(mat)
mel.delete_all_material_expressions(mat)
print(f"[TEST] cleared existing graph (comments removed: {n_cleared_comments})")

def expr(cls, x, y):
    return mel.create_material_expression(mat, cls, x, y)

decl_map = {}
def make_decl(name, src, x, y, color, out_name=""):
    dcl = expr(unreal.MaterialExpressionNamedRerouteDeclaration, x, y)
    dcl.set_editor_property("name", name)
    dcl.set_editor_property("node_color", color)
    _connect(src, out_name, dcl, "")
    decl_map[name] = dcl
    return dcl

def use(name, x, y):
    return dmt.create_named_reroute_usage(mat, decl_map[name], x, y)

def sparam(name, default, x, y):
    p = expr(unreal.MaterialExpressionScalarParameter, x, y)
    p.set_editor_property("parameter_name", name)
    p.set_editor_property("default_value", default)
    return p

# ---- パラメータ (名前は既存 MI / C++ ランタイム互換) ----
prog_p  = sparam("Progress", 0.0, -2900, 550)
dir_p   = sparam("Direction", 0.0, -2900, 0)
split_p = sparam("SplitCount", 8.0, -2900, 150)
delay_p = sparam("StripeDelay", 0.05, -2900, 300)
inv_p   = sparam("Invert", 0.0, -1500, 500)
fade_p = expr(unreal.MaterialExpressionVectorParameter, -1100, -50)
fade_p.set_editor_property("parameter_name", "FadeColor")
fade_p.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 0.0, 1.0))

# ---- uv = TexCoord (viewport UV) / R = ViewSize ----
tc = expr(unreal.MaterialExpressionTextureCoordinate, -2900, -200)
make_decl("uv", tc, -2750, -200, C_CO)
vs = expr(unreal.MaterialExpressionViewSize, -2900, 900)
make_decl("R", vs, -2750, 900, C_UV)

# ---- pos = MF_StripePos(uv, Direction, SplitCount, StripeDelay) ----
uv_u = use("uv", -2500, -50)
pos_call = expr(unreal.MaterialExpressionMaterialFunctionCall, -2300, 0)
pos_call.set_editor_property("material_function", fn_pos)
_connect(uv_u, "", pos_call, "UV")
_connect(dir_p, "", pos_call, "Direction")
_connect(split_p, "", pos_call, "SplitCount")
_connect(delay_p, "", pos_call, "Delay")
make_decl("pos", pos_call, -2100, 0, C_CO)

# ---- accel = 1 + StripeDelay * SplitCount * max(1, SplitCount - 1) ----
sub_s = expr(unreal.MaterialExpressionSubtract, -2500, 350)
sub_s.set_editor_property("const_b", 1.0)
_connect(split_p, "", sub_s, "A")
max_s = expr(unreal.MaterialExpressionMax, -2350, 350)
max_s.set_editor_property("const_a", 1.0)
_connect(sub_s, "", max_s, "B")
mul_ds = expr(unreal.MaterialExpressionMultiply, -2350, 220)
_connect(delay_p, "", mul_ds, "A"); _connect(split_p, "", mul_ds, "B")
mul_a = expr(unreal.MaterialExpressionMultiply, -2200, 280)
_connect(mul_ds, "", mul_a, "A"); _connect(max_s, "", mul_a, "B")
acc = expr(unreal.MaterialExpressionAdd, -2050, 280)
acc.set_editor_property("const_a", 1.0)
_connect(mul_a, "", acc, "B")
make_decl("accel", acc, -1900, 280, C_SC)

# ---- fix = Progress / SplitCount * accel ----
div_t = expr(unreal.MaterialExpressionDivide, -2500, 550)
_connect(prog_p, "", div_t, "A"); _connect(split_p, "", div_t, "B")
acc_u = use("accel", -2350, 620)
mul_f = expr(unreal.MaterialExpressionMultiply, -2200, 550)
_connect(div_t, "", mul_f, "A"); _connect(acc_u, "", mul_f, "B")
make_decl("fix", mul_f, -2050, 550, C_SC)

# ---- px = 進行軸方向の 1px = 1 / lerp(R.x, R.y, step(1.5, Direction)) ----
r_u = use("R", -2500, 900)
rx_aa = expr(unreal.MaterialExpressionComponentMask, -2350, 860)
rx_aa.set_editor_property("r", True); rx_aa.set_editor_property("g", False)
rx_aa.set_editor_property("b", False); rx_aa.set_editor_property("a", False)
_connect(r_u, "", rx_aa, "")
ry_aa = expr(unreal.MaterialExpressionComponentMask, -2350, 940)
ry_aa.set_editor_property("r", False); ry_aa.set_editor_property("g", True)
ry_aa.set_editor_property("b", False); ry_aa.set_editor_property("a", False)
_connect(r_u, "", ry_aa, "")
is_y_aa = expr(unreal.MaterialExpressionStep, -2350, 1030)
is_y_aa.set_editor_property("const_y", 1.5)
_connect(dir_p, "", is_y_aa, "X")
lerp_axis = expr(unreal.MaterialExpressionLinearInterpolate, -2200, 930)
_connect(rx_aa, "", lerp_axis, "A")
_connect(ry_aa, "", lerp_axis, "B")
_connect(is_y_aa, "", lerp_axis, "Alpha")
px_div = expr(unreal.MaterialExpressionDivide, -2050, 930)
px_div.set_editor_property("const_a", 1.0)
_connect(lerp_axis, "", px_div, "B")
make_decl("px", px_div, -1900, 930, C_UV)

# ---- mask: smoothstep(fix + px, fix - px, pos)  (Min > Max の反転指定: pos < fix で 1) ----
fix_u1 = use("fix", -1750, 450)
px_u1 = use("px", -1750, 550)
add_e = expr(unreal.MaterialExpressionAdd, -1600, 500)
_connect(fix_u1, "", add_e, "A"); _connect(px_u1, "", add_e, "B")
fix_u2 = use("fix", -1750, 650)
px_u2 = use("px", -1750, 750)
sub_e = expr(unreal.MaterialExpressionSubtract, -1600, 700)
_connect(fix_u2, "", sub_e, "A"); _connect(px_u2, "", sub_e, "B")
pos_u = use("pos", -1600, 380)
ss = expr(unreal.MaterialExpressionSmoothStep, -1450, 500)
_connect(add_e, "", ss, "Min")
_connect(sub_e, "", ss, "Max")
_connect(pos_u, "", ss, "Value")

# ---- alpha = ApplyInvert(mask, Invert) = |Invert - mask| ----
inv_call = expr(unreal.MaterialExpressionMaterialFunctionCall, -1250, 500)
inv_call.set_editor_property("material_function", fn_inv)
_connect(ss, "", inv_call, "Mask")
_connect(inv_p, "", inv_call, "Invert")
make_decl("alpha", inv_call, -1050, 500, C_FX)

# ---- col = SceneTexture(PostProcessInput0).rgb ----
st = expr(unreal.MaterialExpressionSceneTexture, -1100, -250)
st.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
rgb = expr(unreal.MaterialExpressionComponentMask, -920, -250)
rgb.set_editor_property("r", True); rgb.set_editor_property("g", True)
rgb.set_editor_property("b", True); rgb.set_editor_property("a", False)
_connect(st, "", rgb, "")
make_decl("col", rgb, -770, -250, C_CL)

# ---- EmissiveColor = lerp(col, FadeColor.rgb, alpha) ----
fade_rgb = expr(unreal.MaterialExpressionComponentMask, -920, -50)
fade_rgb.set_editor_property("r", True); fade_rgb.set_editor_property("g", True)
fade_rgb.set_editor_property("b", True); fade_rgb.set_editor_property("a", False)
_connect(fade_p, "", fade_rgb, "")
col_u = use("col", -600, -150)
al_u = use("alpha", -600, 50)
lerp_c = expr(unreal.MaterialExpressionLinearInterpolate, -450, -100)
_connect(col_u, "", lerp_c, "A")
_connect(fade_rgb, "", lerp_c, "B")
_connect(al_u, "", lerp_c, "Alpha")
ok_out = mel.connect_material_property(lerp_c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
results.append(ok_out)

# ---- コメント(短文) ----
COMMENTS = [
    ("uv = viewport UV / R = ViewSize", -2970, -300, 500, 320, C_UV),
    ("stripe params (MI-compatible names)", -2970, -80, 260, 540, C_CO),
    ("pos = StripePos(uv): per-stripe local pos + stripe index * StripeDelay", -2560, -110, 660, 260, C_CO),
    ("accel = 1 + StripeDelay*SplitCount*max(1, SplitCount-1): last stripe closes at Progress=1",
     -2560, 170, 810, 280, C_SC),
    ("fix = Progress/SplitCount * accel: per-stripe close threshold", -2560, 470, 660, 240, C_SC),
    ("px = 1px along transition axis (analytic AA width; fwidth(pos) would spike at stripe seams)",
     -2560, 800, 810, 330, C_UV),
    ("mask = AA step(pos < fix), alpha = ApplyInvert(mask) = |Invert - mask|", -1810, 330, 910, 480, C_FX),
    ("out = lerp(scene.rgb, FadeColor.rgb, alpha) -> emissive", -1160, -350, 860, 480, C_CL),
]
comment_ok = []
for text, x, y, w, h, color in COMMENTS:
    c = dmt.create_comment_in_material(mat, text, x, y, w, h, color)
    comment_ok.append(c is not None)
print(f"[TEST] Comments created: {sum(comment_ok)}/{len(comment_ok)}")

# ---- 検証 ----
decls, usage_count = [], {}
for e in mel.get_material_expressions(mat):
    if isinstance(e, unreal.MaterialExpressionNamedRerouteDeclaration):
        decls.append(str(e.get_editor_property("name")))
    elif isinstance(e, unreal.MaterialExpressionNamedRerouteUsage):
        nm = dmt.get_named_reroute_usage_display_name(e)
        usage_count[nm] = usage_count.get(nm, 0) + 1

expected_decls = sorted(["uv", "R", "pos", "accel", "fix", "px", "alpha", "col"])
expected_usages = {"uv": 1, "R": 1, "pos": 1, "accel": 1, "fix": 2, "px": 2, "alpha": 1, "col": 1}
print(f"[TEST] Declarations ({len(decls)}): {sorted(decls)}")
print(f"[TEST] Usage counts: {dict(sorted(usage_count.items()))}")
print(f"[TEST] Connections: {sum(results)}/{len(results)} succeeded")

mel.recompile_material(mat)
saved = lib.save_asset(MAT_PATH)
print(f"[TEST] Saved: {saved}")

passed = (all(results) and sorted(decls) == expected_decls
          and usage_count == expected_usages and all(comment_ok) and saved)
print("[TEST] RESULT: " + ("PASS" if passed else "FAIL"))
