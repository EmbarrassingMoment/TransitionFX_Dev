"""
TransitionFX マスターマテリアルのビルダー共通ヘルパー (build_m_rings.py / build_m_spiraltiles.py が import する)。

  Graph        1 マテリアル分のグラフ構築。接続結果と Named Reroute の使用回数を記録し、verify_and_save() で検証して保存する
  screen_coords()  p (アスペクト補正座標) / maxR / px を宣言する
  finish()     全マスター共通の最終段: EmissiveColor = lerp(SceneTexture.rgb, FadeColor, MF_ApplyInvert(mask, Invert))

ルール: GLSL ローカル変数 = Named Reroute Declaration (decl) / 再利用 = Usage (use)。
プラグインのコンテンツを保存するので UE 5.5 のワークスペースで実行すること。

ビルダー側の import (pythonscript コマンドレットは Tools フォルダを sys.path に入れない):
  import os, sys
  sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
  from tfx_material_graph import Graph, E, C_UV, C_CO, C_SC, C_CL, C_FX
"""
import unreal

MAT_DIR = "/TransitionFX/Materials"
MF_ASPECT_PATH = "/TransitionFX/MaterialFunctions/MF_CorrectAspectRatio"
MF_MAXR_PATH = "/TransitionFX/MaterialFunctions/MF_GetMaxScreenRadius"
MF_INV_PATH = "/TransitionFX/MaterialFunctions/MF_ApplyInvert"

mel = unreal.MaterialEditingLibrary
lib = unreal.EditorAssetLibrary
dmt = unreal.DevMaterialTools

C_UV = unreal.LinearColor(0.2, 0.6, 1.0, 1.0)   # 画面座標系 = 青
C_CO = unreal.LinearColor(1.0, 0.6, 0.1, 1.0)   # パターン座標 = 橙
C_SC = unreal.LinearColor(0.6, 1.0, 0.2, 1.0)   # スカラー中間値 = 緑
C_CL = unreal.LinearColor(0.7, 0.4, 1.0, 1.0)   # 色 = 紫
C_FX = unreal.LinearColor(1.0, 0.9, 0.3, 1.0)   # マスク = 黄

E = unreal  # 式クラスの短縮用

fn_aspect = lib.load_asset(MF_ASPECT_PATH)
fn_maxr = lib.load_asset(MF_MAXR_PATH)
fn_inv = lib.load_asset(MF_INV_PATH)
assert fn_aspect and fn_maxr and fn_inv, "required material functions not found"


