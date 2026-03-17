"""
Batch Import Script for Sekiro Skill Editor
Supports FBX, glTF (via Interchange), and Collada (DAE) files.

Usage:
    import batch_import
    batch_import.import_all("E:/Sekiro/Export")
"""

import unreal
import os
import json


def import_model(file_path, destination_path, asset_name=None):
    """Import a single model (FBX, glTF, or DAE) as SkeletalMesh."""
    if asset_name is None:
        asset_name = os.path.splitext(os.path.basename(file_path))[0]

    ext = os.path.splitext(file_path)[1].lower()

    task = unreal.AssetImportTask()
    task.set_editor_property('filename', file_path)
    task.set_editor_property('destination_path', destination_path)
    task.set_editor_property('destination_name', asset_name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)

    if ext in ('.fbx',):
        options = unreal.FbxImportUI()
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_as_skeletal', True)
        options.set_editor_property('import_animations', False)
        options.set_editor_property('import_materials', True)
        options.set_editor_property('import_textures', False)
        options.set_editor_property('create_physics_asset', False)

        sk_options = options.get_editor_property('skeletal_mesh_import_data')
        sk_options.set_editor_property('import_morph_targets', False)
        sk_options.set_editor_property('update_skeleton_reference_pose', True)

        task.set_editor_property('options', options)
    # For glTF/glb/DAE: UE5 Interchange framework handles it automatically
    # No special FbxImportUI options needed - Interchange detects skeleton/skin

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = task.get_editor_property('imported_object_paths')
    if imported:
        unreal.log(f"Imported model: {imported[0]}")
        return imported[0]
    else:
        # Check if objects were imported (newer API)
        results = task.get_editor_property('result')
        if results:
            unreal.log(f"Imported model (via result): {results}")
            return str(results)
        unreal.log_warning(f"Failed to import model: {file_path}")
        return None


def import_animation(file_path, skeleton_path, destination_path):
    """Import a single animation.
    For glTF: uses the C++ commandlet's custom parser (run SekiroImport commandlet instead).
    For FBX: uses FbxImportUI with skeleton binding.
    For DAE: uses FbxImportUI (limited support).
    """
    asset_name = os.path.splitext(os.path.basename(file_path))[0]
    ext = os.path.splitext(file_path)[1].lower()

    # glTF animations must use the commandlet's custom parser
    # Interchange doesn't bind animations to an existing skeleton
    if ext in ('.gltf', '.glb'):
        unreal.log_warning(f"  Skipping glTF animation {asset_name} - use SekiroImport commandlet instead")
        return None

    task = unreal.AssetImportTask()
    task.set_editor_property('filename', file_path)
    task.set_editor_property('destination_path', destination_path)
    task.set_editor_property('destination_name', asset_name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)

    options = unreal.FbxImportUI()
    options.set_editor_property('import_mesh', False)
    options.set_editor_property('import_animations', True)
    options.set_editor_property('import_as_skeletal', True)

    skeleton = unreal.load_asset(skeleton_path)
    if skeleton:
        options.set_editor_property('skeleton', skeleton)

    anim_options = options.get_editor_property('anim_sequence_import_data')
    anim_options.set_editor_property('import_bone_tracks', True)
    anim_options.set_editor_property('remove_redundant_keys', False)

    task.set_editor_property('options', options)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = task.get_editor_property('imported_object_paths')
    if imported:
        return imported[0]
    return None


def import_texture(texture_path, destination_path):
    """Import a single texture (PNG/DDS/TGA)."""
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', texture_path)
    task.set_editor_property('destination_path', destination_path)
    task.set_editor_property('destination_name',
                             os.path.splitext(os.path.basename(texture_path))[0])
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    imported = task.get_editor_property('imported_object_paths')
    if imported:
        return imported[0]
    return None


def find_model_file(model_dir, chr_id):
    """Find the best model file for a character. Priority: FBX > glTF > GLB > DAE"""
    for ext in ('.fbx', '.gltf', '.glb', '.dae'):
        path = os.path.join(model_dir, f"{chr_id}{ext}")
        if os.path.exists(path):
            return path
    # Fallback: first mesh file in directory
    if os.path.isdir(model_dir):
        for ext in ('.fbx', '.gltf', '.glb', '.dae'):
            for f in sorted(os.listdir(model_dir)):
                if f.lower().endswith(ext) and not f.startswith('.'):
                    return os.path.join(model_dir, f)
    return None


def find_animation_files(anim_dir):
    """Find animation files, deduplicated by base name. Priority: FBX > DAE (glTF skipped in Python)."""
    if not os.path.isdir(anim_dir):
        return []

    # For Python/editor path: prefer FBX/DAE (glTF needs commandlet's custom parser)
    priority = {'.fbx': 0, '.dae': 1}
    best_files = {}  # base_name -> (priority, full_path)

    for f in os.listdir(anim_dir):
        fl = f.lower()
        name, ext = os.path.splitext(fl)
        if ext in priority:
            p = priority[ext]
            if name not in best_files or p < best_files[name][0]:
                best_files[name] = (p, os.path.join(anim_dir, f))

    return [path for _, path in sorted(best_files.values(), key=lambda x: x[1])]


