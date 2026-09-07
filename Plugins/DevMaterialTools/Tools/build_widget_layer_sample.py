"""
ウィジェットレイヤー版トランジションの動作確認用サンプルを生成する。

  1. /Game/Widget/WBP_WidgetLayerSample
       親クラス = UWidgetLayerSampleWidget (Source/TransitionFX_Dev)。
       レイアウト (Canvas ルート) はここで組む:
         - 右半分: 不透明な UMG パネル (PostProcess では覆えず、ウィジェットレイヤー版では覆われる目印)
         - 左上:   "3D SCENE" ラベル
         - 下部:   操作バー (PresetNameText / StatusText / Prev / Next / Play Widget Layer / Play PostProcess)
       BindWidgetOptional 名と一致させるため、ボタン/テキストは bIsVariable=True で生成する。
       WidgetPresets = /TransitionFX/Data の DA_Widget_* 全部 (アルファベット順)、
       PostProcessPresets = 同名の DA_* (無ければ None)。
  2. /Game/SampleLevel/L_WidgetLayerSample
       Template_Default から新規作成し、AWidgetLayerSampleActor (WidgetClass=上記 WBP) と
       目印のキューブ 3 個 + TextRenderActor を PlayerStart の前方に配置する。

UWidgetTree は Python から触れないため、ウィジェット生成とルート設定は
DevBlueprintTools.construct_widget_in_tree / set_widget_tree_root (C++) 経由。

実行 (エディタは閉じておく。GUI エディタが同プロジェクトを開いていると umap 保存がロックで失敗する):
  UnrealEditor-Cmd.exe TransitionFX_Dev.uproject -run=pythonscript ^
    -script="<abs>/Plugins/DevMaterialTools/Tools/build_widget_layer_sample.py" -EnablePlugins=PythonScriptPlugin

結果は Saved/DevMaterialTools/build_widget_layer_sample.result.json (print はコマンドレットでは見えない)。
"""
import json
import os

import unreal

WBP_DIR = "/Game/Widget"
WBP_NAME = "WBP_WidgetLayerSample"
WBP_PATH = f"{WBP_DIR}/{WBP_NAME}"
LEVEL_PATH = "/Game/SampleLevel/L_WidgetLayerSample"
TEMPLATE_PATH = "/Engine/Maps/Templates/Template_Default"
DA_DIR = "/TransitionFX/Data"
WIDGET_PREFIX = "DA_Widget_"
CUBE_MESH = "/Engine/BasicShapes/Cube"
CUBE_MATERIALS = [
    "/Engine/MapTemplates/Materials/BasicAsset01",
    "/Engine/MapTemplates/Materials/BasicAsset02",
    "/Engine/MapTemplates/Materials/BasicAsset03",
]
FALLBACK_MATERIAL = "/Engine/BasicShapes/BasicShapeMaterial"
OUT_DIR = os.path.join(unreal.Paths.project_saved_dir(), "DevMaterialTools")
OUT_FILE = os.path.join(OUT_DIR, "build_widget_layer_sample.result.json")

lib = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
dbt = unreal.DevBlueprintTools
log = {"checks": {}, "steps": [], "widget_tree": [], "level_actors": []}


def check(label, ok):
    log["checks"][label] = bool(ok)
    return bool(ok)


def step(msg):
    log["steps"].append(msg)


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------
def color(r, g, b, a=1.0):
    return unreal.LinearColor(r, g, b, a)


def slate_color(r, g, b, a=1.0):
    return unreal.SlateColor(specified_color=color(r, g, b, a))


def set_font_size(text_block, size):
    font = text_block.get_editor_property("font")
    font.set_editor_property("size", size)
    text_block.set_editor_property("font", font)


def make_text(wbp, name, text, size, rgba=(1, 1, 1, 1), wrap=False, variable=False, justify=None):
    tb = dbt.construct_widget_in_tree(wbp, unreal.TextBlock, name, variable)
    tb.set_text(text)
    set_font_size(tb, size)
    tb.set_color_and_opacity(slate_color(*rgba))
    if wrap:
        tb.set_auto_wrap_text(True)
    if justify is not None:
        tb.set_justification(justify)
    return tb