class Graph:
    """1 マテリアル分のグラフ構築ヘルパー (接続結果と Named Reroute の使用回数を記録して最後に検証する)。"""

    def __init__(self, name):
        self.name = name
        self.path = f"{MAT_DIR}/{name}"
        self.mat = lib.load_asset(self.path) if lib.does_asset_exist(self.path) else None
        self.is_new = self.mat is None
        if self.is_new:
            # 他のマスターと同じ PostProcess 設定で新規作成する
            self.mat = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
                name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
            self.mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
            self.mat.set_editor_property("blendable_location",
                                         unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
            self.mat.set_editor_property("blendable_output_alpha", False)
        assert self.mat, f"could not load or create {self.path}"
        self.results = []
        self.created = []
        self.decl_map = {}
        self.use_counts = {}
        self.comments = []
        dmt.clear_comments_in_material(self.mat)
        # delete_all_material_expressions は1回では消し残る (交互スキップ様) ため空になるまで繰り返す
        for _ in range(10):
            if not dmt.get_material_expressions(self.mat):
                break
            mel.delete_all_material_expressions(self.mat)
        self.graph_cleared = not dmt.get_material_expressions(self.mat)

    # ---- ノード生成 ----
    def expr(self, cls, x, y, **props):
        e = mel.create_material_expression(self.mat, cls, x, y)
        for k, v in props.items():
            e.set_editor_property(k, v)
        self.created.append(e)
        return e

    def connect(self, frm, out_name, to, in_name):
        ok = mel.connect_material_expressions(frm, out_name, to, in_name)
        if not ok:
            print(f"[TEST] {self.name}: FAILED connect {frm.get_class().get_name()}('{out_name}') -> "
                  f"{to.get_class().get_name()}('{in_name}')")
        self.results.append(ok)
        return ok

    def binop(self, cls, x, y, a, b):
        """2 入力ノード (A/B)。数値を渡した側は const_a / const_b に入れる。"""
        e = self.expr(cls, x, y)
        for pin, v in (("A", a), ("B", b)):
            if isinstance(v, (int, float)):
                e.set_editor_property(f"const_{pin.lower()}", float(v))
            else:
                self.connect(v, "", e, pin)
        return e

    def unary(self, cls, x, y, src):
        e = self.expr(cls, x, y)
        self.connect(src, "", e, "")
        return e

    def channel(self, x, y, src, channels):
        e = self.expr(E.MaterialExpressionComponentMask, x, y,
                      r="r" in channels, g="g" in channels, b="b" in channels, a="a" in channels)
        self.connect(src, "", e, "")
        return e

    def call(self, fn, x, y, **inputs):
        e = self.expr(E.MaterialExpressionMaterialFunctionCall, x, y, material_function=fn)
        for pin, src in inputs.items():
            self.connect(src, "", e, pin)
        return e

    def smoothstep(self, x, y, vmin, vmax, value):
        e = self.expr(E.MaterialExpressionSmoothStep, x, y)
        self.connect(vmin, "", e, "Min")
        self.connect(vmax, "", e, "Max")
        self.connect(value, "", e, "Value")
        return e

    def lerp(self, x, y, a, b, alpha):
        e = self.expr(E.MaterialExpressionLinearInterpolate, x, y)
        self.connect(a, "", e, "A")
        self.connect(b, "", e, "B")
        self.connect(alpha, "", e, "Alpha")
        return e

    def sparam(self, name, default, x, y):
        return self.expr(E.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=default)

    def decl(self, name, src, x, y, color):
        d = self.expr(E.MaterialExpressionNamedRerouteDeclaration, x, y, name=name, node_color=color)
        self.connect(src, "", d, "")
        self.decl_map[name] = d
        self.use_counts.setdefault(name, 0)
        return d

    def use(self, name, x, y):
        u = dmt.create_named_reroute_usage(self.mat, self.decl_map[name], x, y)
        self.created.append(u)
        self.use_counts[name] += 1
        return u

    def comment(self, text, x, y, w, h, color):
        self.comments.append(dmt.create_comment_in_material(self.mat, text, x, y, w, h, color) is not None)

    # ---- 全マスター共通の座標と最終段 ----
    def screen_coords(self, x, y):
        """p (アスペクト補正座標) / maxR (中心→角の距離) / px (p 単位の 1px) を宣言する。"""
        tc = self.expr(E.MaterialExpressionTextureCoordinate, x, y)
        self.decl("p", self.call(fn_aspect, x + 200, y, TexCoord=tc), x + 500, y, C_UV)
        self.decl("maxR", self.call(fn_maxr, x + 200, y + 180), x + 500, y + 180, C_UV)
        p_y = self.channel(x + 900, y + 90, self.use("p", x + 750, y + 90), "g")
        ddy = self.unary(E.MaterialExpressionDDY, x + 1050, y + 90, p_y)
        self.decl("px", self.unary(E.MaterialExpressionAbs, x + 1200, y + 90, ddy), x + 1350, y + 90, C_UV)
        self.comment("p = aspect-corrected centred coords (height = 1) / maxR = centre-to-corner distance / "
                     "px = |ddy(p.y)| = one pixel in p units", x - 70, y - 100, 1700, 420, C_UV)

    def finish(self, mask, x, y):
        """alpha = ApplyInvert(mask, Invert) → EmissiveColor = lerp(scene.rgb, FadeColor, alpha)。"""
        inv_p = self.sparam("Invert", 0.0, x, y + 120)
        inv_call = self.call(fn_inv, x + 200, y, Mask=mask, Invert=inv_p)
        self.decl("alpha", inv_call, x + 450, y, C_FX)
        st = self.expr(E.MaterialExpressionSceneTexture, x + 200, y - 500,
                       scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
        self.decl("col", self.channel(x + 450, y - 500, st, "rgb"), x + 600, y - 500, C_CL)
        fade_p = self.expr(E.MaterialExpressionVectorParameter, x + 600, y - 330, parameter_name="FadeColor",
                           default_value=unreal.LinearColor(0.0, 0.0, 0.0, 1.0))
        lerp_c = self.expr(E.MaterialExpressionLinearInterpolate, x + 1000, y - 350)
        self.connect(self.use("col", x + 800, y - 450), "", lerp_c, "A")
        self.connect(fade_p, "", lerp_c, "B")
        self.connect(self.use("alpha", x + 800, y - 150), "", lerp_c, "Alpha")
        self.results.append(mel.connect_material_property(lerp_c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR))
        self.comment("alpha = ApplyInvert(mask) = |Invert - mask|", x - 60, y - 90, 720, 330, C_FX)
        self.comment("out = lerp(scene.rgb, FadeColor, alpha) -> emissive", x + 140, y - 600, 1100, 400, C_CL)

    # ---- 検証と保存 ----
    def verify_and_save(self, required_params):
        decls, usage_count = [], {}
        all_exprs = dmt.get_material_expressions(self.mat)
        for e in all_exprs:
            if isinstance(e, E.MaterialExpressionNamedRerouteDeclaration):
                decls.append(str(e.get_editor_property("name")))
            elif isinstance(e, E.MaterialExpressionNamedRerouteUsage):
                nm = dmt.get_named_reroute_usage_display_name(e)
                usage_count[nm] = usage_count.get(nm, 0) + 1
        unused = sorted(n for n, c in self.use_counts.items() if c == 0)
        scalars = sorted(str(n) for n in mel.get_scalar_parameter_names(self.mat))
        vectors = sorted(str(n) for n in mel.get_vector_parameter_names(self.mat))
        missing = sorted(set(required_params) - set(scalars) - set(vectors))
        tag = f"[TEST] {self.name}"
        print(f"{tag}: {'created new asset' if self.is_new else 'rebuilt in place'}, cleared={self.graph_cleared}")
        print(f"{tag}: declarations ({len(decls)}): {sorted(decls)}")
        print(f"{tag}: usage counts: {dict(sorted(usage_count.items()))}")
        print(f"{tag}: connections {sum(self.results)}/{len(self.results)}, "
              f"expressions {len(all_exprs)} (created {len(self.created)}), "
              f"comments {sum(self.comments)}/{len(self.comments)}")
        print(f"{tag}: scalar params {scalars}, vector params {vectors}, missing {missing}, unused decls {unused}")
        mel.recompile_material(self.mat)
        saved = lib.save_asset(self.path)
        passed = (all(self.results) and self.graph_cleared and sorted(decls) == sorted(self.decl_map)
                  and usage_count == self.use_counts and not unused and len(all_exprs) == len(self.created)
                  and all(self.comments) and not missing and saved)
        print(f"{tag}: saved={saved} RESULT: {'PASS' if passed else 'FAIL'}")
        return passed