def find_skeleton_from_mesh(content_path):
    """Find the skeleton asset from an imported SkeletalMesh."""
    registry = unreal.AssetRegistryHelpers.get_asset_registry()

    # Search for SkeletalMesh assets in the path
    assets = registry.get_assets_by_path(content_path, recursive=True)
    for asset_data in assets:
        asset = asset_data.get_asset()
        if isinstance(asset, unreal.SkeletalMesh):
            skeleton = asset.get_editor_property('skeleton')
            if skeleton:
                return skeleton.get_path_name()

    # Search for Skeleton assets directly
    for asset_data in assets:
        asset = asset_data.get_asset()
        if isinstance(asset, unreal.Skeleton):
            return asset.get_path_name()

    return None


def import_character(export_dir, chr_id, base_content_path="/Game/SekiroAssets/Characters"):
    """Import all assets for a single character."""
    chr_content_path = f"{base_content_path}/{chr_id}"
    results = {"model": None, "skeleton": None, "animations": [], "textures": []}

    # 1. Import textures first (needed for material setup)
    tex_dir = os.path.join(export_dir, "Textures")
    if os.path.isdir(tex_dir):
        tex_dest = f"{chr_content_path}/Textures"
        for tex_file in sorted(os.listdir(tex_dir)):
            if tex_file.lower().endswith(('.png', '.tga', '.dds', '.bmp')):
                tex_path = os.path.join(tex_dir, tex_file)
                result = import_texture(tex_path, tex_dest)
                if result:
                    results["textures"].append(result)
        unreal.log(f"  Textures: {len(results['textures'])} imported")

    # 2. Import model
    model_dir = os.path.join(export_dir, "Model")
    model_file = find_model_file(model_dir, chr_id)
    if model_file:
        result = import_model(model_file, f"{chr_content_path}/Mesh")
        if result:
            results["model"] = result
            # Try to find skeleton from imported skeletal mesh
            skeleton_path = find_skeleton_from_mesh(f"{chr_content_path}/Mesh")
            if skeleton_path:
                results["skeleton"] = skeleton_path
                unreal.log(f"  Model: {chr_id} -> SkeletalMesh (skeleton: {skeleton_path})")
            else:
                unreal.log(f"  Model: {chr_id} -> {result} (no skeleton found)")

    # 3. Import animations (only if we have a skeleton)
    if results.get("skeleton"):
        anim_dir = os.path.join(export_dir, "Animations")
        anim_files = find_animation_files(anim_dir)
        if anim_files:
            anim_dest = f"{chr_content_path}/Animations"
            for anim_file in anim_files:
                result = import_animation(anim_file, results["skeleton"], anim_dest)
                if result:
                    results["animations"].append(result)
            unreal.log(f"  Animations: {len(results['animations'])} imported")
    else:
        unreal.log_warning(f"  Animations: SKIPPED (no skeleton)")

    # 4. Copy skill config
    skill_dir = os.path.join(export_dir, "Skills")
    skill_src = os.path.join(skill_dir, "skill_config.json")
    if os.path.exists(skill_src):
        skill_dst_dir = os.path.join(
            unreal.Paths.project_content_dir(),
            "SekiroAssets", "Characters", chr_id, "Skills"
        )
        os.makedirs(skill_dst_dir, exist_ok=True)
        import shutil
        shutil.copy2(skill_src, os.path.join(skill_dst_dir, "skill_config.json"))
        unreal.log(f"  Skills: copied skill_config.json")

    return results


def import_all(export_root, base_content_path="/Game/SekiroAssets/Characters"):
    """
    Import all exported characters from the export root directory.

    Args:
        export_root: Path to the export directory (e.g., "E:/Sekiro/Export")
        base_content_path: UE content path for imported assets
    """
    if not os.path.isdir(export_root):
        unreal.log_error(f"Export directory not found: {export_root}")
        return

    # Find character directories (those starting with 'c')
    chr_dirs = sorted([d for d in os.listdir(export_root)
                       if os.path.isdir(os.path.join(export_root, d))
                       and d.startswith('c')])

    unreal.log(f"Found {len(chr_dirs)} character(s) to import")

    all_results = {}
    total_models = 0
    total_skeletal = 0
    total_textures = 0
    total_anims = 0

    for i, chr_id in enumerate(chr_dirs):
        unreal.log(f"[{i+1}/{len(chr_dirs)}] Importing {chr_id}...")
        chr_export_dir = os.path.join(export_root, chr_id)
        results = import_character(chr_export_dir, chr_id, base_content_path)
        all_results[chr_id] = results

        if results["model"]:
            total_models += 1
        if results.get("skeleton"):
            total_skeletal += 1
        total_textures += len(results["textures"])
        total_anims += len(results["animations"])

    # Save all
    unreal.EditorAssetLibrary.save_directory(base_content_path)
    unreal.log(f"=== Import Complete ===")
    unreal.log(f"  Characters: {len(all_results)}")
    unreal.log(f"  Models: {total_models} ({total_skeletal} with skeleton)")
    unreal.log(f"  Textures: {total_textures}")
    unreal.log(f"  Animations: {total_anims}")
    return all_results
