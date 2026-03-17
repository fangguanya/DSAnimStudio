"""
Skill Config Import Script for Sekiro Skill Editor
Reads skill_config.json files and creates USekiroSkillDataAsset instances.

Usage:
    import import_skill_configs
    import_skill_configs.import_character_skills("c0000", "E:/Export/Characters/c0000")
"""

import unreal
import os
import json


def create_skill_data_asset(chr_id, anim_name, anim_data, output_path):
    """Create a USekiroSkillDataAsset from parsed animation data."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    # Create the data asset
    factory = unreal.DataAssetFactory()
    asset_name = f"SKD_{chr_id}_{anim_name}"
    
    asset = asset_tools.create_asset(
        asset_name, output_path,
        unreal.load_class(None, "/Script/SekiroSkillEditorPlugin.SekiroSkillDataAsset"),
        factory)
    
    if not asset:
        # Fallback: try loading if already exists
        existing_path = f"{output_path}/{asset_name}.{asset_name}"
        asset = unreal.load_asset(f"{output_path}/{asset_name}")
        if not asset:
            unreal.log_warning(f"Failed to create skill data asset: {asset_name}")
            return None
    
    # Set properties
    asset.set_editor_property('character_id', chr_id)
    asset.set_editor_property('animation_name', anim_name)
    asset.set_editor_property('frame_count', anim_data.get('frameCount', 0))
    asset.set_editor_property('frame_rate', anim_data.get('frameRate', 30.0))
    
    # Create events
    events = []
    for evt_data in anim_data.get('events', []):
        evt = unreal.SekiroTaeEvent()
        evt.set_editor_property('type', evt_data.get('type', 0))
        evt.set_editor_property('type_name', evt_data.get('typeName', 'Unknown'))
        evt.set_editor_property('category', evt_data.get('category', 'Unknown'))
        evt.set_editor_property('start_frame', evt_data.get('startFrame', 0.0))
        evt.set_editor_property('end_frame', evt_data.get('endFrame', 0.0))
        
        # Parameters as string map
        params = {}
        for k, v in evt_data.get('parameters', {}).items():
            params[k] = str(v)
        evt.set_editor_property('parameters', params)
        
        events.append(evt)
    
    asset.set_editor_property('events', events)
    
    # Save
    unreal.EditorAssetLibrary.save_asset(f"{output_path}/{asset_name}")
    
    return asset


def import_character_skills(chr_id, export_dir,
                            base_content_path="/Game/SekiroAssets/Characters"):
    """
    Import skill configs for a single character.
    
    Args:
        chr_id: Character ID (e.g., "c0000")
        export_dir: Path to the character's export directory
        base_content_path: Base UE content path
    """
    skill_config_path = os.path.join(export_dir, "Skills", "skill_config.json")
    if not os.path.exists(skill_config_path):
        unreal.log_warning(f"Skill config not found: {skill_config_path}")
        return []
    
    with open(skill_config_path, 'r', encoding='utf-8') as f:
        config = json.load(f)
    
    output_path = f"{base_content_path}/{chr_id}/Skills"
    assets_created = []
    
    # Navigate the JSON structure
    characters = config.get("characters", {})
    char_data = characters.get(chr_id, {})
    animations = char_data.get("animations", {})
    
    if not animations:
        # Try flat structure (direct animations at root)
        animations = config.get("animations", {})
    
    for anim_name, anim_data in animations.items():
        asset = create_skill_data_asset(chr_id, anim_name, anim_data, output_path)
        if asset:
            assets_created.append(asset)
    
    unreal.log(f"Imported {len(assets_created)} skill configs for {chr_id}")
    return assets_created


def import_all_skills(export_root, base_content_path="/Game/SekiroAssets/Characters"):
    """Import skill configs for all exported characters."""
    if not os.path.isdir(export_root):
        unreal.log_error(f"Export directory not found: {export_root}")
        return
    
    chr_dirs = sorted([d for d in os.listdir(export_root)
                       if os.path.isdir(os.path.join(export_root, d))])
    
    total = 0
    for chr_id in chr_dirs:
        chr_export_dir = os.path.join(export_root, chr_id)
        skills_dir = os.path.join(chr_export_dir, "Skills")
        if os.path.isdir(skills_dir):
            assets = import_character_skills(chr_id, chr_export_dir, base_content_path)
            total += len(assets)
    
    unreal.log(f"Total skill configs imported: {total}")


def import_full_pipeline(export_root, base_content_path="/Game/SekiroAssets/Characters"):
    """
    Run the complete import pipeline:
    1. Import FBX models and animations (batch_import)
    2. Setup materials (setup_materials)
    3. Import skill configs
    """
    import batch_import
    import setup_materials
    
    unreal.log("=== Starting Full Sekiro Asset Import Pipeline ===")
    
    # Step 1: Import FBX assets
    unreal.log("Step 1/3: Importing FBX models and animations...")
    batch_import.import_all(export_root, base_content_path)
    
    # Step 2: Setup materials
    unreal.log("Step 2/3: Setting up materials...")
    setup_materials.setup_all_materials(export_root, base_content_path)
    
    # Step 3: Import skill configs
    unreal.log("Step 3/3: Importing skill configurations...")
    import_all_skills(export_root, base_content_path)
    
    unreal.log("=== Full Import Pipeline Complete ===")
