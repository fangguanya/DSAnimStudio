from __future__ import annotations

import json
import math
from pathlib import Path
import sys


DEFAULT_OUTPUT_DIR = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\RetargetAnimationSelfTest")
DEFAULT_SOURCE_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "source_animation_samples.json"
DEFAULT_TARGET_SAMPLES_PATH = DEFAULT_OUTPUT_DIR / "target_animation_samples.json"
DEFAULT_OUTPUT_PATH = DEFAULT_OUTPUT_DIR / "retarget_wrist_diagnostic.json"


SIDE_SPECS = {
    "left": {
        "source": {
            "forearm": "L_Forearm",
            "hand": "L_Hand",
            "index": "L_Finger1",
            "middle": "L_Finger2",
            "pinky": "L_Finger4",
        },
        "target": {
            "forearm": "lowerarm_l",
            "hand": "hand_l",
            "index": "index_metacarpal_l",
            "middle": "middle_metacarpal_l",
            "pinky": "pinky_metacarpal_l",
        },
    },
    "right": {
        "source": {
            "forearm": "R_Forearm",
            "hand": "R_Hand",
            "index": "R_Finger1",
            "middle": "R_Finger2",
            "pinky": "R_Finger4",
        },
        "target": {
            "forearm": "lowerarm_r",
            "hand": "hand_r",
            "index": "index_metacarpal_r",
            "middle": "middle_metacarpal_r",
            "pinky": "pinky_metacarpal_r",
        },
    },
}


def load_json(path: Path):
    return json.loads(path.read_text(encoding="utf-8"))


def write_json(path: Path, payload) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def vec_sub(lhs, rhs):
    return {"x": lhs["x"] - rhs["x"], "y": lhs["y"] - rhs["y"], "z": lhs["z"] - rhs["z"]}


def vec_dot(lhs, rhs):
    return lhs["x"] * rhs["x"] + lhs["y"] * rhs["y"] + lhs["z"] * rhs["z"]


def vec_length(vec):
    return math.sqrt(vec_dot(vec, vec))


def vec_normalize(vec):
    length = vec_length(vec)
    if length == 0:
        return {"x": 0.0, "y": 0.0, "z": 0.0}
    return {"x": vec["x"] / length, "y": vec["y"] / length, "z": vec["z"] / length}


def vec_cross(lhs, rhs):
    return {
        "x": lhs["y"] * rhs["z"] - lhs["z"] * rhs["y"],
        "y": lhs["z"] * rhs["x"] - lhs["x"] * rhs["z"],
        "z": lhs["x"] * rhs["y"] - lhs["y"] * rhs["x"],
    }


def vec_scale(vec, scalar):
    return {"x": vec["x"] * scalar, "y": vec["y"] * scalar, "z": vec["z"] * scalar}


def vec_project_onto_plane(vec, plane_normal):
    normal = vec_normalize(plane_normal)
    return vec_sub(vec, vec_scale(normal, vec_dot(vec, normal)))


def signed_angle_on_axis(current_vec, desired_vec, axis):
    projected_current = vec_normalize(vec_project_onto_plane(current_vec, axis))
    projected_desired = vec_normalize(vec_project_onto_plane(desired_vec, axis))
    if vec_length(projected_current) == 0 or vec_length(projected_desired) == 0:
        return None
    clamped_dot = max(-1.0, min(1.0, vec_dot(projected_current, projected_desired)))
    signed_sin = vec_dot(axis, vec_cross(projected_current, projected_desired))
    return math.degrees(math.atan2(signed_sin, clamped_dot))


def sample_map(frame_entry):
    samples = {}
    for sample in frame_entry.get("samples", []):
        if "error" in sample:
            continue
        samples[sample["bone"]] = sample["component"]
    return samples


def bone_translation(samples, bone_name):
    return samples[bone_name]["translation"]


def hand_basis(samples, hand_bone, index_bone, middle_bone, pinky_bone):
    hand = bone_translation(samples, hand_bone)
    index = bone_translation(samples, index_bone)
    middle = bone_translation(samples, middle_bone)
    pinky = bone_translation(samples, pinky_bone)
    palm_forward = vec_normalize(vec_sub(middle, hand))
    palm_lateral_seed = vec_normalize(vec_sub(index, pinky))
    palm_normal = vec_normalize(vec_cross(palm_forward, palm_lateral_seed))
    palm_lateral = vec_normalize(vec_cross(palm_normal, palm_forward))
    return {
        "palm_forward": palm_forward,
        "palm_lateral": palm_lateral,
        "palm_normal": palm_normal,
    }


def chain_direction(samples, start_bone, end_bone):
    return vec_normalize(vec_sub(bone_translation(samples, end_bone), bone_translation(samples, start_bone)))


def mean(values):
    return sum(values) / len(values) if values else None


def stddev(values):
    if len(values) < 2:
        return 0.0 if values else None
    avg = mean(values)
    return math.sqrt(sum((value - avg) ** 2 for value in values) / len(values))


