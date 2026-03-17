"""
Material Setup Script for Sekiro Skill Editor
Reads material_manifest.json and auto-creates Material Instances with correct texture assignments.

Usage:
    import setup_materials
    setup_materials.setup_character_materials("c0000", "E:/Export/Characters/c0000")
"""

import unreal
import os
import json


# Texture type to UE material parameter name mapping
TEXTURE_PARAM_MAP = {
    "BaseColor": "BaseColor",
    "Normal": "Normal",
    "Specular": "Specular",
    "Metallic": "Metallic",
    "Roughness": "Roughness",
    "Emissive": "Emissive",
    "AmbientOcclusion": "AmbientOcclusion",
    "BlendMask": "BlendMask",
}


def find_texture_asset(texture_name, search_path):
    """Find a texture asset in the content browser by name."""
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    
    # Search in the specified path
    assets = ar.get_assets_by_path(search_path, recursive=True)
    for asset_data in assets:
        if asset_data.asset_name == texture_name:
            return asset_data.get_asset()
    
    # Try without extension
    base_name = os.path.splitext(texture_name)[0]
    for asset_data in assets:
        if asset_data.asset_name == base_name:
            return asset_data.get_asset()
    
    return None


def create_material_instance(mat_name, parent_material_path, texture_assignments, output_path):
    """Create a Material Instance with texture assignments."""
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    # Create material instance
    factory = unreal.MaterialInstanceConstantFactoryNew()
    parent_mat = unreal.load_asset(parent_material_path)
    if parent_mat:
        factory.set_editor_property('initial_parent', parent_mat)
    
    mi = asset_tools.create_asset(
        mat_name, output_path, unreal.MaterialInstanceConstant, factory)
    
    if not mi:
        unreal.log_warning(f"Failed to create material instance: {mat_name}")
        return None
    
    # Assign textures
    for param_name, texture in texture_assignments.items():
        if texture:
            param = unreal.TextureParameterValue()
            param.set_editor_property('parameter_info',
                unreal.MaterialParameterInfo(name=param_name))
            param.set_editor_property('parameter_value', texture)
            
            current_overrides = list(mi.get_editor_property('texture_parameter_values'))
            current_overrides.append(param)
            mi.set_editor_property('texture_parameter_values', current_overrides)
    
    return mi


def setup_character_materials(chr_id, export_dir,
                              base_content_path="/Game/SekiroAssets/Characters",
                              parent_material="/Engine/EngineMaterials/DefaultLitMaterial"):
    """
    Set up materials for a character from material_manifest.json.
    
    Args:
        chr_id: Character ID (e.g., "c0000")
        export_dir: Path to the character's export directory
        base_content_path: Base UE content path
        parent_material: Parent material path for instances
    """
    manifest_path = os.path.join(export_dir, "Model", "material_manifest.json")
    if not os.path.exists(manifest_path):
        unreal.log_warning(f"Material manifest not found: {manifest_path}")
        return []
    
    with open(manifest_path, 'r', encoding='utf-8') as f:
        manifest = json.load(f)
    
    chr_content = f"{base_content_path}/{chr_id}"
    tex_content = f"{chr_content}/Textures"
    mat_content = f"{chr_content}/Materials"
    
    materials_created = []
    
    for mat_entry in manifest.get("materials", []):
        mat_name = mat_entry.get("name", "UnknownMaterial")
        safe_name = f"MI_{mat_name}".replace(" ", "_").replace("|", "_")
        
        # Resolve textures
        texture_assignments = {}
        for tex_entry in mat_entry.get("textures", []):
            tex_type = tex_entry.get("semanticType", "")
            exported_file = tex_entry.get("exportedFileName", "")
            
            if tex_type in TEXTURE_PARAM_MAP and exported_file:
                param_name = TEXTURE_PARAM_MAP[tex_type]
                tex_name = os.path.splitext(exported_file)[0]
                texture = find_texture_asset(tex_name, tex_content)
                if texture:
                    texture_assignments[param_name] = texture
                else:
                    unreal.log_warning(f"  Texture not found: {tex_name} for {mat_name}")
        
        mi = create_material_instance(safe_name, parent_material,
                                       texture_assignments, mat_content)
        if mi:
            materials_created.append(mi)
            unreal.log(f"  Created material: {safe_name} ({len(texture_assignments)} textures)")
    
    unreal.log(f"Materials setup complete for {chr_id}: {len(materials_created)} materials created")
    return materials_created


def setup_all_materials(export_root, base_content_path="/Game/SekiroAssets/Characters"):
    """Set up materials for all exported characters."""
    if not os.path.isdir(export_root):
        unreal.log_error(f"Export directory not found: {export_root}")
        return
    
    chr_dirs = sorted([d for d in os.listdir(export_root)
                       if os.path.isdir(os.path.join(export_root, d))])
    
    for chr_id in chr_dirs:
        chr_export_dir = os.path.join(export_root, chr_id)
        setup_character_materials(chr_id, chr_export_dir, base_content_path)