def make_button(wbp, name, label):
    btn = dbt.construct_widget_in_tree(wbp, unreal.Button, name, True)
    text = make_text(wbp, f"{name}Label", label, 18, (0.05, 0.05, 0.05, 1))
    btn.add_child(text)
    return btn


def collect_presets():
    """DA_Widget_* (sorted) and their PostProcess counterparts (loaded objects, None when missing)."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    names = sorted(str(ad.asset_name) for ad in registry.get_assets_by_path(DA_DIR, recursive=False))
    widget_names = [n for n in names if n.startswith(WIDGET_PREFIX)]
    widget_presets, pp_presets, pairs = [], [], []
    for wn in widget_names:
        pp_name = "DA_" + wn[len(WIDGET_PREFIX):]
        w = lib.load_asset(f"{DA_DIR}/{wn}")
        pp = lib.load_asset(f"{DA_DIR}/{pp_name}") if pp_name in names else None
        widget_presets.append(w)
        pp_presets.append(pp)
        pairs.append([wn, pp_name if pp else None])
    log["preset_pairs"] = pairs
    return widget_presets, pp_presets


def dump_widget(widget, depth=0):
    entry = f"{'  ' * depth}{widget.get_name()} ({widget.get_class().get_name()})"
    log["widget_tree"].append(entry)
    if isinstance(widget, unreal.PanelWidget):
        for child in widget.get_all_children():
            dump_widget(child, depth + 1)


# ---------------------------------------------------------------------------
# 1. Widget Blueprint
# ---------------------------------------------------------------------------
def build_widget():
    if lib.does_asset_exist(WBP_PATH):
        check("old WBP deleted", lib.delete_asset(WBP_PATH))

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.WidgetLayerSampleWidget)
    wbp = tools.create_asset(WBP_NAME, WBP_DIR, unreal.WidgetBlueprint, factory)
    if not check("WBP created", wbp is not None):
        return None
    step("WBP created with parent UWidgetLayerSampleWidget")

    root = dbt.construct_widget_in_tree(wbp, unreal.CanvasPanel, "RootCanvas", False)
    check("root canvas constructed", root is not None)
    check("root set", dbt.set_widget_tree_root(wbp, root))

    # --- right half: opaque UMG panel -------------------------------------
    panel = dbt.construct_widget_in_tree(wbp, unreal.Border, "UmgPanel", False)
    panel.set_brush_color(color(0.02, 0.07, 0.22, 1.0))
    panel.set_padding(unreal.Margin(32.0, 32.0, 32.0, 32.0))
    panel.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    panel.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_TOP)
    panel_slot = root.add_child_to_canvas(panel)
    panel_slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
    panel_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 170.0))

    panel_box = dbt.construct_widget_in_tree(wbp, unreal.VerticalBox, "UmgPanelBox", False)
    panel.set_content(panel_box)
    for name, text, size, rgba, top in [
        ("UmgPanelTitle", "UMG PANEL", 44, (1.0, 0.85, 0.2, 1.0), 0.0),
        ("UmgPanelLine1", "This half of the screen is a UMG widget (above PostProcess).", 22, (1, 1, 1, 1), 24.0),
        ("UmgPanelLine2", "Play PostProcess: only the 3D view on the left changes; this panel stays visible.", 20, (0.8, 0.85, 0.95, 1), 12.0),
        ("UmgPanelLine3", "Play Widget Layer: the whole screen, including this panel and the buttons, is covered.", 20, (0.8, 0.85, 0.95, 1), 12.0),
    ]:
        tb = make_text(wbp, name, text, size, rgba, wrap=True)
        vslot = panel_box.add_child_to_vertical_box(tb)
        vslot.set_padding(unreal.Margin(0.0, top, 0.0, 0.0))

    # --- top-left: scene label ---------------------------------------------
    scene_label = make_text(wbp, "SceneLabel", "3D SCENE (below UMG)", 30, (1.0, 1.0, 1.0, 0.9))
    scene_slot = root.add_child_to_canvas(scene_label)
    scene_slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(0.0, 0.0)))
    scene_slot.set_position(unreal.Vector2D(24.0, 24.0))
    scene_slot.set_auto_size(True)

    # --- bottom: control bar -----------------------------------------------
    bar = dbt.construct_widget_in_tree(wbp, unreal.Border, "ControlBar", False)
    bar.set_brush_color(color(0.0, 0.0, 0.0, 0.8))
    bar.set_padding(unreal.Margin(24.0, 16.0, 24.0, 16.0))
    bar.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    bar.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)
    bar_slot = root.add_child_to_canvas(bar)
    bar_slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 1.0), maximum=unreal.Vector2D(1.0, 1.0)))
    bar_slot.set_alignment(unreal.Vector2D(0.0, 1.0))
    bar_slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 170.0))

    bar_box = dbt.construct_widget_in_tree(wbp, unreal.VerticalBox, "ControlBarBox", False)
    bar.set_content(bar_box)

    preset_text = make_text(wbp, "PresetNameText", "[preset]", 26, (1.0, 0.85, 0.2, 1.0), variable=True)
    bar_box.add_child_to_vertical_box(preset_text)
    status_text = make_text(wbp, "StatusText", "[status]", 16, (0.85, 0.85, 0.85, 1.0), wrap=True, variable=True)
    status_slot = bar_box.add_child_to_vertical_box(status_text)
    status_slot.set_padding(unreal.Margin(0.0, 6.0, 0.0, 10.0))

    buttons = dbt.construct_widget_in_tree(wbp, unreal.HorizontalBox, "ButtonRow", False)
    bar_box.add_child_to_vertical_box(buttons)
    for name, label in [
        ("PrevButton", "<  Prev"),
        ("NextButton", "Next  >"),
        ("PlayWidgetButton", "Play Widget Layer"),
        ("PlayPostProcessButton", "Play PostProcess"),
    ]:
        btn = make_button(wbp, name, label)
        hslot = buttons.add_child_to_horizontal_box(btn)
        hslot.set_padding(unreal.Margin(0.0, 0.0, 12.0, 0.0))

    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    status = wbp.get_editor_property("status")
    log["wbp_status_after_layout"] = str(status)
    check("WBP compiles (layout)", status == unreal.BlueprintStatus.BS_UP_TO_DATE)

    # --- preset lists on the CDO -------------------------------------------
    widget_presets, pp_presets = collect_presets()
    if not check("widget presets found", len(widget_presets) > 0):
        return wbp
    gen_class = wbp.generated_class()
    cdo = unreal.get_default_object(gen_class)
    w_arr = unreal.Array(unreal.TransitionPreset)
    for p in widget_presets:
        w_arr.append(p)
    pp_arr = unreal.Array(unreal.TransitionPreset)
    for p in pp_presets:
        pp_arr.append(p)
    cdo.set_editor_property("widget_presets", w_arr)
    cdo.set_editor_property("post_process_presets", pp_arr)
    unreal.BlueprintEditorLibrary.compile_blueprint(wbp)
    cdo = unreal.get_default_object(wbp.generated_class())
    saved_w = [p.get_name() if p else "None" for p in cdo.get_editor_property("widget_presets")]
    saved_pp = [p.get_name() if p else "None" for p in cdo.get_editor_property("post_process_presets")]
    log["cdo_widget_presets"] = saved_w
    log["cdo_post_process_presets"] = saved_pp
    check("CDO preset lists", len(saved_w) == len(widget_presets) and "None" not in saved_w
          and len(saved_pp) == len(pp_presets))
    status = wbp.get_editor_property("status")
    log["wbp_status_final"] = str(status)
    check("WBP compiles (final)", status == unreal.BlueprintStatus.BS_UP_TO_DATE)

    dump_widget(dbt.get_widget_tree_root(wbp))
    check("WBP saved", lib.save_asset(WBP_PATH))
    return wbp


# ---------------------------------------------------------------------------
# 2. Level
# ---------------------------------------------------------------------------
def build_level(wbp):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    if lib.does_asset_exist(LEVEL_PATH):
        check("old level deleted", lib.delete_asset(LEVEL_PATH))
    if not check("level created from template", les.new_level_from_template(LEVEL_PATH, TEMPLATE_PATH)):
        return

    ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    world = ues.get_editor_world()

    starts = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.PlayerStart)
    if not check("PlayerStart found", len(starts) > 0):
        return
    start = starts[0]
    base = start.get_actor_location()
    fwd = start.get_actor_forward_vector()
    right = start.get_actor_right_vector()
    yaw = start.get_actor_rotation().yaw
    log["player_start"] = [base.x, base.y, base.z, yaw]

    # sample driver actor
    driver = eas.spawn_actor_from_class(unreal.WidgetLayerSampleActor, base + unreal.Vector(0, 0, 100))
    if not check("sample actor spawned", driver is not None):
        return
    driver.set_actor_label("WidgetLayerSample")
    driver.set_editor_property("widget_class", wbp.generated_class())
    check("sample actor widget class", driver.get_editor_property("widget_class") == wbp.generated_class())

    # landmarks: three cubes in front of the player, biased to the left half of the view
    # Engine content is not in the commandlet's asset registry, so load by object path instead of
    # EditorAssetLibrary.load_asset; fall back to the plain BasicShapeMaterial if a template material is missing.
    def load_engine_asset(path):
        return unreal.load_object(None, f"{path}.{path.rsplit('/', 1)[-1]}")

    cube = load_engine_asset(CUBE_MESH)
    fallback_mat = load_engine_asset(FALLBACK_MATERIAL)
    mats = [load_engine_asset(p) or fallback_mat for p in CUBE_MATERIALS]
    log["cube_materials"] = [m.get_path_name() if m else "None" for m in mats]
    check("cube mesh loaded", cube is not None)
    check("cube materials loaded", all(m is not None for m in mats))
    # All landmarks sit on the player's left (negative "right" offset): the right half of the
    # view is hidden behind the opaque UMG panel.
    for i, (dist, side, height, scale) in enumerate([(700.0, -560.0, 60.0, 1.2), (950.0, -300.0, 90.0, 1.8), (520.0, -120.0, 40.0, 0.8)]):
        loc = base + fwd * dist + right * side
        loc.z = height
        # unreal.Rotator's positional order is (roll, pitch, yaw): always pass keywords.
        actor = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc, unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 25.0 * i))
        actor.set_actor_label(f"LandmarkCube_{i}")
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
        smc = actor.static_mesh_component
        smc.set_static_mesh(cube)
        if mats[i % len(mats)]:
            smc.set_material(0, mats[i % len(mats)])

    # TextRender's readable face is its local +X, so turn it back towards the player (yaw + 180).
    text = eas.spawn_actor_from_class(unreal.TextRenderActor, base + fwd * 1100.0 + right * -380.0 + unreal.Vector(0, 0, 340),
                                      unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw + 180.0))
    text.set_actor_label("SceneText")
    trc = text.text_render
    trc.set_text("3D SCENE\n(below UMG)")
    trc.set_world_size(70.0)
    trc.set_horizontal_alignment(unreal.HorizTextAligment.EHTA_CENTER)
    trc.set_text_render_color(unreal.Color(255, 220, 60, 255))

    for a in eas.get_all_level_actors():
        log["level_actors"].append(f"{a.get_actor_label()} ({a.get_class().get_name()})")

    check("level saved", unreal.EditorLoadingAndSavingUtils.save_map(world, LEVEL_PATH))


def run():
    if not check("UWidgetLayerSampleWidget class available", hasattr(unreal, "WidgetLayerSampleWidget")):
        return
    if not check("DevBlueprintTools widget helpers available", hasattr(dbt, "construct_widget_in_tree")):
        return
    wbp = build_widget()
    if wbp is None:
        return
    build_level(wbp)


try:
    run()
except Exception as ex:  # noqa: BLE001
    import traceback
    log["exception"] = repr(ex)
    log["traceback"] = traceback.format_exc()

log["result"] = "PASS" if log["checks"] and all(log["checks"].values()) and "exception" not in log else "FAIL"
os.makedirs(OUT_DIR, exist_ok=True)
with open(OUT_FILE, "w", encoding="utf-8") as f:
    json.dump(log, f, ensure_ascii=False, indent=2)
print(f"[TEST] RESULT: {log['result']} -> {OUT_FILE}")
