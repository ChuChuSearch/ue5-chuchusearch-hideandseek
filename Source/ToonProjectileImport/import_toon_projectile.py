import os
import unreal


SOURCE_ROOT = os.path.join(os.path.dirname(__file__), "Raw")
DEST_ROOT = "/Game/ToonProjectile"


def normalized_name(value):
    return value.replace(" ", "_").replace("-", "_")


def import_sources():
    tasks = []
    for root, _, files in os.walk(SOURCE_ROOT):
        for filename in files:
            extension = os.path.splitext(filename)[1].lower()
            if extension not in (".png", ".exr", ".fbx"):
                continue

            source = os.path.join(root, filename)
            relative = os.path.relpath(root, SOURCE_ROOT).replace("\\", "/")
            destination = DEST_ROOT if relative == "." else DEST_ROOT + "/" + "/".join(
                normalized_name(part) for part in relative.split("/")
            )

            task = unreal.AssetImportTask()
            task.filename = source
            task.destination_path = destination
            task.destination_name = normalized_name(os.path.splitext(filename)[0])
            task.automated = True
            task.replace_existing = True
            task.save = True

            if extension == ".fbx":
                options = unreal.FbxImportUI()
                options.import_mesh = True
                options.import_as_skeletal = False
                options.import_materials = False
                options.import_textures = False
                options.static_mesh_import_data.combine_meshes = True
                options.static_mesh_import_data.generate_lightmap_u_vs = False
                task.options = options

            tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log("ToonProjectile: imported {} source assets".format(len(tasks)))


def make_master_material(name, blend_mode):
    package_path = DEST_ROOT + "/Materials"
    full_path = package_path + "/" + name
    existing = unreal.EditorAssetLibrary.load_asset(full_path)
    if existing:
        return existing

    factory = unreal.MaterialFactoryNew()
    material = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, package_path, unreal.Material, factory
    )
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("blend_mode", blend_mode)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    material.set_editor_property("two_sided", True)

    texture = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionTextureSampleParameter2D, -420, -80
    )
    texture.set_editor_property("parameter_name", "Texture")

    tint = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionVectorParameter, -420, 120
    )
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(1.0, 1.0, 1.0, 1.0))

    intensity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionScalarParameter, -420, 260
    )
    intensity.set_editor_property("parameter_name", "Intensity")
    intensity.set_editor_property("default_value", 10.0)

    multiply_color = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, -160, 20
    )
    multiply_intensity = unreal.MaterialEditingLibrary.create_material_expression(
        material, unreal.MaterialExpressionMultiply, 40, 20
    )

    unreal.MaterialEditingLibrary.connect_material_expressions(texture, "RGB", multiply_color, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(tint, "", multiply_color, "B")
    unreal.MaterialEditingLibrary.connect_material_expressions(multiply_color, "", multiply_intensity, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(intensity, "", multiply_intensity, "B")
    unreal.MaterialEditingLibrary.connect_material_property(
        multiply_intensity, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
    )
    unreal.MaterialEditingLibrary.connect_material_property(
        texture, "A", unreal.MaterialProperty.MP_OPACITY
    )
    unreal.MaterialEditingLibrary.layout_material_expressions(material)
    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def main():
    import_sources()
    make_master_material("M_ToonProjectile_Additive", unreal.BlendMode.BLEND_ADDITIVE)
    make_master_material("M_ToonProjectile_Translucent", unreal.BlendMode.BLEND_TRANSLUCENT)
    unreal.EditorAssetLibrary.save_directory(DEST_ROOT, only_if_is_dirty=False, recursive=True)
    unreal.log("ToonProjectile: conversion base assets completed")


main()
