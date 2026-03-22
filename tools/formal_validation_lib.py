import json
from pathlib import Path


IMPORT_REQUIRED_BONES = [
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

RETARGET_REFERENCE_THRESHOLDS = {
    "left_shoulder_hand": 0.95,
    "left_upper_lower": 0.95,
    "left_lower_hand": 0.75,
    "right_shoulder_hand": 0.95,
    "right_upper_lower": 0.95,
    "right_lower_hand": 0.75,
}

RETARGET_SOURCE_THRESHOLDS = {
    "left_shoulder_hand": 0.99,
    "left_upper_lower": 0.99,
    "left_lower_hand": 0.99,
    "right_shoulder_hand": 0.99,
    "right_upper_lower": 0.99,
    "right_lower_hand": 0.99,
}

RETARGET_ANIMATION_THRESHOLDS = {
    "left_shoulder_hand": 0.95,
    "left_upper_lower": 0.95,
    "left_lower_hand": 0.95,
    "right_shoulder_hand": 0.95,
    "right_upper_lower": 0.95,
    "right_lower_hand": 0.95,
}

RETARGET_POSE_HAND_BASIS_THRESHOLDS = {
    "left_hand_palm_forward": 0.85,
    "left_hand_palm_lateral": 0.85,
    "left_hand_palm_normal": 0.85,
    "right_hand_palm_forward": 0.85,
    "right_hand_palm_lateral": 0.85,
    "right_hand_palm_normal": 0.85,
}

RETARGET_ANIMATION_HAND_BASIS_THRESHOLDS = {
    "left_hand_palm_forward": 0.80,
    "left_hand_palm_lateral": 0.80,
    "left_hand_palm_normal": 0.80,
    "right_hand_palm_forward": 0.80,
    "right_hand_palm_lateral": 0.80,
    "right_hand_palm_normal": 0.80,
}

IMPORT_ANIMATION_DIRECTION_THRESHOLDS = {
    "left_shoulder_hand": 0.99,
    "left_upper_lower": 0.99,
    "left_lower_hand": 0.99,
    "right_shoulder_hand": 0.99,
    "right_upper_lower": 0.99,
    "right_lower_hand": 0.99,
}

IMPORT_ANIMATION_HAND_BASIS_THRESHOLDS = {
    "left_hand_palm_forward": 0.98,
    "left_hand_palm_lateral": 0.98,
    "left_hand_palm_normal": 0.98,
    "right_hand_palm_forward": 0.98,
    "right_hand_palm_lateral": 0.98,
    "right_hand_palm_normal": 0.98,
}

RETARGET_ANIMATION_CHAINS = {
    "left_shoulder_hand": ("L_Shoulder", "L_Hand", "clavicle_l", "hand_l"),
    "left_upper_lower": ("L_UpperArm", "L_Forearm", "upperarm_l", "lowerarm_l"),
    "left_lower_hand": ("L_Forearm", "L_Hand", "lowerarm_l", "hand_l"),
    "right_shoulder_hand": ("R_Shoulder", "R_Hand", "clavicle_r", "hand_r"),
    "right_upper_lower": ("R_UpperArm", "R_Forearm", "upperarm_r", "lowerarm_r"),
    "right_lower_hand": ("R_Forearm", "R_Hand", "lowerarm_r", "hand_r"),
}

RETARGET_HAND_BASIS_BONES = {
    "left_hand": ("L_Hand", "L_Finger1", "L_Finger2", "L_Finger4", "hand_l", "index_metacarpal_l", "middle_metacarpal_l", "pinky_metacarpal_l"),
    "right_hand": ("R_Hand", "R_Finger1", "R_Finger2", "R_Finger4", "hand_r", "index_metacarpal_r", "middle_metacarpal_r", "pinky_metacarpal_r"),
}


class ValidationError(RuntimeError):
    pass


def load_json(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))


