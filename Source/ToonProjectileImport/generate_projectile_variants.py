import unreal


ROOT = "/Game/ToonProjectile"
MATERIAL_ROOT = ROOT + "/Materials/Variants"
SYSTEM_ROOT = ROOT + "/Systems/Projectiles"
MASTER = ROOT + "/Materials/M_ToonProjectile_Additive"
TEX_ROOT = ROOT + "/HSFiles/Textures"
TEMPLATE = ROOT + "/NS_Projectile_01"


# Name, core texture, trail texture, spark texture, linear color, intensity
VARIANTS = [
    ("Magic", "Glow1", "Trail54", "Point12", (0.40, 0.10, 1.00), 18.0),
    ("Acid", "Circle116", "Trail55", "Point11", (0.20, 1.00, 0.08), 14.0),
    ("Torpedo", "Flare21", "Trail59", "Point19", (1.00, 0.35, 0.03), 18.0),
    ("Water", "HPwater2", "Trail68", "Point12", (0.04, 0.45, 1.00), 15.0),
    ("Star", "Star12", "Trail75", "SS1", (1.00, 0.60, 0.05), 22.0),
    ("BlackHole", "Moon1", "Trail54", "Point19", (0.45, 0.05, 1.00), 16.0),
    ("Fire", "Flare17", "Trail59", "Point11", (1.00, 0.08, 0.01), 24.0),
    ("Triangle", "Triangle1", "Trail55", "Point12", (0.10, 1.00, 0.65), 16.0),
    ("Thunder", "Lightning11", "Trail68", "Point19", (0.10, 0.45, 1.00), 25.0),
    ("Green", "Aura2", "Trail55", "Point11", (0.05, 1.00, 0.25), 17.0),
    ("Bullets", "Flash21", "Trail54", "Point12", (1.00, 0.45, 0.05), 18.0),
    ("Blue", "Aura8", "Trail68", "Point19", (0.03, 0.20, 1.00), 20.0),
    ("StarRainbow", "CircleRainbow15", "Trail75", "Star12", (1.00, 0.20, 0.55), 22.0),
    ("Huricara", "Aura1", "Trail59", "Point11", (0.65, 0.10, 1.00), 20.0),
    ("Arrow", "Flare15", "Trail54", "Point12", (0.20, 0.75, 1.00), 18.0),
]


def load_texture(name):
    candidates = [
        TEX_ROOT + "/" + name,
        TEX_ROOT + "/MeshTextures/" + name,
    ]
    for path in candidates:
        asset = unreal.EditorAssetLibrary.load_asset(path)
        if asset:
            return asset
    unreal.log_warning("Missing texture: " + name)
    return None


def make_instance(asset_name, texture_name, color, intensity):
    path = MATERIAL_ROOT + "/" + asset_name
    instance = unreal.EditorAssetLibrary.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    if not instance:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            asset_name, MATERIAL_ROOT, unreal.MaterialInstanceConstant, factory
        )
        instance.set_editor_property("parent", unreal.load_asset(MASTER))

    texture = load_texture(texture_name)
    if texture:
        unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
            instance, "Texture", texture
        )
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        instance, "Tint", unreal.LinearColor(color[0], color[1], color[2], 1.0)
    )
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, "Intensity", intensity
    )
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def duplicate_system(index, label):
    name = "NS_Projectile_{:02d}_{}".format(index, label)
    destination = SYSTEM_ROOT + "/" + name
    if unreal.EditorAssetLibrary.does_asset_exist(destination):
        unreal.log("Keeping existing system: " + destination)
        return
    if not unreal.EditorAssetLibrary.duplicate_asset(TEMPLATE, destination):
        raise RuntimeError("Could not duplicate Niagara template to " + destination)
    unreal.EditorAssetLibrary.save_asset(destination, only_if_is_dirty=False)


def main():
    if not unreal.EditorAssetLibrary.does_asset_exist(TEMPLATE):
        raise RuntimeError("Template not found: " + TEMPLATE)
    for index, (label, core, trail, sparks, color, intensity) in enumerate(VARIANTS, 1):
        duplicate_system(index, label)
        make_instance("MI_P{:02d}_Core_{}".format(index, label), core, color, intensity)
        make_instance("MI_P{:02d}_Trail_{}".format(index, label), trail, color, intensity * 0.65)
        make_instance("MI_P{:02d}_Sparks_{}".format(index, label), sparks, color, intensity * 0.8)
    unreal.EditorAssetLibrary.save_directory(ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log("TOON_AUTOMATION_COMPLETE systems=15 material_instances=45")


main()
