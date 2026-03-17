"""
Test import c1020 - textures + model + a few animations
"""
import sys, os

script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

import unreal

EXPORT_ROOT = "E:/Sekiro/Export"
CONTENT_BASE = "/Game/SekiroAssets/Characters"
CHR_ID = "c1020"
MODEL_EXTENSIONS = ('.fbx', '.dae')
TEXTURE_EXTENSIONS = ('.png', '.tga', '.dds')


def import_asset(file_path, dest_path, dest_name=None):
    """Import using AssetImportTask."""
    if dest_name is None:
        dest_name = os.path.splitext(os.path.basename(file_path))[0]
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', file_path)
    task.set_editor_property('destination_path', dest_path)
    task.set_editor_property('destination_name', dest_name)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('automated', True)
    task.set_editor_property('save', True)
    try:
        unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
        imported = task.get_editor_property('imported_object_paths')
        if imported:
            return str(imported[0])
    except Exception as e:
        unreal.log_warning(f"  Exception: {e}")
    return None


chr_dir = os.path.join(EXPORT_ROOT, CHR_ID)
chr_content = f"{CONTENT_BASE}/{CHR_ID}"
unreal.log(f"=== Test Import: {CHR_ID} ===")

# 1. Textures
tex_dir = os.path.join(chr_dir, "Textures")
tex_count = 0
if os.path.isdir(tex_dir):
    for f in sorted(os.listdir(tex_dir)):
        if f.lower().endswith(TEXTURE_EXTENSIONS):
            r = import_asset(os.path.join(tex_dir, f), f"{chr_content}/Textures")
            if r:
                tex_count += 1
                unreal.log(f"  TEX: {r}")
unreal.log(f"Textures imported: {tex_count}")

# 2. Model
model_dir = os.path.join(chr_dir, "Model")
model_result = None
skeleton_path = None
if os.path.isdir(model_dir):
    for f in os.listdir(model_dir):
        if f.lower().endswith(MODEL_EXTENSIONS):
            unreal.log(f"  Importing model: {f}")
            model_result = import_asset(os.path.join(model_dir, f), f"{chr_content}/Mesh")
            if model_result:
                unreal.log(f"  MODEL: {model_result}")
                mesh = unreal.load_asset(model_result)
                if mesh:
                    try:
                        skel = mesh.get_editor_property('skeleton')
                        if skel:
                            skeleton_path = skel.get_path_name()
                            unreal.log(f"  SKELETON: {skeleton_path}")
                    except Exception as e:
                        unreal.log_warning(f"  Skeleton access error: {e}")
            break
unreal.log(f"Model: {'OK' if model_result else 'FAIL'}")

# 3. A few animations
anim_dir = os.path.join(chr_dir, "Animations")
anim_count = 0
if os.path.isdir(anim_dir):
    anim_files = sorted([f for f in os.listdir(anim_dir) if f.lower().endswith(MODEL_EXTENSIONS)])
    unreal.log(f"Found {len(anim_files)} animation files")
    for f in anim_files[:3]:
        unreal.log(f"  Importing anim: {f}")
        r = import_asset(os.path.join(anim_dir, f), f"{chr_content}/Animations")
        if r:
            anim_count += 1
            unreal.log(f"  ANIM: {r}")
    unreal.log(f"Animations imported: {anim_count}/3 (of {len(anim_files)} total)")

unreal.log(f"=== DONE: tex={tex_count}, model={'OK' if model_result else 'FAIL'}, anims={anim_count} ===")
