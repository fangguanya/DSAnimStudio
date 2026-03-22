import json
from pathlib import Path

import unreal


SOURCE_MESH_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Mesh/c0000"
TARGET_MESH_PATH = "/Game/GhostSamurai_Bundle/Demo/Characters/Mannequins/Meshes/SKM_Manny"
RETARGETER_PATH = "/Game/Retargeted/SekiroToManny/RTG_SekiroToManny"
SOURCE_ANIMATION_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Animations/a000_202300"
TARGET_ANIMATION_PATH = "/Game/Retargeted/SekiroToManny/Animations/a000_202300_Manny"
OUTPUT_DIR = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\HandRetargetDiagnostic")

SOURCE_HAND_ROOTS = ["L_Hand", "R_Hand"]
TARGET_HAND_ROOTS = ["hand_l", "hand_r"]
SOURCE_SAMPLE_BONES = [
    "L_Shoulder",
    "L_UpperArm",
    "L_Forearm",
    "L_Hand",
    "L_Finger0",
    "L_Finger1",
    "L_Finger2",
    "L_Finger3",
    "L_Finger4",
    "R_Shoulder",
    "R_UpperArm",
    "R_Forearm",
    "R_Hand",
    "R_Finger0",
    "R_Finger1",
    "R_Finger2",
    "R_Finger3",
    "R_Finger4",
]
TARGET_SAMPLE_BONES = [
    "clavicle_l",
    "upperarm_l",
    "lowerarm_l",
    "hand_l",
    "thumb_01_l",
    "index_metacarpal_l",
    "middle_metacarpal_l",
    "ring_metacarpal_l",
    "pinky_metacarpal_l",
    "clavicle_r",
    "upperarm_r",
    "lowerarm_r",
    "hand_r",
    "thumb_01_r",
    "index_metacarpal_r",
    "middle_metacarpal_r",
    "ring_metacarpal_r",
    "pinky_metacarpal_r",
]
FRAMES = [0, 5, 10, 15, 20, 25, 30]


def load_required_asset(asset_path: str):
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"missing asset: {asset_path}")
    return asset


def load_optional_asset(asset_path: str):
    return unreal.load_asset(asset_path)


def parse_json(json_text: str):
    parsed = json.loads(json_text)
    if "error" in parsed:
        raise RuntimeError(parsed["error"])
    return parsed


def dump_json(file_name: str, data) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    output_path = OUTPUT_DIR / file_name
    output_path.write_text(json.dumps(data, indent=2), encoding="utf-8")
    unreal.log_warning(f"wrote diagnostic file: {output_path}")


def get_skeleton(asset) -> unreal.Skeleton:
    if isinstance(asset, unreal.SkeletalMesh):
        skeleton = asset.skeleton
    elif isinstance(asset, unreal.AnimSequence):
        skeleton = asset.skeleton
    else:
        raise RuntimeError(f"unsupported skeleton asset type: {asset}")

    if skeleton is None:
        raise RuntimeError(f"asset has no skeleton: {asset.get_path_name()}")
    return skeleton


def summarize_branch(branch_json):
    return [entry["bone"] for entry in branch_json["branch"]]


def main() -> None:
    source_mesh = load_required_asset(SOURCE_MESH_PATH)
    target_mesh = load_required_asset(TARGET_MESH_PATH)
    retargeter = load_required_asset(RETARGETER_PATH)
    source_animation = load_optional_asset(SOURCE_ANIMATION_PATH)
    target_animation = load_optional_asset(TARGET_ANIMATION_PATH)

    source_skeleton = get_skeleton(source_mesh)
    target_skeleton = get_skeleton(target_mesh)

    branch_report = {
        "source": {},
        "target": {},
    }
    for root_bone in SOURCE_HAND_ROOTS:
        branch_report["source"][root_bone] = parse_json(
            unreal.SekiroRetargetDiagnosticsLibrary.dump_skeleton_branch_to_json(source_skeleton, root_bone)
        )
    for root_bone in TARGET_HAND_ROOTS:
        branch_report["target"][root_bone] = parse_json(
            unreal.SekiroRetargetDiagnosticsLibrary.dump_skeleton_branch_to_json(target_skeleton, root_bone)
        )

    dump_json("hand_branches.json", branch_report)

    source_pose = parse_json(
        unreal.SekiroRetargetDiagnosticsLibrary.dump_retarget_pose_to_json(
            retargeter,
            unreal.RetargetSourceOrTarget.SOURCE,
            [unreal.Name(bone_name) for bone_name in SOURCE_SAMPLE_BONES],
        )
    )
    target_pose = parse_json(
        unreal.SekiroRetargetDiagnosticsLibrary.dump_retarget_pose_to_json(
            retargeter,
            unreal.RetargetSourceOrTarget.TARGET,
            [unreal.Name(bone_name) for bone_name in TARGET_SAMPLE_BONES],
        )
    )
    dump_json("retarget_pose_source.json", source_pose)
    dump_json("retarget_pose_target.json", target_pose)

    summary = {
        "sourceAnimationPath": SOURCE_ANIMATION_PATH,
        "targetAnimationPath": TARGET_ANIMATION_PATH,
        "sourceAnimationLoaded": source_animation is not None,
        "targetAnimationLoaded": target_animation is not None,
    }

    if source_animation is not None and target_animation is not None:
        source_samples = parse_json(
            unreal.SekiroRetargetDiagnosticsLibrary.dump_animation_bone_samples_to_json(
                source_animation,
                [unreal.Name(bone_name) for bone_name in SOURCE_SAMPLE_BONES],
                FRAMES,
                True,
            )
        )
        target_samples = parse_json(
            unreal.SekiroRetargetDiagnosticsLibrary.dump_animation_bone_samples_to_json(
                target_animation,
                [unreal.Name(bone_name) for bone_name in TARGET_SAMPLE_BONES],
                FRAMES,
                True,
            )
        )
        dump_json("source_animation_samples.json", source_samples)
        dump_json("target_animation_samples.json", target_samples)
        summary["animationSamplingSkipped"] = False
    else:
        summary["animationSamplingSkipped"] = True
        unreal.log_warning("Skipping hand animation sample dump because source or target animation asset was missing.")

    dump_json("hand_diagnostic_summary.json", summary)

    unreal.log_warning("Source hand branches:")
    for root_bone, branch_json in branch_report["source"].items():
        unreal.log_warning(f"  {root_bone}: {summarize_branch(branch_json)}")

    unreal.log_warning("Target hand branches:")
    for root_bone, branch_json in branch_report["target"].items():
        unreal.log_warning(f"  {root_bone}: {summarize_branch(branch_json)}")


main()