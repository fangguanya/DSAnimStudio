import json
from pathlib import Path

import unreal


SOURCE_ANIMATION_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Animations/a000_201030"
TARGET_ANIMATION_PATH = "/Game/Retargeted/SekiroToManny/Animations/a000_201030_Manny"
OUTPUT_DIR = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\RetargetAnimationSelfTest")
FRAMES = [0, 5, 10, 15, 20, 25, 30]

SOURCE_SAMPLE_BONES = [
    "L_Shoulder",
    "L_UpperArm",
    "L_Forearm",
    "L_Hand",
    "L_Finger1",
    "L_Finger2",
    "L_Finger4",
    "R_Shoulder",
    "R_UpperArm",
    "R_Forearm",
    "R_Hand",
    "R_Finger1",
    "R_Finger2",
    "R_Finger4",
]

TARGET_SAMPLE_BONES = [
    "clavicle_l",
    "upperarm_l",
    "lowerarm_l",
    "hand_l",
    "index_metacarpal_l",
    "middle_metacarpal_l",
    "pinky_metacarpal_l",
    "clavicle_r",
    "upperarm_r",
    "lowerarm_r",
    "hand_r",
    "index_metacarpal_r",
    "middle_metacarpal_r",
    "pinky_metacarpal_r",
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


def dump_json(file_name: str, payload) -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    output_path = OUTPUT_DIR / file_name
    output_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")
    unreal.log_warning(f"wrote retarget animation self-test file: {output_path}")


source_animation = load_required_asset(SOURCE_ANIMATION_PATH)
target_animation = load_required_asset(TARGET_ANIMATION_PATH)

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
dump_json(
    "retarget_animation_selftest_summary.json",
    {
        "sourceAnimationPath": SOURCE_ANIMATION_PATH,
        "targetAnimationPath": TARGET_ANIMATION_PATH,
        "frames": FRAMES,
        "sourceBones": SOURCE_SAMPLE_BONES,
        "targetBones": TARGET_SAMPLE_BONES,
    },
)