import json
from pathlib import Path

import unreal


MESH_PATH = "/Game/SekiroAssets/SelfTest/c0000/Mesh/c0000"
SKELETON_PATH = "/Game/SekiroAssets/SelfTest/c0000/Mesh/c0000_Skeleton"
ANIMATION_PATH = "/Game/SekiroAssets/SelfTest/c0000/Animations/a000_201030"
OUTPUT_PATH = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\SelfTestImportDiagnostic\selftest_import_probe.json")
REQUIRED_BONES = [
    "Master",
    "Pelvis",
    "Spine",
    "Spine1",
    "Spine2",
    "Neck",
    "Head",
    "L_Shoulder",
    "R_Shoulder",
    "L_Hand",
    "R_Hand",
]


def parse_json(json_text: str):
    parsed = json.loads(json_text)
    if "error" in parsed:
        raise RuntimeError(parsed["error"])
    return parsed


def load_required_asset(asset_path: str):
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"missing asset: {asset_path}")
    return asset


def resolve_asset_skeleton(asset):
    skeleton = None
    try:
        skeleton = asset.skeleton
    except Exception:
        skeleton = None

    if skeleton is None and hasattr(asset, "get_editor_property"):
        try:
            skeleton = asset.get_editor_property("skeleton")
        except Exception:
            skeleton = None

    return skeleton


mesh = load_required_asset(MESH_PATH)
skeleton = load_required_asset(SKELETON_PATH)
animation = load_required_asset(ANIMATION_PATH)
reference_pose = parse_json(
    unreal.SekiroRetargetDiagnosticsLibrary.dump_skeletal_mesh_reference_pose_to_json(
        mesh,
        [unreal.Name(bone_name) for bone_name in REQUIRED_BONES],
    )
)

physics_asset = None
try:
    physics_asset = mesh.get_editor_property("physics_asset")
except Exception:
    physics_asset = None

result = {
    "meshPath": MESH_PATH,
    "skeletonPath": SKELETON_PATH,
    "animationPath": ANIMATION_PATH,
    "assets": {
        "meshLoaded": mesh is not None,
        "skeletonLoaded": skeleton is not None,
        "animationLoaded": animation is not None,
        "meshUsesSkeleton": resolve_asset_skeleton(mesh) == skeleton,
        "animationUsesSkeleton": resolve_asset_skeleton(animation) == skeleton,
        "physicsAssetPath": physics_asset.get_path_name() if physics_asset else "",
    },
    "bones": {},
}

for entry in reference_pose["bones"]:
    result["bones"][entry["bone"]] = {
        "boneIndex": entry["boneIndex"],
        "parentBone": entry["parentBone"],
        "componentTranslation": entry["refComponent"]["translation"],
        "componentRotation": entry["refComponent"]["rotation"],
        "localTranslation": entry["refLocal"]["translation"],
        "localRotation": entry["refLocal"]["rotation"],
    }

OUTPUT_PATH.parent.mkdir(parents=True, exist_ok=True)
OUTPUT_PATH.write_text(json.dumps(result, indent=2), encoding="utf-8")
unreal.log_warning(f"wrote import self-test probe: {OUTPUT_PATH}")