def side_frame_metrics(source_samples, target_samples, side_name, side_spec):
    source_basis = hand_basis(source_samples, **{k + "_bone": v for k, v in {
        "hand": side_spec["source"]["hand"],
        "index": side_spec["source"]["index"],
        "middle": side_spec["source"]["middle"],
        "pinky": side_spec["source"]["pinky"],
    }.items()})
    target_basis = hand_basis(target_samples, **{k + "_bone": v for k, v in {
        "hand": side_spec["target"]["hand"],
        "index": side_spec["target"]["index"],
        "middle": side_spec["target"]["middle"],
        "pinky": side_spec["target"]["pinky"],
    }.items()})

    source_axis = chain_direction(source_samples, side_spec["source"]["forearm"], side_spec["source"]["hand"])
    target_axis = chain_direction(target_samples, side_spec["target"]["forearm"], side_spec["target"]["hand"])
    diagnosis_axis = vec_normalize({
        "x": source_axis["x"] + target_axis["x"],
        "y": source_axis["y"] + target_axis["y"],
        "z": source_axis["z"] + target_axis["z"],
    })
    if vec_length(diagnosis_axis) == 0:
        diagnosis_axis = source_axis

    forward_twist = signed_angle_on_axis(target_basis["palm_forward"], source_basis["palm_forward"], diagnosis_axis)
    lateral_twist = signed_angle_on_axis(target_basis["palm_lateral"], source_basis["palm_lateral"], diagnosis_axis)
    normal_twist = signed_angle_on_axis(target_basis["palm_normal"], source_basis["palm_normal"], diagnosis_axis)

    twist_candidates = [value for value in (forward_twist, lateral_twist, normal_twist) if value is not None]
    twist_mean = mean(twist_candidates)

    return {
        "side": side_name,
        "axisDot": vec_dot(source_axis, target_axis),
        "handBasisDots": {
            "palm_forward": vec_dot(source_basis["palm_forward"], target_basis["palm_forward"]),
            "palm_lateral": vec_dot(source_basis["palm_lateral"], target_basis["palm_lateral"]),
            "palm_normal": vec_dot(source_basis["palm_normal"], target_basis["palm_normal"]),
        },
        "diagnosisAxis": diagnosis_axis,
        "signedTwistDegrees": {
            "fromPalmForward": forward_twist,
            "fromPalmLateral": lateral_twist,
            "fromPalmNormal": normal_twist,
            "mean": twist_mean,
            "spread": None if len(twist_candidates) < 2 else max(twist_candidates) - min(twist_candidates),
        },
    }


def main() -> int:
    source_path = Path(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_SOURCE_SAMPLES_PATH
    target_path = Path(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_TARGET_SAMPLES_PATH
    output_path = Path(sys.argv[3]) if len(sys.argv) > 3 else DEFAULT_OUTPUT_PATH

    source_report = load_json(source_path)
    target_report = load_json(target_path)

    source_frames = {int(frame["frame"]): sample_map(frame) for frame in source_report.get("frames", [])}
    target_frames = {int(frame["frame"]): sample_map(frame) for frame in target_report.get("frames", [])}
    common_frames = sorted(set(source_frames) & set(target_frames))
    if not common_frames:
        raise RuntimeError("No overlapping frames between source and target retarget sample reports.")

    frame_reports = []
    side_summaries = {}
    for side_name, side_spec in SIDE_SPECS.items():
        side_frame_reports = []
        for frame in common_frames:
            side_frame_reports.append({
                "frame": frame,
                **side_frame_metrics(source_frames[frame], target_frames[frame], side_name, side_spec),
            })

        twist_means = [entry["signedTwistDegrees"]["mean"] for entry in side_frame_reports if entry["signedTwistDegrees"]["mean"] is not None]
        forward_dots = [entry["handBasisDots"]["palm_forward"] for entry in side_frame_reports]
        lateral_dots = [entry["handBasisDots"]["palm_lateral"] for entry in side_frame_reports]
        normal_dots = [entry["handBasisDots"]["palm_normal"] for entry in side_frame_reports]
        axis_dots = [entry["axisDot"] for entry in side_frame_reports]

        side_summaries[side_name] = {
            "axisDot": {
                "min": min(axis_dots),
                "max": max(axis_dots),
                "mean": mean(axis_dots),
            },
            "handBasisDots": {
                "palm_forward": {"min": min(forward_dots), "max": max(forward_dots), "mean": mean(forward_dots)},
                "palm_lateral": {"min": min(lateral_dots), "max": max(lateral_dots), "mean": mean(lateral_dots)},
                "palm_normal": {"min": min(normal_dots), "max": max(normal_dots), "mean": mean(normal_dots)},
            },
            "meanTwistDegrees": mean(twist_means),
            "twistStdDevDegrees": stddev(twist_means),
            "twistRangeDegrees": None if not twist_means else max(twist_means) - min(twist_means),
            "suggestsConstantWristOffset": bool(twist_means) and (stddev(twist_means) or 0.0) < 5.0,
        }

        frame_reports.extend(side_frame_reports)

    payload = {
        "sourceAnimation": source_report.get("animation"),
        "targetAnimation": target_report.get("animation"),
        "frames": common_frames,
        "summary": side_summaries,
        "frameReports": frame_reports,
    }
    write_json(output_path, payload)

    print(f"wrote retarget wrist diagnostic: {output_path}")
    for side_name, summary in side_summaries.items():
        mean_twist = summary["meanTwistDegrees"]
        twist_stddev = summary["twistStdDevDegrees"]
        print(
            f"{side_name}: twist_mean={'nan' if mean_twist is None else f'{mean_twist:.3f}'} "
            f"twist_stddev={'nan' if twist_stddev is None else f'{twist_stddev:.3f}'} "
            f"constant_offset={summary['suggestsConstantWristOffset']}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())