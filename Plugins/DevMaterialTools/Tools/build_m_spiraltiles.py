"""
M_Transition_SpiralTiles を Named Reroute 注釈付きで構築する (in-place 再構築可)。

プラグインのコンテンツなので UE 5.5 のワークスペースで実行すること (CLAUDE.md "Plugin content is saved with UE 5.5")。
数式の原型は Saved/TransitionIdeas/transition_ideas.py (numpy プロトタイプ) の spiral_tiles。

画面を Columns x Rows のタイルに分け、外周の左上から時計回りに中心へ向かう矩形スパイラルの順に
タイルが中心から膨らんで埋まる。画面 UV をそのまま使うので、タイルが正方形になるのは
Columns : Rows が画面のアスペクト比と同じとき (既定 16 x 9)。

  cols, rows = max(round(Columns), 1), max(round(Rows), 1)
  cell  = uv * (cols, rows)                         タイル座標
  tile  = min(floor(cell), last)                    タイルの (列, 行)。last = (cols, rows) - 1
  ring  = min(tile, last - tile) の小さい方の成分    外周から数えた周回番号 (0 = 最外周)
  span  = last - 2 * ring                           その周の (幅 - 1, 高さ - 1) = (sw, sh)
  (a, b) = tile - ring                              周の左上を原点としたタイル位置
  off   = 周の中での順番 (左上から時計回り):
            上辺 (b = 0)   a
            右辺 (a = sw)  sw + b
            下辺 (b = sh)  2 * sw + sh - a
            左辺           2 * sw + 2 * sh - b
          上 > 右 > 下 > 左 の優先順で lerp で選ぶ (a, b は整数なので saturate が 0/1 の選択子になる)。
          幅か高さが 1 の最内周もこの式のままで正しい。
  order = (ring * (2 * (cols + rows) - 4 * ring) + off) / (cols * rows)     0..(N-1)/N
  local = saturate((Progress * (1 + Tail) - order) / Tail)    タイルの成長度。Tail = 同時に成長中の区間の長さ
  size  = |frac(cell) - 0.5| * 2                    タイル中心からの距離 (軸ごと、1 = タイルの端)
  w     = |(ddx(cell.x), ddy(cell.y))| * 4          軸ごとの 2px (size 単位)
  edge  = local * (1 + w)                           タイルの現在の半幅。local = 1 で端を w だけ越える
  mask  = smoothstep(edge, edge - w, size) の x 成分 * y 成分
  出力  = lerp(SceneTexture.rgb, FadeColor, MF_ApplyInvert(mask, Invert)) -> EmissiveColor
  Progress = 0 で mask = 0、Progress = 1 で mask = 1。
ルール: GLSL ローカル変数 = Named Reroute Declaration / 再利用 = Usage。

実行例 (UE 5.5 ワークスペース):
  UnrealEditor-Cmd.exe T55W.uproject -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/build_m_spiraltiles.py" -EnablePlugins=PythonScriptPlugin
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from tfx_material_graph import Graph, E, C_CO, C_FX, C_SC, C_UV  # noqa: E402

ADD = E.MaterialExpressionAdd
SUB = E.MaterialExpressionSubtract
MUL = E.MaterialExpressionMultiply
DIV = E.MaterialExpressionDivide
MIN = E.MaterialExpressionMin
MAX = E.MaterialExpressionMax
SAT = E.MaterialExpressionSaturate
ABS = E.MaterialExpressionAbs


def build_spiral_tiles():
    g = Graph("M_Transition_SpiralTiles")

    # ---- grid = (cols, rows) / last = grid - 1 ----
    cols_p = g.sparam("Columns", 16.0, -4400, 0)
    rows_p = g.sparam("Rows", 9.0, -4400, 180)
    g.decl("cols", g.binop(MAX, -4050, 0, g.unary(E.MaterialExpressionRound, -4200, 0, cols_p), 1.0),
           -3900, 0, C_CO)
    g.decl("rows", g.binop(MAX, -4050, 180, g.unary(E.MaterialExpressionRound, -4200, 180, rows_p), 1.0),
           -3900, 180, C_CO)
    g.decl("grid", g.binop(E.MaterialExpressionAppendVector, -3500, 90,
                           g.use("cols", -3650, 40), g.use("rows", -3650, 140)), -3350, 90, C_CO)
    g.decl("last", g.binop(SUB, -2950, 90, g.use("grid", -3100, 90), 1.0), -2800, 90, C_CO)
    g.comment("grid = (cols, rows) = max(round(Columns / Rows), 1) / last = grid - 1 (index of the last column / row)",
              -4470, -100, 1900, 420, C_CO)

    # ---- cell = uv * grid / tile = min(floor(cell), last) ----
    tc = g.expr(E.MaterialExpressionTextureCoordinate, -4400, 500)
    g.decl("cell", g.binop(MUL, -4200, 500, tc, g.use("grid", -4400, 620)), -4050, 500, C_UV)
    fl = g.unary(E.MaterialExpressionFloor, -3700, 500, g.use("cell", -3850, 500))
    g.decl("tile", g.binop(MIN, -3550, 500, fl, g.use("last", -3700, 600)), -3400, 500, C_CO)
    g.comment("cell = uv * grid (tile coordinates) / tile = (column, row) of this pixel's tile",
              -4470, 400, 1300, 330, C_UV)

    # ---- ring = min component of min(tile, last - tile) ----
    far = g.binop(SUB, -2850, 560, g.use("last", -3000, 520), g.use("tile", -3000, 610))
    g.decl("edgeDist", g.binop(MIN, -2700, 500, g.use("tile", -2850, 460), far), -2550, 500, C_CO)
    ring = g.binop(MIN, -2050, 500, g.channel(-2200, 450, g.use("edgeDist", -2350, 450), "r"),
                   g.channel(-2200, 560, g.use("edgeDist", -2350, 560), "g"))
    g.decl("ring", ring, -1900, 500, C_CO)
    g.comment("ring = how many tiles this tile is away from the nearest screen edge (0 = outermost lap)",
              -3060, 400, 1400, 330, C_CO)

    # ---- span = last - 2 * ring = (sw, sh) / (a, b) = tile - ring ----
    ring2 = g.binop(MUL, -4200, 960, g.use("ring", -4400, 960), 2.0)
    g.decl("span", g.binop(SUB, -4050, 900, g.use("last", -4200, 860), ring2), -3900, 900, C_CO)
    g.decl("ab", g.binop(SUB, -4050, 1150, g.use("tile", -4200, 1110), g.use("ring", -4200, 1200)),
           -3900, 1150, C_CO)
    g.decl("sw", g.channel(-3500, 860, g.use("span", -3650, 860), "r"), -3350, 860, C_CO)
    g.decl("sh", g.channel(-3500, 960, g.use("span", -3650, 960), "g"), -3350, 960, C_CO)
    g.decl("a", g.channel(-3500, 1110, g.use("ab", -3650, 1110), "r"), -3350, 1110, C_CO)
    g.decl("b", g.channel(-3500, 1210, g.use("ab", -3650, 1210), "g"), -3350, 1210, C_CO)
    g.comment("this lap is a rectangle of (sw + 1) x (sh + 1) tiles; (a, b) = tile position inside it "
              "(0, 0 = the lap's top-left tile)", -4470, 770, 1300, 560, C_CO)

    # ---- onFar = saturate(ab - span + 1) = (right edge?, bottom edge?) / isTop = 1 - saturate(b) ----
    rel = g.binop(SUB, -2850, 900, g.use("ab", -3000, 860), g.use("span", -3000, 950))
    g.decl("onFar", g.unary(SAT, -2550, 900, g.binop(ADD, -2700, 900, rel, 1.0)), -2400, 900, C_SC)
    g.decl("isTop", g.unary(E.MaterialExpressionOneMinus, -2550, 1100, g.unary(SAT, -2700, 1100,
                                                                              g.use("b", -2850, 1100))),
           -2400, 1100, C_SC)
    g.comment("edge selectors (0 or 1, a / b are integers): onFar = (a == sw, b == sh) = on the right / bottom edge, "
              "isTop = (b == 0)", -3060, 770, 850, 480, C_SC)

    # ---- off = index of the tile along its lap (clockwise from the top-left tile) ----
    off_right = g.binop(ADD, -1650, 900, g.use("sw", -1800, 860), g.use("b", -1800, 950))
    sw2 = g.binop(MUL, -1650, 1080, g.use("sw", -1800, 1080), 2.0)
    g.decl("base", g.binop(ADD, -1500, 1120, sw2, g.use("sh", -1650, 1180)), -1350, 1120, C_SC)
    off_bottom = g.binop(SUB, -950, 1050, g.use("base", -1100, 1010), g.use("a", -1100, 1100))
    left_k = g.binop(ADD, -950, 1250, g.use("base", -1100, 1210), g.use("sh", -1100, 1300))
    off_left = g.binop(SUB, -800, 1290, left_k, g.use("b", -950, 1370))
    far_r = g.channel(-650, 860, g.use("onFar", -800, 860), "r")
    far_b = g.channel(-650, 1130, g.use("onFar", -800, 1130), "g")
    sel_bottom = g.lerp(-450, 1150, off_left, off_bottom, far_b)
    sel_right = g.lerp(-250, 950, sel_bottom, off_right, far_r)
    sel_top = g.lerp(-50, 800, sel_right, g.use("a", -250, 760), g.use("isTop", -250, 850))
    g.decl("off", sel_top, 100, 800, C_SC)
    g.comment("off = index along the lap, clockwise from its top-left tile: top a / right sw + b / "
              "bottom 2sw + sh - a / left 2sw + 2sh - b (priority top > right > bottom > left)",
              -1860, 670, 2150, 800, C_SC)

    # ---- order = (ring * (2 * (cols + rows) - 4 * ring) + off) / (cols * rows) ----
    perim2 = g.binop(MUL, -4050, 1650, g.binop(ADD, -4200, 1650, g.use("cols", -4400, 1610),
                                              g.use("rows", -4400, 1700)), 2.0)
    ring4 = g.binop(MUL, -4050, 1800, g.use("ring", -4200, 1800), 4.0)
    before = g.binop(MUL, -3700, 1700, g.use("ring", -3900, 1620), g.binop(SUB, -3900, 1720, perim2, ring4))
    total = g.binop(MUL, -3700, 1900, g.use("cols", -3900, 1870), g.use("rows", -3900, 1960))
    index = g.binop(ADD, -3500, 1700, before, g.use("off", -3700, 1600))
    g.decl("order", g.binop(DIV, -3350, 1750, index, total), -3200, 1750, C_SC)
    g.comment("order = (tiles in the laps outside this one + off) / tile count: 0 for the first tile, "
              "(N - 1) / N for the last", -4470, 1500, 1500, 560, C_SC)

    # ---- local = saturate((Progress * (1 + Tail) - order) / Tail) ----
    tail_p = g.sparam("Tail", 0.07, -2850, 1900)
    prog_p = g.sparam("Progress", 0.5, -2400, 1600)
    g.decl("tail", g.binop(MAX, -2650, 1900, tail_p, 0.0001), -2500, 1900, C_SC)
    stretch = g.binop(MUL, -2000, 1650, prog_p, g.binop(ADD, -2150, 1750, g.use("tail", -2300, 1750), 1.0))
    ahead = g.binop(SUB, -1850, 1700, stretch, g.use("order", -2000, 1800))
    grown = g.binop(DIV, -1700, 1750, ahead, g.use("tail", -1850, 1850))
    g.decl("local", g.unary(SAT, -1550, 1750, grown), -1400, 1750, C_SC)
    g.comment("local = saturate((Progress * (1 + Tail) - order) / Tail): growth of this tile; "
              "Tail = share of the spiral that is growing at the same time", -2910, 1500, 1750, 560, C_SC)

    # ---- size = |frac(cell) - 0.5| * 2 / w = |(ddx(cell.x), ddy(cell.y))| * 4 ----
    centred = g.binop(SUB, -4050, 2300, g.unary(E.MaterialExpressionFrac, -4200, 2300,
                                               g.use("cell", -4400, 2300)), 0.5)
    g.decl("size", g.binop(MUL, -3750, 2300, g.unary(ABS, -3900, 2300, centred), 2.0), -3600, 2300, C_CO)
    ddx = g.unary(E.MaterialExpressionDDX, -4050, 2500, g.channel(-4200, 2500, g.use("cell", -4400, 2500), "r"))
    ddy = g.unary(E.MaterialExpressionDDY, -4050, 2620, g.channel(-4200, 2620, g.use("cell", -4400, 2620), "g"))
    grad = g.unary(ABS, -3750, 2550, g.binop(E.MaterialExpressionAppendVector, -3900, 2550, ddx, ddy))
    g.decl("w", g.binop(MUL, -3600, 2550, grad, 4.0), -3450, 2550, C_SC)
    g.comment("size = distance from the tile centre per axis (1 = tile border) / w = 2 pixels per axis in size units",
              -4470, 2200, 1250, 560, C_CO)

    # ---- edge = local * (1 + w) / mask = smoothstep(edge, edge - w, size).x * .y ----
    reach = g.binop(ADD, -2850, 2400, g.use("w", -3000, 2400), 1.0)
    g.decl("edge", g.binop(MUL, -2700, 2350, g.use("local", -2850, 2300), reach), -2550, 2350, C_SC)
    inner = g.binop(SUB, -2050, 2420, g.use("edge", -2200, 2380), g.use("w", -2200, 2470))
    g.decl("axisMask", g.smoothstep(-1850, 2350, g.use("edge", -2050, 2300), inner, g.use("size", -2050, 2550)),
           -1650, 2350, C_FX)
    mask = g.binop(MUL, -1100, 2350, g.channel(-1250, 2300, g.use("axisMask", -1400, 2300), "r"),
                   g.channel(-1250, 2410, g.use("axisMask", -1400, 2410), "g"))
    g.comment("edge = local * (1 + w): the tile's half-size, a full tile passes its border by w / "
              "mask = smoothstep(edge, edge - w, size) in x times y (Min > Max: 1 inside the tile)",
              -3060, 2200, 2150, 500, C_FX)

    g.finish(mask, -700, 2350)
    return g.verify_and_save(["Progress", "Invert", "FadeColor", "Columns", "Rows", "Tail"])


print("[TEST] RESULT: " + ("PASS" if build_spiral_tiles() else "FAIL"))
