"""
M_Transition_RippleRings / M_Transition_OrbitRings を Named Reroute 注釈付きで構築する (in-place 再構築可)。

プラグインのコンテンツなので UE 5.5 のワークスペースで実行すること (CLAUDE.md "Plugin content is saved with UE 5.5")。
数式の原型は Saved/TransitionIdeas/transition_ideas.py (numpy プロトタイプ) の ripple_rings / orbit_rings。

共通:
  p     = MF_CorrectAspectRatio(TexCoord)            画面中心原点・高さ 1 のアスペクト補正座標
  maxR  = MF_GetMaxScreenRadius()                    中心から画面の角までの距離
  ringPos = length(p) / maxR * RingCount             角でちょうど RingCount になるリング座標
  px    = |ddy(p.y)|                                 p 単位の 1px (ViewSize 不要 = UI ドメインでも同じ式)
  出力  = lerp(SceneTexture.rgb, FadeColor, MF_ApplyInvert(mask, Invert)) -> EmissiveColor
  どちらも Progress=0 で mask=0、Progress=1 で mask=1 (リングの遅延は saturate(index / RingCount))。

RippleRings (同心円のリングが中心から順に太ってつながる):
  リングの中心線は ringPos が整数の位置。各ピクセルは下側のリング A (index i = floor(ringPos)) と
  上側のリング B (i + 1) の両方を評価して max を取る。リングは隣との中間線を越えても太り続けるので
  リング同士の境目に不連続が無く、全てのエッジを smoothstep で AA できる。
    dist  = 中心線までの距離 (1 = リング間隔の半分)        distA = frac(ringPos) * 2, distB = 2 - distA
    w     = max(2px, Smoothness)                          エッジ幅 (dist 単位)
    prog  = Progress * (1 + Stagger + w)
    edge  = prog - Stagger * saturate(index / RingCount)  そのリングの現在の半幅
    mask  = max over A,B of smoothstep(edge, edge - w, dist)

OrbitRings (リングごとに時計スイープ。回転方向は交互、開始角は黄金比ずつずれる):
  リング k は ringPos が [k, k+1) の帯 (中心線 k + 0.5)。各ピクセルは最寄りの 2 本
  (ringBase = floor(ringPos - 0.5) として A = max(ringBase, 0), B = ringBase + 1) を評価して max を取る。
  完了したリングの縁は境目を wb だけ越えるので、隣のリングがまだ開いていても縁が AA される。
    local = saturate(Progress * (1 + Stagger) - Stagger * saturate(k / RingCount))
    half  = local * (0.5 + wa)                            扇の半角 (turn)。wa = 角度方向の 2px
    dc    = |frac(ang * dir - k * 0.618034 - half + 0.5) - 0.5|   扇の中心からの角距離 (両端が同じ式で AA される)
    sweep = smoothstep(half, half - wa, dc)
    edge  = 1 + wb - Gap * (1 - local)                    帯の半幅 (dist 単位)。スイープ完了で隙間が閉じる
    band  = smoothstep(edge, edge - wb, dist)
    mask  = max over A,B of sweep * band
ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage。

実行例 (UE 5.5 ワークスペース):
  UnrealEditor-Cmd.exe T55W.uproject -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/build_m_rings.py" -EnablePlugins=PythonScriptPlugin
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tfx_material_graph import Graph, E, C_CO, C_FX, C_SC, C_UV  # noqa: E402


def build_ripple_rings():
    g = Graph("M_Transition_RippleRings")
    rings_p = g.sparam("RingCount", 8.0, -3600, 300)
    stag_p = g.sparam("Stagger", 1.4, -3600, 1500)
    prog_p = g.sparam("Progress", 0.5, -3600, 1380)
    g.comment("ring params: RingCount rings from centre to corner / Stagger = delay of the outermost ring "
              "(in units of one ring's own growth time)", -3670, 200, 330, 1500, C_CO)

    g.screen_coords(-3600, -300)

    # ---- ringPos = length(p) / maxR * RingCount ----
    length = g.unary(E.MaterialExpressionLength, -2950, 300, g.use("p", -3100, 300))
    norm = g.binop(E.MaterialExpressionDivide, -2800, 300, length, g.use("maxR", -2950, 400))
    g.decl("ringPos", g.binop(E.MaterialExpressionMultiply, -2650, 300, norm, rings_p), -2500, 300, C_CO)
    g.comment("ringPos = length(p) / maxR * RingCount (ring centre lines at integers)", -3160, 200, 900, 300, C_CO)

    # ---- ringIdx = floor(ringPos) / distA = frac(ringPos) * 2 / distB = 2 - distA ----
    g.decl("ringIdx", g.unary(E.MaterialExpressionFloor, -2950, 620, g.use("ringPos", -3100, 620)),
           -2800, 620, C_CO)
    fr = g.unary(E.MaterialExpressionFrac, -2950, 740, g.use("ringPos", -3100, 740))
    g.decl("distA", g.binop(E.MaterialExpressionMultiply, -2800, 740, fr, 2.0), -2650, 740, C_CO)
    g.decl("distB", g.binop(E.MaterialExpressionSubtract, -2300, 740, 2.0, g.use("distA", -2450, 740)),
           -2150, 740, C_CO)
    g.comment("ring A = floor(ringPos), ring B = A + 1; dist = distance to that ring's centre line "
              "(1 = half the ring spacing)", -3160, 530, 1300, 330, C_CO)

    # ---- w = max(2px, Smoothness) in dist units: 2px = px * 4 * RingCount / maxR ----
    px4 = g.binop(E.MaterialExpressionMultiply, -2950, 1000, g.use("px", -3100, 1000), 4.0)
    px_n = g.binop(E.MaterialExpressionMultiply, -2800, 1000, px4, rings_p)
    px_d = g.binop(E.MaterialExpressionDivide, -2650, 1000, px_n, g.use("maxR", -2800, 1100))
    smooth_p = g.sparam("Smoothness", 0.0, -2800, 1190)
    g.decl("w", g.binop(E.MaterialExpressionMax, -2500, 1000, px_d, smooth_p), -2350, 1000, C_SC)
    g.comment("w = max(2px, Smoothness): edge width in dist units (Smoothness 0 = AA only)",
              -3160, 910, 1000, 400, C_SC)

    # ---- prog = Progress * (1 + Stagger + w) ----
    one_s = g.binop(E.MaterialExpressionAdd, -3250, 1500, stag_p, 1.0)
    span = g.binop(E.MaterialExpressionAdd, -2950, 1500, one_s, g.use("w", -3100, 1600))
    g.decl("prog", g.binop(E.MaterialExpressionMultiply, -2800, 1420, prog_p, span), -2650, 1420, C_SC)
    g.comment("prog = Progress * (1 + Stagger + w): +w keeps Progress 0/1 fully open/closed",
              -3310, 1330, 850, 370, C_SC)

    # ---- edge = prog - Stagger * saturate(index / RingCount): リングの現在の半幅 ----
    def edge(tag, y, index_offset):
        idx = g.use("ringIdx", -2250, y)
        if index_offset:
            idx = g.binop(E.MaterialExpressionAdd, -2100, y, idx, float(index_offset))
        delay = g.unary(E.MaterialExpressionSaturate, -1800, y,
                        g.binop(E.MaterialExpressionDivide, -1950, y, idx, rings_p))
        lag = g.binop(E.MaterialExpressionMultiply, -1650, y, delay, stag_p)
        g.decl(f"edge{tag}", g.binop(E.MaterialExpressionSubtract, -1500, y, g.use("prog", -1650, y - 90), lag),
               -1350, y, C_SC)

    edge("A", 1420, 0)
    edge("B", 1650, 1)
    g.comment("edge = prog - Stagger * saturate(index / RingCount): the ring's current half-width; "
              "outer rings start later", -2310, 1240, 1160, 560, C_SC)

    # ---- mask = max(smoothstep(edgeA, edgeA - w, distA), smoothstep(edgeB, edgeB - w, distB)) ----
    def ring_mask(tag, y):
        inner = g.binop(E.MaterialExpressionSubtract, -750, y + 60,
                        g.use(f"edge{tag}", -900, y + 30), g.use("w", -900, y + 110))
        return g.smoothstep(-600, y, g.use(f"edge{tag}", -900, y - 50), inner, g.use(f"dist{tag}", -900, y + 190))

    union = g.binop(E.MaterialExpressionMax, -400, 1550, ring_mask("A", 1420), ring_mask("B", 1750))
    g.comment("mask = max over rings A/B of smoothstep(edge, edge - w, dist) (Min > Max: 1 inside the ring)",
              -960, 1270, 760, 760, C_FX)

    g.finish(union, -200, 1550)
    return g.verify_and_save(["Progress", "Invert", "FadeColor", "RingCount", "Stagger", "Smoothness"])


def build_orbit_rings():
    g = Graph("M_Transition_OrbitRings")
    rings_p = g.sparam("RingCount", 6.0, -3600, 300)
    stag_p = g.sparam("Stagger", 1.0, -3600, 1750)
    prog_p = g.sparam("Progress", 0.5, -3600, 1630)
    gap_p = g.sparam("Gap", 0.35, -3600, 2300)
    g.comment("ring params: RingCount rings from centre to corner / Stagger = delay of the outermost ring / "
              "Gap = gap between rings while they sweep (0..1 of the ring width)", -3670, 200, 330, 2300, C_CO)

    g.screen_coords(-3600, -300)

    # ---- r = length(p) / ringPos = r / maxR * RingCount ----
    g.decl("r", g.unary(E.MaterialExpressionLength, -2950, 300, g.use("p", -3100, 300)), -2800, 300, C_UV)
    norm = g.binop(E.MaterialExpressionDivide, -2450, 300, g.use("r", -2600, 300), g.use("maxR", -2600, 400))
    g.decl("ringPos", g.binop(E.MaterialExpressionMultiply, -2300, 300, norm, rings_p), -2150, 300, C_CO)
    g.comment("r = length(p) / ringPos = r / maxR * RingCount (ring k spans [k, k+1))", -3160, 200, 1260, 300, C_CO)

    # ---- ang = atan2(p.y, p.x) / 2pi (turn、画面上で時計回りに増える) ----
    p_use = g.use("p", -3100, 650)
    atan = g.expr(E.MaterialExpressionArctangent2, -2750, 650)
    g.connect(g.channel(-2950, 600, p_use, "g"), "", atan, "Y")
    g.connect(g.channel(-2950, 700, p_use, "r"), "", atan, "X")
    g.decl("ang", g.binop(E.MaterialExpressionDivide, -2600, 650, atan, 6.283185307), -2450, 650, C_UV)
    g.comment("ang = atan2(p.y, p.x) / 2pi: angle in turns, grows clockwise on screen", -3160, 540, 960, 280, C_UV)

    # ---- wa = min(px / max(r * pi, eps), 0.25): 角度方向の 2px (turn) ----
    r_pi = g.binop(E.MaterialExpressionMultiply, -2950, 950, g.use("r", -3100, 950), 3.14159265)
    r_safe = g.binop(E.MaterialExpressionMax, -2800, 950, r_pi, 0.00001)
    wa_raw = g.binop(E.MaterialExpressionDivide, -2650, 950, g.use("px", -2800, 870), r_safe)
    g.decl("wa", g.binop(E.MaterialExpressionMin, -2500, 950, wa_raw, 0.25), -2350, 950, C_SC)
    # ---- wb = px * 4 * RingCount / maxR: 半径方向の 2px (dist 単位) ----
    px4 = g.binop(E.MaterialExpressionMultiply, -2950, 1200, g.use("px", -3100, 1200), 4.0)
    px_n = g.binop(E.MaterialExpressionMultiply, -2800, 1200, px4, rings_p)
    g.decl("wb", g.binop(E.MaterialExpressionDivide, -2650, 1200, px_n, g.use("maxR", -2800, 1300)),
           -2500, 1200, C_SC)
    g.comment("edge widths (2px): wa along the angle in turns (clamped near the centre), "
              "wb across the ring in dist units", -3160, 850, 1000, 550, C_SC)

    # ---- prog = Progress * (1 + Stagger) ----
    one_s = g.binop(E.MaterialExpressionAdd, -3250, 1750, stag_p, 1.0)
    g.decl("prog", g.binop(E.MaterialExpressionMultiply, -3100, 1670, prog_p, one_s), -2950, 1670, C_SC)
    g.comment("prog = Progress * (1 + Stagger)", -3310, 1580, 560, 300, C_SC)

    # ---- ringBase = floor(ringPos - 0.5) / ringA = max(ringBase, 0) / ringB = ringBase + 1 ----
    shifted = g.binop(E.MaterialExpressionSubtract, -2450, 1670, g.use("ringPos", -2600, 1670), 0.5)
    g.decl("ringBase", g.unary(E.MaterialExpressionFloor, -2300, 1670, shifted), -2150, 1670, C_CO)
    g.decl("ringA", g.binop(E.MaterialExpressionMax, -1750, 1620, g.use("ringBase", -1900, 1620), 0.0),
           -1600, 1620, C_CO)
    g.decl("ringB", g.binop(E.MaterialExpressionAdd, -1750, 1740, g.use("ringBase", -1900, 1740), 1.0),
           -1600, 1740, C_CO)
    g.comment("the two rings nearest to this pixel: A = max(floor(ringPos - 0.5), 0), B = floor(ringPos - 0.5) + 1",
              -2660, 1580, 1260, 300, C_CO)

    def ring(tag, y):
        """リング 1 本分: mask = sweep * band。y を起点に下方向へ 4 行使う。"""
        k = f"ring{tag}"
        # dist = |ringPos - k - 0.5| * 2: リング中心線までの距離 (1 = 帯の端)
        centre = g.binop(E.MaterialExpressionAdd, -2950, y, g.use(k, -3100, y), 0.5)
        off = g.binop(E.MaterialExpressionSubtract, -2800, y, g.use("ringPos", -2950, y - 90), centre)
        dist2 = g.binop(E.MaterialExpressionMultiply, -2500, y, g.unary(E.MaterialExpressionAbs, -2650, y, off), 2.0)
        g.decl(f"dist{tag}", dist2, -2350, y, C_CO)
        # local = saturate(prog - Stagger * saturate(k / RingCount))
        delay = g.unary(E.MaterialExpressionSaturate, -2800, y + 200,
                        g.binop(E.MaterialExpressionDivide, -2950, y + 200, g.use(k, -3100, y + 200), rings_p))
        lag = g.binop(E.MaterialExpressionMultiply, -2650, y + 200, delay, stag_p)
        raw = g.binop(E.MaterialExpressionSubtract, -2500, y + 200, g.use("prog", -2650, y + 110), lag)
        g.decl(f"local{tag}", g.unary(E.MaterialExpressionSaturate, -2350, y + 200, raw), -2200, y + 200, C_SC)
        # half = local * (0.5 + wa): 扇の半角。local=1 で 0.5 + wa (= 全周 + AA 幅)
        half_k = g.binop(E.MaterialExpressionAdd, -2950, y + 420, g.use("wa", -3100, y + 420), 0.5)
        g.decl(f"half{tag}", g.binop(E.MaterialExpressionMultiply, -2800, y + 420,
                                     g.use(f"local{tag}", -2950, y + 340), half_k), -2650, y + 420, C_SC)
        # dc = |frac(ang * dir - k * 0.618034 - half + 0.5) - 0.5|, dir = 1 - 4 * frac(k * 0.5)
        k_half = g.binop(E.MaterialExpressionMultiply, -1900, y + 80, g.use(k, -2050, y + 80), 0.5)
        k_odd4 = g.binop(E.MaterialExpressionMultiply, -1600, y + 80,
                         g.unary(E.MaterialExpressionFrac, -1750, y + 80, k_half), 4.0)
        direction = g.binop(E.MaterialExpressionSubtract, -1450, y + 80, 1.0, k_odd4)
        turned = g.binop(E.MaterialExpressionMultiply, -1300, y, g.use("ang", -1450, y - 20), direction)
        start = g.binop(E.MaterialExpressionMultiply, -1450, y + 200, g.use(k, -1600, y + 200), 0.618034)
        rel = g.binop(E.MaterialExpressionSubtract, -1150, y + 60, turned, start)
        rel_c = g.binop(E.MaterialExpressionSubtract, -1000, y + 120, rel, g.use(f"half{tag}", -1150, y + 200))
        wrapped = g.unary(E.MaterialExpressionFrac, -700, y + 120,
                          g.binop(E.MaterialExpressionAdd, -850, y + 120, rel_c, 0.5))
        dc = g.unary(E.MaterialExpressionAbs, -400, y + 120,
                     g.binop(E.MaterialExpressionSubtract, -550, y + 120, wrapped, 0.5))
        # sweep = smoothstep(half, half - wa, dc)
        half_in = g.binop(E.MaterialExpressionSubtract, -400, y + 300,
                          g.use(f"half{tag}", -550, y + 280), g.use("wa", -550, y + 360))
        sweep = g.smoothstep(-200, y + 200, g.use(f"half{tag}", -400, y + 220), half_in, dc)
        # edge = 1 + wb - Gap * (1 - local)
        open_k = g.unary(E.MaterialExpressionOneMinus, -1750, y + 520, g.use(f"local{tag}", -1900, y + 520))
        gap_now = g.binop(E.MaterialExpressionMultiply, -1600, y + 520, open_k, gap_p)
        full = g.binop(E.MaterialExpressionAdd, -1600, y + 420, g.use("wb", -1750, y + 420), 1.0)
        g.decl(f"edge{tag}", g.binop(E.MaterialExpressionSubtract, -1450, y + 470, full, gap_now),
               -1300, y + 470, C_SC)
        # band = smoothstep(edge, edge - wb, dist)
        edge_in = g.binop(E.MaterialExpressionSubtract, -850, y + 560,
                          g.use(f"edge{tag}", -1000, y + 540), g.use("wb", -1000, y + 620))
        band = g.smoothstep(-650, y + 480, g.use(f"edge{tag}", -1000, y + 460), edge_in,
                            g.use(f"dist{tag}", -1000, y + 700))
        g.decl(f"mask{tag}", g.binop(E.MaterialExpressionMultiply, 0, y + 340, sweep, band), 150, y + 340, C_FX)
        g.comment(f"ring {tag}: dist / local (0..1 sweep progress) / half (wedge half-angle)",
                  -3160, y - 150, 1150, 700, C_SC)
        g.comment(f"ring {tag}: sweep = wedge from the ring's start angle (golden-ratio step), "
                  "direction alternates per ring; dc = angular distance to the wedge centre",
                  -2110, y - 150, 2050, 500, C_FX)
        g.comment(f"ring {tag}: band = ring body; the gap closes as the sweep finishes, "
                  "then the edge passes the border by wb", -2110, y + 370, 1650, 430, C_FX)

    ring("A", 2150)
    ring("B", 3150)
    union = g.binop(E.MaterialExpressionMax, 550, 2950, g.use("maskA", 400, 2900), g.use("maskB", 400, 3000))
    g.finish(union, 750, 2950)
    return g.verify_and_save(["Progress", "Invert", "FadeColor", "RingCount", "Stagger", "Gap"])


ok_ripple = build_ripple_rings()
ok_orbit = build_orbit_rings()
print("[TEST] RESULT: " + ("PASS" if ok_ripple and ok_orbit else "FAIL"))
