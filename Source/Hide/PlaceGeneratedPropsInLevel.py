import unreal

SCENE_ACTOR_NAME = "FbxScene_main_map"
GENERATED_BP_DIR = "/Game/ThirdPerson/Blueprints/Main/Props/Generated"
PROP_BP_PREFIX = "BP_Prop_"
PLACED_LABEL_PREFIX = "Placed_BP_Prop_"

# 배치되는 BP_Prop Actor의 월드 스케일을 고정합니다.
FIXED_ACTOR_SCALE = unreal.Vector(25.4, 25.4, 25.4)

# True로 바꾸면 배치 후 원본 FbxScene_main_map Actor를 숨깁니다. 처음 실행은 False 권장.
HIDE_SOURCE_SCENE_ACTOR = False

editor_level = unreal.EditorLevelLibrary
asset_lib = unreal.EditorAssetLibrary


def normalize_name(name):
    return str(name).lower().replace(" ", "").replace("_", "").replace("-", "")


def is_scene_actor(actor):
    label = actor.get_actor_label() if hasattr(actor, "get_actor_label") else actor.get_name()
    class_name = actor.get_class().get_name()
    wanted = normalize_name(SCENE_ACTOR_NAME)
    return wanted in normalize_name(label) or wanted in normalize_name(class_name) or wanted in normalize_name(actor.get_name())


def find_source_scene_actor():
    selected = editor_level.get_selected_level_actors()
    for actor in selected:
        if actor and is_scene_actor(actor):
            return actor

    for actor in editor_level.get_all_level_actors():
        if actor and is_scene_actor(actor):
            return actor

    return None


def get_static_mesh(comp):
    if hasattr(comp, "get_static_mesh"):
        return comp.get_static_mesh()
    try:
        return comp.get_editor_property("static_mesh")
    except Exception:
        return None


def get_comp_location(comp):
    for method_name in ["get_world_location", "get_component_location"]:
        if hasattr(comp, method_name):
            return getattr(comp, method_name)()
    return comp.get_editor_property("relative_location")


def get_comp_rotation(comp):
    for method_name in ["get_world_rotation", "get_component_rotation"]:
        if hasattr(comp, method_name):
            return getattr(comp, method_name)()
    return comp.get_editor_property("relative_rotation")


def get_comp_scale(comp):
    for method_name in ["get_world_scale", "get_component_scale"]:
        if hasattr(comp, method_name):
            return getattr(comp, method_name)()
    return comp.get_editor_property("relative_scale3d")




def load_prop_class(mesh_name):
    bp_path = f"{GENERATED_BP_DIR}/{PROP_BP_PREFIX}{mesh_name}"
    if hasattr(asset_lib, "load_blueprint_class"):
        return asset_lib.load_blueprint_class(bp_path)
    return None


def get_existing_labels():
    labels = set()
    for actor in editor_level.get_all_level_actors():
        if actor and hasattr(actor, "get_actor_label"):
            labels.add(actor.get_actor_label())
    return labels


def main():
    source_actor = find_source_scene_actor()
    if not source_actor:
        raise RuntimeError(f"Level actor not found: {SCENE_ACTOR_NAME}. Select it in the level and run again.")

    existing_labels = get_existing_labels()
    comps = source_actor.get_components_by_class(unreal.StaticMeshComponent)

    created = 0
    skipped = 0
    failed = 0

    unreal.log(f"Source scene actor: {source_actor.get_actor_label()}")
    unreal.log(f"StaticMeshComponent count: {len(comps)}")

    for comp in comps:
        mesh = get_static_mesh(comp)
        if not mesh:
            skipped += 1
            continue

        mesh_name = mesh.get_name()
        if not (len(mesh_name) == 3 and mesh_name.isdigit()):
            skipped += 1
            continue

        prop_class = load_prop_class(mesh_name)
        if not prop_class:
            failed += 1
            unreal.log_warning(f"Prop BP class not found for mesh {mesh_name}: {GENERATED_BP_DIR}/{PROP_BP_PREFIX}{mesh_name}")
            continue

        label = f"{PLACED_LABEL_PREFIX}{mesh_name}_{comp.get_name()}"
        if label in existing_labels:
            skipped += 1
            unreal.log(f"Skipped existing actor label: {label}")
            continue

        try:
            location = get_comp_location(comp)
            rotation = get_comp_rotation(comp)
            actor = editor_level.spawn_actor_from_class(prop_class, location, rotation)
            if not actor:
                raise RuntimeError("spawn_actor_from_class returned None")

            actor.set_actor_scale3d(FIXED_ACTOR_SCALE)
            if hasattr(actor, "set_actor_label"):
                actor.set_actor_label(label)

            created += 1
            existing_labels.add(label)
            unreal.log(f"Placed: {label}")
        except Exception as exc:
            failed += 1
            unreal.log_error(f"Failed placing {mesh_name} from {comp.get_name()}: {exc}")

    if HIDE_SOURCE_SCENE_ACTOR:
        source_actor.set_actor_hidden_in_game(True)
        source_actor.set_is_temporarily_hidden_in_editor(True)

    unreal.log(f"Done. Placed={created}, Skipped={skipped}, Failed={failed}")


main()