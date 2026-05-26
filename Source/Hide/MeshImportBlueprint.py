import unreal

SOURCE_MESH_DIR = "/Game/ThirdPerson/Blueprints/Main/Map/GameMap2"
OUTPUT_BP_DIR = "/Game/ThirdPerson/Blueprints/Main/Props/Generated"
TEMPLATE_BP = "/Game/ThirdPerson/Blueprints/Main/Props/BP_Prop_Template"
OUTLINE_MATERIAL = "/Game/ThirdPerson/Blueprints/Main/Materials/Material_Outline"
MAIN_COMPONENT_NAME = "StaticMesh"
OUTLINE_COMPONENT_NAME = "OutlineMesh"

MAIN_SCALE = unreal.Vector(2.54, 2.54, 2.54)
OUTLINE_SCALE = unreal.Vector(1.0, 1.0, 1.0)

asset_registry = unreal.AssetRegistryHelpers.get_asset_registry()
asset_lib = unreal.EditorAssetLibrary

outline_mat = asset_lib.load_asset(OUTLINE_MATERIAL)
template_bp = asset_lib.load_asset(TEMPLATE_BP)

if not outline_mat:
    raise RuntimeError(f"Outline material not found: {OUTLINE_MATERIAL}")

if not template_bp:
    raise RuntimeError(f"Template BP not found: {TEMPLATE_BP}")

if not asset_lib.does_directory_exist(OUTPUT_BP_DIR):
    asset_lib.make_directory(OUTPUT_BP_DIR)


def normalize_name(name):
    return str(name).lower().replace(" ", "").replace("_", "").replace("-", "")


def is_static_component_name(name):
    n = normalize_name(name)
    return n in [normalize_name(MAIN_COMPONENT_NAME), "staticmeshcomponent"]


def is_outline_component_name(name):
    n = normalize_name(name)
    return "outline" in n


def add_bp_component(bp, component_class, component_name):
    if not hasattr(unreal, "SubobjectDataSubsystem"):
        unreal.log_warning("SubobjectDataSubsystem is not available in this Unreal version")
        return None

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    if not subsystem:
        unreal.log_warning("SubobjectDataSubsystem could not be loaded")
        return None

    handles = subsystem.k2_gather_subobject_data_for_blueprint(context=bp)
    if not handles:
        unreal.log_warning(f"{bp.get_name()}: no blueprint subobject root handle found")
        return None

    root_handle = handles[0]
    params = unreal.AddNewSubobjectParams(
        parent_handle=root_handle,
        new_class=component_class,
        blueprint_context=bp,
        conform_transform_to_parent=True,
    )

    new_handle, fail_reason = subsystem.add_new_subobject(params=params)

    if fail_reason and not fail_reason.is_empty():
        unreal.log_warning(f"{bp.get_name()}: add {component_name} failed: {fail_reason}")
        return None

    subsystem.rename_subobject(handle=new_handle, new_name=unreal.Text(component_name))
    subsystem.attach_subobject(owner_handle=root_handle, child_to_add_handle=new_handle)

    data_lib = unreal.SubobjectDataBlueprintFunctionLibrary
    subobject_data = data_lib.get_data(new_handle)
    component = data_lib.get_object(subobject_data)

    if not component:
        component = data_lib.get_object_for_blueprint(subobject_data, bp)

    return component


def get_generated_class(bp_path, bp):
    if hasattr(asset_lib, "load_blueprint_class"):
        cls = asset_lib.load_blueprint_class(bp_path)
        if cls:
            return cls

    if hasattr(unreal, "BlueprintEditorLibrary"):
        lib = unreal.BlueprintEditorLibrary
        if hasattr(lib, "get_blueprint_generated_class"):
            cls = lib.get_blueprint_generated_class(bp)
            if cls:
                return cls

    gen = getattr(bp, "generated_class", None)
    if callable(gen):
        return gen()

    if gen:
        return gen

    return None


def try_compile_blueprint(bp):
    if hasattr(unreal, "KismetEditorUtilities"):
        unreal.KismetEditorUtilities.compile_blueprint(bp)


def set_visibility_safe(comp, visible):
    if hasattr(comp, "set_visibility"):
        comp.set_visibility(visible, True)
        return

    try:
        comp.set_editor_property("visible", visible)
    except Exception as exc:
        unreal.log_warning(f"{comp.get_name()}: visible set failed: {exc}")


def set_hidden_in_game_safe(comp, hidden):
    if hasattr(comp, "set_hidden_in_game"):
        comp.set_hidden_in_game(hidden, True)
        return

    try:
        comp.set_editor_property("hidden_in_game", hidden)
    except Exception as exc:
        unreal.log_warning(f"{comp.get_name()}: hidden_in_game set failed: {exc}")


def set_collision_safe(comp, enabled):
    if hasattr(comp, "set_collision_enabled"):
        comp.set_collision_enabled(enabled)
        return

    if hasattr(comp, "set_collision_profile_name") and enabled == unreal.CollisionEnabled.NO_COLLISION:
        comp.set_collision_profile_name("NoCollision")
        return

    unreal.log_warning(f"{comp.get_name()}: collision setting method not found")