def write_json(path, payload):
    output_path = Path(path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def _vector_length(vec):
    return (vec["x"] ** 2 + vec["y"] ** 2 + vec["z"] ** 2) ** 0.5


def _vector_sub(lhs, rhs):
    return {
        "x": lhs["x"] - rhs["x"],
        "y": lhs["y"] - rhs["y"],
        "z": lhs["z"] - rhs["z"],
    }


def _vector_dot(lhs, rhs):
    return lhs["x"] * rhs["x"] + lhs["y"] * rhs["y"] + lhs["z"] * rhs["z"]


def _vector_normalize(vec):
    length = _vector_length(vec)
    if length == 0:
        return {"x": 0.0, "y": 0.0, "z": 0.0}
    return {"x": vec["x"] / length, "y": vec["y"] / length, "z": vec["z"] / length}


def _bone_translation(bones, bone_name):
    bone_entry = bones[bone_name]
    if "componentTranslation" in bone_entry:
        return bone_entry["componentTranslation"]
    if "translation" in bone_entry:
        return bone_entry["translation"]
    raise KeyError(f"Missing translation data for bone '{bone_name}'.")


def _require(condition, message, issues):
    if not condition:
        issues.append(message)


def validate_import_selftest_report(report):
    issues = []
    assets = report.get("assets", {})
    bones = report.get("bones", {})

    _require(assets.get("meshLoaded") is True, "Self-test mesh asset was not loaded.", issues)
    _require(assets.get("skeletonLoaded") is True, "Self-test skeleton asset was not loaded.", issues)
    _require(assets.get("animationLoaded") is True, "Self-test animation asset was not loaded.", issues)
    _require(assets.get("meshUsesSkeleton") is True, "Imported mesh was not bound to the expected skeleton.", issues)
    _require(assets.get("animationUsesSkeleton") is True, "Imported animation was not bound to the expected skeleton.", issues)
    _require(bool(assets.get("physicsAssetPath")), "Imported mesh did not expose a physics asset.", issues)

    for bone_name in IMPORT_REQUIRED_BONES:
        _require(bone_name in bones, f"Required imported bone '{bone_name}' was missing from the reference pose dump.", issues)

    if issues:
        raise ValidationError("\n".join(issues))

    pelvis = _bone_translation(bones, "Pelvis")
    spine = _bone_translation(bones, "Spine")
    spine1 = _bone_translation(bones, "Spine1")
    spine2 = _bone_translation(bones, "Spine2")
    neck = _bone_translation(bones, "Neck")
    head = _bone_translation(bones, "Head")
    left_shoulder = _bone_translation(bones, "L_Shoulder")
    right_shoulder = _bone_translation(bones, "R_Shoulder")
    left_hand = _bone_translation(bones, "L_Hand")
    right_hand = _bone_translation(bones, "R_Hand")

    hand_lateral_delta = _vector_sub(left_hand, right_hand)
    head_minus_pelvis = _vector_sub(head, pelvis)

    _require(left_shoulder["x"] > right_shoulder["x"], "Imported shoulder ordering no longer matches left/right semantics.", issues)
    _require(left_hand["x"] > right_hand["x"], "Imported hand ordering no longer matches left/right semantics.", issues)
    _require(abs(hand_lateral_delta["x"]) >= 20.0, "Imported hands are too close together laterally.", issues)
    _require(
        abs(hand_lateral_delta["x"]) >= max(abs(hand_lateral_delta["y"]), abs(hand_lateral_delta["z"]), 1.0) * 4.0,
        "Imported hand separation is no longer X-dominant.",
        issues,
    )
    _require(head_minus_pelvis["z"] >= 20.0, "Imported head is not clearly above pelvis in UE Z.", issues)
    _require(spine["z"] > pelvis["z"], "Spine is not above pelvis.", issues)
    _require(spine1["z"] > spine["z"], "Spine1 is not above Spine.", issues)
    _require(spine2["z"] > spine1["z"], "Spine2 is not above Spine1.", issues)
    _require(neck["z"] > spine2["z"], "Neck is not above Spine2.", issues)
    _require(head["z"] > neck["z"], "Head is not above Neck.", issues)

    summary = {
        "success": not issues,
        "checks": {
            "meshUsesSkeleton": assets.get("meshUsesSkeleton") is True,
            "animationUsesSkeleton": assets.get("animationUsesSkeleton") is True,
            "physicsAssetPresent": bool(assets.get("physicsAssetPath")),
            "handLateralXAxisDominant": abs(hand_lateral_delta["x"]) >= max(abs(hand_lateral_delta["y"]), abs(hand_lateral_delta["z"]), 1.0) * 4.0,
            "torsoAscendingInZ": head["z"] > neck["z"] > spine2["z"] > spine1["z"] > spine["z"] > pelvis["z"],
        },
        "metrics": {
            "handLateralDelta": hand_lateral_delta,
            "headMinusPelvis": head_minus_pelvis,
        },
        "issues": issues,
    }

    if issues:
        raise ValidationError("\n".join(issues))

    return summary


def _validate_threshold_map(metrics, thresholds, label, issues):
    for metric_name, minimum in thresholds.items():
        value = metrics.get(metric_name)
        _require(value is not None, f"Missing {label} metric '{metric_name}'.", issues)
        if value is not None:
            _require(value >= minimum, f"{label} metric '{metric_name}' fell below threshold {minimum:.3f}: {value:.6f}", issues)


def _sample_frame_map(samples_report):
    frame_map = {}
    for frame_entry in samples_report.get("frames", []):
        sample_map = {}
        for sample in frame_entry.get("samples", []):
            if "error" in sample:
                continue
            sample_map[sample["bone"]] = sample["component"]
        frame_map[int(frame_entry["frame"])] = sample_map
    return frame_map


def _chain_direction(frame_map, start_bone, end_bone):
    if start_bone not in frame_map or end_bone not in frame_map:
        return None
    return _vector_normalize(_vector_sub(_bone_translation(frame_map, end_bone), _bone_translation(frame_map, start_bone)))


def _hand_basis_metrics(bones, hand_bone, index_bone, middle_bone, pinky_bone):
    for bone_name in (hand_bone, index_bone, middle_bone, pinky_bone):
        if bone_name not in bones:
            return None

    hand_translation = _bone_translation(bones, hand_bone)
    index_translation = _bone_translation(bones, index_bone)
    middle_translation = _bone_translation(bones, middle_bone)
    pinky_translation = _bone_translation(bones, pinky_bone)

    palm_forward = _vector_normalize(_vector_sub(middle_translation, hand_translation))
    palm_lateral = _vector_normalize(_vector_sub(index_translation, pinky_translation))
    palm_normal = _vector_normalize(
        {
            "x": palm_forward["y"] * palm_lateral["z"] - palm_forward["z"] * palm_lateral["y"],
            "y": palm_forward["z"] * palm_lateral["x"] - palm_forward["x"] * palm_lateral["z"],
            "z": palm_forward["x"] * palm_lateral["y"] - palm_forward["y"] * palm_lateral["x"],
        }
    )
    palm_lateral = _vector_normalize(
        {
            "x": palm_normal["y"] * palm_forward["z"] - palm_normal["z"] * palm_forward["y"],
            "y": palm_normal["z"] * palm_forward["x"] - palm_normal["x"] * palm_forward["z"],
            "z": palm_normal["x"] * palm_forward["y"] - palm_normal["y"] * palm_forward["x"],
        }
    )

    return {
        "palm_forward": palm_forward,
        "palm_lateral": palm_lateral,
        "palm_normal": palm_normal,
    }


def _compute_hand_basis_dot_metrics(source_bones, target_bones):
    metrics = {}
    for metric_prefix, bone_names in RETARGET_HAND_BASIS_BONES.items():
        (
            source_hand_bone,
            source_index_bone,
            source_middle_bone,
            source_pinky_bone,
            target_hand_bone,
            target_index_bone,
            target_middle_bone,
            target_pinky_bone,
        ) = bone_names

        source_basis = _hand_basis_metrics(source_bones, source_hand_bone, source_index_bone, source_middle_bone, source_pinky_bone)
        target_basis = _hand_basis_metrics(target_bones, target_hand_bone, target_index_bone, target_middle_bone, target_pinky_bone)
        if source_basis is None or target_basis is None:
            continue

        metrics[f"{metric_prefix}_palm_forward"] = _vector_dot(source_basis["palm_forward"], target_basis["palm_forward"])
        metrics[f"{metric_prefix}_palm_lateral"] = _vector_dot(source_basis["palm_lateral"], target_basis["palm_lateral"])
        metrics[f"{metric_prefix}_palm_normal"] = _vector_dot(source_basis["palm_normal"], target_basis["palm_normal"])

    return metrics


def validate_retarget_animation_samples(source_samples_report, target_samples_report):
    issues = []
    source_frames = _sample_frame_map(source_samples_report)
    target_frames = _sample_frame_map(target_samples_report)
    common_frames = sorted(set(source_frames.keys()) & set(target_frames.keys()))
    _require(bool(common_frames), "No overlapping animation sample frames were available for retarget validation.", issues)

    minimum_dots = {}
    for metric_name, (source_start, source_end, target_start, target_end) in RETARGET_ANIMATION_CHAINS.items():
        frame_dots = []
        for frame_index in common_frames:
            source_direction = _chain_direction(source_frames[frame_index], source_start, source_end)
            target_direction = _chain_direction(target_frames[frame_index], target_start, target_end)
            if source_direction is None or target_direction is None:
                continue
            frame_dots.append(_vector_dot(source_direction, target_direction))

        _require(bool(frame_dots), f"No valid animation samples were available for metric '{metric_name}'.", issues)
        if frame_dots:
            minimum_dots[metric_name] = min(frame_dots)

    minimum_hand_basis_dots = {}
    for metric_name in RETARGET_ANIMATION_HAND_BASIS_THRESHOLDS:
        minimum_hand_basis_dots[metric_name] = None

    for frame_index in common_frames:
        frame_metrics = _compute_hand_basis_dot_metrics(source_frames[frame_index], target_frames[frame_index])
        for metric_name, value in frame_metrics.items():
            current_minimum = minimum_hand_basis_dots.get(metric_name)
            if current_minimum is None or value < current_minimum:
                minimum_hand_basis_dots[metric_name] = value

    for metric_name in RETARGET_ANIMATION_HAND_BASIS_THRESHOLDS:
        _require(minimum_hand_basis_dots.get(metric_name) is not None, f"No valid animation samples were available for hand basis metric '{metric_name}'.", issues)

    _validate_threshold_map(minimum_dots, RETARGET_ANIMATION_THRESHOLDS, "retarget animation", issues)
    _validate_threshold_map(minimum_hand_basis_dots, RETARGET_ANIMATION_HAND_BASIS_THRESHOLDS, "retarget animation hand basis", issues)

    summary = {
        "success": not issues,
        "commonFrames": common_frames,
        "minimumDirectionDots": minimum_dots,
        "minimumHandBasisDots": minimum_hand_basis_dots,
        "issues": issues,
    }

    if issues:
        raise ValidationError("\n".join(issues))

    return summary


def validate_retarget_pose_report(report, source_samples_report=None, target_samples_report=None):
    issues = []
    bones = report.get("bones", {})
    source_bones = report.get("sourceBones", {})
    reference_metrics = report.get("segmentDotsAgainstReference", {})
    source_metrics = report.get("segmentDotsAgainstSource", {})

    required_target_bones = [
        "clavicle_l", "upperarm_l", "lowerarm_l", "hand_l", "index_metacarpal_l", "middle_metacarpal_l", "pinky_metacarpal_l",
        "clavicle_r", "upperarm_r", "lowerarm_r", "hand_r", "index_metacarpal_r", "middle_metacarpal_r", "pinky_metacarpal_r",
    ]
    required_source_bones = [
        "L_Shoulder", "L_UpperArm", "L_Forearm", "L_Hand", "L_Finger1", "L_Finger2", "L_Finger4",
        "R_Shoulder", "R_UpperArm", "R_Forearm", "R_Hand", "R_Finger1", "R_Finger2", "R_Finger4",
    ]
    for bone_name in required_target_bones:
        _require(bone_name in bones, f"Missing target pose bone '{bone_name}'.", issues)
    for bone_name in required_source_bones:
        _require(bone_name in source_bones, f"Missing source pose bone '{bone_name}'.", issues)

    if issues:
        raise ValidationError("\n".join(issues))

    _validate_threshold_map(reference_metrics, RETARGET_REFERENCE_THRESHOLDS, "retarget reference", issues)
    _validate_threshold_map(source_metrics, RETARGET_SOURCE_THRESHOLDS, "retarget source", issues)

    _require(
        _bone_translation(bones, "clavicle_l")["x"] > _bone_translation(bones, "clavicle_r")["x"],
        "Target clavicle ordering no longer matches left/right semantics.",
        issues,
    )
    _require(
        _bone_translation(bones, "hand_l")["x"] > _bone_translation(bones, "hand_r")["x"],
        "Target hand ordering no longer matches left/right semantics.",
        issues,
    )
    _require(
        _bone_translation(source_bones, "L_Shoulder")["x"] > _bone_translation(source_bones, "R_Shoulder")["x"],
        "Source shoulder ordering no longer matches left/right semantics.",
        issues,
    )
    _require(
        _bone_translation(source_bones, "L_Hand")["x"] > _bone_translation(source_bones, "R_Hand")["x"],
        "Source hand ordering no longer matches left/right semantics.",
        issues,
    )

    for start_bone, end_bone in (("clavicle_l", "hand_l"), ("upperarm_l", "lowerarm_l"), ("lowerarm_l", "hand_l"), ("clavicle_r", "hand_r"), ("upperarm_r", "lowerarm_r"), ("lowerarm_r", "hand_r")):
        segment_length = _vector_length(_vector_sub(_bone_translation(bones, end_bone), _bone_translation(bones, start_bone)))
        _require(segment_length >= 5.0, f"Target segment {start_bone}->{end_bone} collapsed below 5 units.", issues)

    hand_basis_metrics = _compute_hand_basis_dot_metrics(source_bones, bones)
    _validate_threshold_map(hand_basis_metrics, RETARGET_POSE_HAND_BASIS_THRESHOLDS, "retarget pose hand basis", issues)

    animation_summary = None
    if source_samples_report is not None and target_samples_report is not None:
        animation_summary = validate_retarget_animation_samples(source_samples_report, target_samples_report)

    summary = {
        "success": not issues,
        "checks": {
            "referenceThresholds": not any(metric.startswith("retarget reference") for metric in issues),
            "sourceThresholds": not any(metric.startswith("retarget source") for metric in issues),
        },
        "metrics": {
            "segmentDotsAgainstReference": reference_metrics,
            "segmentDotsAgainstSource": source_metrics,
            "handBasisDots": hand_basis_metrics,
        },
        "animation": animation_summary,
        "issues": issues,
    }

    if issues:
        raise ValidationError("\n".join(issues))

    return summary


def validate_ue_import_report(report):
    issues = []

    _require(report.get("success") is True, "UE import report did not indicate success.", issues)
    _require(report.get("visiblePoseValidated") is True, "UE import report did not validate the visible pose.", issues)
    _require(report.get("skeletonImported") is True, "UE import report did not import a skeleton.", issues)
    _require(report.get("hasPhysicsAsset") is True, "UE import report did not produce a physics asset.", issues)

    relationships = report.get("relationships", {})
    _require(relationships.get("skeletalMeshBound") is True, "UE import report did not bind a skeletal mesh.", issues)
    _require(relationships.get("skeletonBound") is True, "UE import report did not bind a skeleton.", issues)
    _require(relationships.get("animationsCoverageComplete") is True, "UE import report did not import the complete expected animation coverage.", issues)

    post_import_checks = report.get("postImportChecks", {})
    _require(post_import_checks.get("animationBranchBasisValidated") is True, "UE import report did not execute animation branch-basis validation.", issues)
    _require(post_import_checks.get("rtgDiagnosticsRequiredForSuccess") is False, "UE import report still treats RTG diagnostics as required for success.", issues)
    _require(post_import_checks.get("rtgDiagnosticsRole") == "post-import-only", "UE import report did not mark RTG diagnostics as post-import-only.", issues)

    animation_validation = report.get("animationValidation", {})
    clips = animation_validation.get("clips", [])
    _require(bool(clips), "UE import report did not record per-clip animation validation results.", issues)

    validated_clip_count = 0
    for clip in clips:
        clip_name = clip.get("clip", "<unknown>")
        clip_issues = clip.get("issues", [])
        _require(clip.get("success") is True, f"UE import clip validation failed for '{clip_name}'.", issues)
        _require(not clip_issues, f"UE import clip validation reported issues for '{clip_name}': {'; '.join(clip_issues)}", issues)
        _validate_threshold_map(
            clip.get("minimumDirectionDots", {}),
            IMPORT_ANIMATION_DIRECTION_THRESHOLDS,
            f"UE import animation '{clip_name}' direction",
            issues,
        )
        _validate_threshold_map(
            clip.get("minimumHandBasisDots", {}),
            IMPORT_ANIMATION_HAND_BASIS_THRESHOLDS,
            f"UE import animation '{clip_name}' hand basis",
            issues,
        )
        validated_clip_count += 1

    summary = {
        "success": not issues,
        "validatedClipCount": validated_clip_count,
        "checks": {
            "visiblePoseValidated": report.get("visiblePoseValidated") is True,
            "animationBranchBasisValidated": post_import_checks.get("animationBranchBasisValidated") is True,
            "rtgDiagnosticsPostImportOnly": post_import_checks.get("rtgDiagnosticsRequiredForSuccess") is False
                and post_import_checks.get("rtgDiagnosticsRole") == "post-import-only",
            "animationCoverageComplete": relationships.get("animationsCoverageComplete") is True,
        },
        "issues": issues,
    }

    if issues:
        raise ValidationError("\n".join(issues))

    return summary