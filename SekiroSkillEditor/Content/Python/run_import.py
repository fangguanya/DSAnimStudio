"""
Master import script - run via UE5 commandlet:
    UnrealEditor-Cmd.exe SekiroSkillEditor.uproject -run=pythonscript -script="Content/Python/run_import.py"

Or from UE5 Python console:
    exec(open("E:/Sekiro/SekiroSkillEditor/Content/Python/run_import.py").read())
"""

import sys
import os

# Add our script directory to path
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.insert(0, script_dir)

import unreal
import batch_import
import setup_materials
import import_skill_configs

EXPORT_ROOT = "E:/Sekiro/Export"
CONTENT_BASE = "/Game/SekiroAssets/Characters"

def run():
    unreal.log("=" * 60)
    unreal.log("=== Sekiro Full Asset Import Pipeline ===")
    unreal.log(f"=== Source: {EXPORT_ROOT}")
    unreal.log(f"=== Destination: {CONTENT_BASE}")
    unreal.log("=" * 60)

    # Step 1: Import models, textures, and animations
    unreal.log("")
    unreal.log("=== STEP 1/3: Importing models, textures, and animations ===")
    results = batch_import.import_all(EXPORT_ROOT, CONTENT_BASE)

    # Step 2: Setup materials
    unreal.log("")
    unreal.log("=== STEP 2/3: Setting up materials ===")
    setup_materials.setup_all_materials(EXPORT_ROOT, CONTENT_BASE)

    # Step 3: Import skill configs
    unreal.log("")
    unreal.log("=== STEP 3/3: Importing skill configurations ===")
    import_skill_configs.import_all_skills(EXPORT_ROOT, CONTENT_BASE)

    unreal.log("")
    unreal.log("=" * 60)
    unreal.log("=== FULL IMPORT PIPELINE COMPLETE ===")
    unreal.log("=" * 60)

# Run
run()