def set_static_mesh_component(comp, mesh, is_outline):
    comp.set_editor_property("static_mesh", mesh)
    comp.set_editor_property("relative_scale3d", OUTLINE_SCALE if is_outline else MAIN_SCALE)

    if is_outline:
        comp.set_material(0, outline_mat)
        set_collision_safe(comp, unreal.CollisionEnabled.NO_COLLISION)
        set_visibility_safe(comp, False)
        set_hidden_in_game_safe(comp, True)
    else:
        # Static Mesh 원본 Material 유지
        comp.set_editor_property("override_materials", [])


def find_scs_static_mesh_components(bp):
    result = []
    scs = None

    try:
        scs = bp.get_editor_property("simple_construction_script")
    except Exception:
        scs = getattr(bp, "simple_construction_script", None)

    if not scs:
        return result

    for node in scs.get_all_nodes():
        var_name = str(node.get_variable_name())
        template = node.get_editor_property("component_template")

        if isinstance(template, unreal.StaticMeshComponent):
            result.append((var_name, template.get_name(), template))

    return result


def set_bp_component_mesh(bp_path, bp, mesh):
    found_static = False
    found_outline = False

    # C++ inherited component 처리
    generated_class = get_generated_class(bp_path, bp)

    if generated_class:
        cdo = unreal.get_default_object(generated_class)
        cdo_comps = cdo.get_components_by_class(unreal.StaticMeshComponent)

        for comp in cdo_comps:
            comp_name = comp.get_name()
            unreal.log(f"{bp.get_name()} CDO component: {comp_name}")

            if is_static_component_name(comp_name):
                set_static_mesh_component(comp, mesh, False)
                found_static = True

            elif is_outline_component_name(comp_name):
                set_static_mesh_component(comp, mesh, True)
                found_outline = True
    else:
        unreal.log_warning(f"{bp_path}: generated class not found")

    # BP에서 추가한 SCS component 처리
    for var_name, template_name, template in find_scs_static_mesh_components(bp):
        unreal.log(f"{bp.get_name()} SCS var: {var_name}, template: {template_name}")

        if is_static_component_name(var_name) or is_static_component_name(template_name):
            set_static_mesh_component(template, mesh, False)
            found_static = True

        elif is_outline_component_name(var_name) or is_outline_component_name(template_name):
            set_static_mesh_component(template, mesh, True)
            found_outline = True

    if not found_static:
        comp = add_bp_component(bp, unreal.StaticMeshComponent, MAIN_COMPONENT_NAME)
        if comp:
            set_static_mesh_component(comp, mesh, False)
            found_static = True

    if not found_outline:
        comp = add_bp_component(bp, unreal.StaticMeshComponent, OUTLINE_COMPONENT_NAME)
        if comp:
            set_static_mesh_component(comp, mesh, True)
            found_outline = True

    return found_static, found_outline


mesh_assets = asset_registry.get_assets_by_path(SOURCE_MESH_DIR, recursive=True)

created = 0
updated = 0
skipped = 0
failed = 0

for asset_data in mesh_assets:
    mesh = asset_data.get_asset()

    if not isinstance(mesh, unreal.StaticMesh):
        continue

    mesh_name = mesh.get_name()

    # 001~479 같은 3자리 숫자 이름만 처리
    if not (len(mesh_name) == 3 and mesh_name.isdigit()):
        continue

    new_bp_path = f"{OUTPUT_BP_DIR}/BP_Prop_{mesh_name}"

    was_existing = asset_lib.does_asset_exist(new_bp_path)

    if was_existing:
        bp = asset_lib.load_asset(new_bp_path)
        if not bp:
            failed += 1
            unreal.log_warning(f"Load existing failed: {new_bp_path}")
            continue
        unreal.log(f"Updating existing: {new_bp_path}")
    else:
        bp = asset_lib.duplicate_asset(TEMPLATE_BP, new_bp_path)

        if not bp:
            failed += 1
            unreal.log_warning(f"Duplicate failed: {new_bp_path}")
            continue

    try:
        found_static, found_outline = set_bp_component_mesh(new_bp_path, bp, mesh)

        if not found_static:
            raise RuntimeError(f"{new_bp_path}: StaticMesh component not found")

        if not found_outline:
            raise RuntimeError(f"{new_bp_path}: Outline Mesh component not found")

        try_compile_blueprint(bp)
        asset_lib.save_loaded_asset(bp)

        if was_existing:
            updated += 1
            unreal.log(f"Updated: {new_bp_path}")
        else:
            created += 1
            unreal.log(f"Created: {new_bp_path}")
    except Exception as exc:
        failed += 1
        unreal.log_error(f"Failed: {new_bp_path}: {exc}")

unreal.log(f"Done. Created={created}, Updated={updated}, Skipped={skipped}, Failed={failed}")
