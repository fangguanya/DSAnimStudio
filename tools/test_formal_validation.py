import unittest

try:
    from tools.formal_validation_lib import (
        ValidationError,
        validate_import_selftest_report,
        validate_retarget_animation_samples,
        validate_retarget_pose_report,
        validate_ue_import_report,
    )
except ModuleNotFoundError:
    from formal_validation_lib import (
        ValidationError,
        validate_import_selftest_report,
        validate_retarget_animation_samples,
        validate_retarget_pose_report,
        validate_ue_import_report,
    )


def _bone(x, y, z):
    return {
        "componentTranslation": {"x": x, "y": y, "z": z},
        "componentRotation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
    }


def _midpoint(lhs, rhs):
    return tuple((lhs[index] + rhs[index]) / 2.0 for index in range(3))


def _translation_tuple(entry):
    if isinstance(entry, tuple):
        return entry
    translation = entry["componentTranslation"]
    return (translation["x"], translation["y"], translation["z"])


def _make_matching_entry(template_entry, xyz):
    if isinstance(template_entry, tuple):
        return xyz
    return _bone(*xyz)


def _augment_hand_basis_entries(entries):
    augmented = dict(entries)

    if "L_Finger1" in augmented and "L_Finger4" in augmented and "L_Finger2" not in augmented:
        midpoint = _midpoint(_translation_tuple(augmented["L_Finger1"]), _translation_tuple(augmented["L_Finger4"]))
        augmented["L_Finger2"] = _make_matching_entry(augmented["L_Finger1"], midpoint)
    if "R_Finger1" in augmented and "R_Finger4" in augmented and "R_Finger2" not in augmented:
        midpoint = _midpoint(_translation_tuple(augmented["R_Finger1"]), _translation_tuple(augmented["R_Finger4"]))
        augmented["R_Finger2"] = _make_matching_entry(augmented["R_Finger1"], midpoint)
    if "index_metacarpal_l" in augmented and "pinky_metacarpal_l" in augmented and "middle_metacarpal_l" not in augmented:
        midpoint = _midpoint(_translation_tuple(augmented["index_metacarpal_l"]), _translation_tuple(augmented["pinky_metacarpal_l"]))
        augmented["middle_metacarpal_l"] = _make_matching_entry(augmented["index_metacarpal_l"], midpoint)
    if "index_metacarpal_r" in augmented and "pinky_metacarpal_r" in augmented and "middle_metacarpal_r" not in augmented:
        midpoint = _midpoint(_translation_tuple(augmented["index_metacarpal_r"]), _translation_tuple(augmented["pinky_metacarpal_r"]))
        augmented["middle_metacarpal_r"] = _make_matching_entry(augmented["index_metacarpal_r"], midpoint)

    return augmented


def _frame(frame_index, samples):
    augmented_samples = _augment_hand_basis_entries(samples)
    return {
        "frame": frame_index,
        "samples": [
            {
                "bone": bone_name,
                "component": {
                    "translation": {"x": x, "y": y, "z": z},
                    "rotation": {"x": 0.0, "y": 0.0, "z": 0.0, "w": 1.0},
                    "scale": {"x": 1.0, "y": 1.0, "z": 1.0},
                },
            }
            for bone_name, (x, y, z) in augmented_samples.items()
        ],
    }


class FormalValidationTests(unittest.TestCase):
    def test_import_selftest_validation_accepts_valid_pose(self):
        report = {
            "assets": {
                "meshLoaded": True,
                "skeletonLoaded": True,
                "animationLoaded": True,
                "meshUsesSkeleton": True,
                "animationUsesSkeleton": True,
                "physicsAssetPath": "/Game/Test/PA",
            },
            "bones": {
                "Master": _bone(0, 0, 0),
                "Pelvis": _bone(0, 0, 90),
                "Spine": _bone(0, 0, 110),
                "Spine1": _bone(0, 0, 125),
                "Spine2": _bone(0, 0, 140),
                "Neck": _bone(0, 0, 155),
                "Head": _bone(0, 0, 170),
                "L_Shoulder": _bone(15, 0, 145),
                "R_Shoulder": _bone(-15, 0, 145),
                "L_Hand": _bone(55, 1, 110),
                "R_Hand": _bone(-55, -1, 110),
            },
        }

        summary = validate_import_selftest_report(report)

        self.assertTrue(summary["success"])
        self.assertTrue(summary["checks"]["handLateralXAxisDominant"])

    def test_import_selftest_validation_rejects_collapsed_hands(self):
        report = {
            "assets": {
                "meshLoaded": True,
                "skeletonLoaded": True,
                "animationLoaded": True,
                "meshUsesSkeleton": True,
                "animationUsesSkeleton": True,
                "physicsAssetPath": "/Game/Test/PA",
            },
            "bones": {
                "Master": _bone(0, 0, 0),
                "Pelvis": _bone(0, 0, 90),
                "Spine": _bone(0, 0, 110),
                "Spine1": _bone(0, 0, 125),
                "Spine2": _bone(0, 0, 140),
                "Neck": _bone(0, 0, 155),
                "Head": _bone(0, 0, 170),
                "L_Shoulder": _bone(3, 0, 145),
                "R_Shoulder": _bone(-3, 0, 145),
                "L_Hand": _bone(4, 8, 110),
                "R_Hand": _bone(-4, -8, 110),
            },
        }

        with self.assertRaises(ValidationError):
            validate_import_selftest_report(report)

    def test_import_selftest_validation_requires_physics_asset(self):
        report = {
            "assets": {
                "meshLoaded": True,
                "skeletonLoaded": True,
                "animationLoaded": True,
                "meshUsesSkeleton": True,
                "animationUsesSkeleton": True,
                "physicsAssetPath": "",
            },
            "bones": {
                "Master": _bone(0, 0, 0),
                "Pelvis": _bone(0, 0, 90),
                "Spine": _bone(0, 0, 110),
                "Spine1": _bone(0, 0, 125),
                "Spine2": _bone(0, 0, 140),
                "Neck": _bone(0, 0, 155),
                "Head": _bone(0, 0, 170),
                "L_Shoulder": _bone(15, 0, 145),
                "R_Shoulder": _bone(-15, 0, 145),
                "L_Hand": _bone(55, 1, 110),
                "R_Hand": _bone(-55, -1, 110),
            },
        }

        with self.assertRaises(ValidationError):
            validate_import_selftest_report(report)

    def test_retarget_pose_validation_accepts_aligned_chains(self):
        report = {
            "bones": _augment_hand_basis_entries({
                "clavicle_l": _bone(2, 0, 150),
                "upperarm_l": _bone(14, 0, 138),
                "lowerarm_l": _bone(34, 0, 118),
                "hand_l": _bone(53, 0, 98),
                "index_metacarpal_l": _bone(58, 5, 95),
                "pinky_metacarpal_l": _bone(57, -5, 95),
                "clavicle_r": _bone(-2, 0, 150),
                "upperarm_r": _bone(-14, 0, 138),
                "lowerarm_r": _bone(-34, 0, 118),
                "hand_r": _bone(-53, 0, 98),
                "index_metacarpal_r": _bone(-58, -5, 95),
                "pinky_metacarpal_r": _bone(-57, 5, 95),
            }),
            "sourceBones": _augment_hand_basis_entries({
                "L_Shoulder": _bone(17, 0, 140),
                "L_UpperArm": _bone(17, 0, 139),
                "L_Forearm": _bone(37, 0, 119),
                "L_Hand": _bone(53, 0, 103),
                "L_Finger1": _bone(58, 5, 100),
                "L_Finger4": _bone(57, -5, 100),
                "R_Shoulder": _bone(-17, 0, 140),
                "R_UpperArm": _bone(-17, 0, 139),
                "R_Forearm": _bone(-37, 0, 119),
                "R_Hand": _bone(-53, 0, 103),
                "R_Finger1": _bone(-58, -5, 100),
                "R_Finger4": _bone(-57, 5, 100),
            }),
            "segmentDotsAgainstReference": {
                "left_shoulder_hand": 0.98,
                "left_upper_lower": 0.99,
                "left_lower_hand": 0.85,
                "right_shoulder_hand": 0.98,
                "right_upper_lower": 0.99,
                "right_lower_hand": 0.85,
            },
            "segmentDotsAgainstSource": {
                "left_shoulder_hand": 0.999,
                "left_upper_lower": 0.997,
                "left_lower_hand": 0.999,
                "right_shoulder_hand": 0.999,
                "right_upper_lower": 0.997,
                "right_lower_hand": 0.999,
            },
        }

        summary = validate_retarget_pose_report(report)

        self.assertTrue(summary["success"])

    def test_retarget_pose_validation_rejects_bad_segment_dot(self):
        report = {
            "bones": _augment_hand_basis_entries({
                "clavicle_l": _bone(2, 0, 150),
                "upperarm_l": _bone(14, 0, 138),
                "lowerarm_l": _bone(34, 0, 118),
                "hand_l": _bone(53, 0, 98),
                "index_metacarpal_l": _bone(58, 5, 95),
                "pinky_metacarpal_l": _bone(57, -5, 95),
                "clavicle_r": _bone(-2, 0, 150),
                "upperarm_r": _bone(-14, 0, 138),
                "lowerarm_r": _bone(-34, 0, 118),
                "hand_r": _bone(-53, 0, 98),
                "index_metacarpal_r": _bone(-58, -5, 95),
                "pinky_metacarpal_r": _bone(-57, 5, 95),
            }),
            "sourceBones": _augment_hand_basis_entries({
                "L_Shoulder": _bone(17, 0, 140),
                "L_UpperArm": _bone(17, 0, 139),
                "L_Forearm": _bone(37, 0, 119),
                "L_Hand": _bone(53, 0, 103),
                "L_Finger1": _bone(58, 5, 100),
                "L_Finger4": _bone(57, -5, 100),
                "R_Shoulder": _bone(-17, 0, 140),
                "R_UpperArm": _bone(-17, 0, 139),
                "R_Forearm": _bone(-37, 0, 119),
                "R_Hand": _bone(-53, 0, 103),
                "R_Finger1": _bone(-58, -5, 100),
                "R_Finger4": _bone(-57, 5, 100),
            }),
            "segmentDotsAgainstReference": {
                "left_shoulder_hand": 0.2,
                "left_upper_lower": 0.99,
                "left_lower_hand": 0.85,
                "right_shoulder_hand": 0.98,
                "right_upper_lower": 0.99,
                "right_lower_hand": 0.85,
            },
            "segmentDotsAgainstSource": {
                "left_shoulder_hand": 0.999,
                "left_upper_lower": 0.997,
                "left_lower_hand": 0.999,
                "right_shoulder_hand": 0.999,
                "right_upper_lower": 0.997,
                "right_lower_hand": 0.999,
            },
        }

        with self.assertRaises(ValidationError):
            validate_retarget_pose_report(report)

    def test_retarget_pose_validation_rejects_collapsed_target_segment(self):
        report = {
            "bones": _augment_hand_basis_entries({
                "clavicle_l": _bone(2, 0, 150),
                "upperarm_l": _bone(14, 0, 138),
                "lowerarm_l": _bone(14.5, 0, 138.5),
                "hand_l": _bone(53, 0, 98),
                "index_metacarpal_l": _bone(58, 5, 95),
                "pinky_metacarpal_l": _bone(57, -5, 95),
                "clavicle_r": _bone(-2, 0, 150),
                "upperarm_r": _bone(-14, 0, 138),
                "lowerarm_r": _bone(-34, 0, 118),
                "hand_r": _bone(-53, 0, 98),
                "index_metacarpal_r": _bone(-58, -5, 95),
                "pinky_metacarpal_r": _bone(-57, 5, 95),
            }),
            "sourceBones": _augment_hand_basis_entries({
                "L_Shoulder": _bone(17, 0, 140),
                "L_UpperArm": _bone(17, 0, 139),
                "L_Forearm": _bone(37, 0, 119),
                "L_Hand": _bone(53, 0, 103),
                "L_Finger1": _bone(58, 5, 100),
                "L_Finger4": _bone(57, -5, 100),
                "R_Shoulder": _bone(-17, 0, 140),
                "R_UpperArm": _bone(-17, 0, 139),
                "R_Forearm": _bone(-37, 0, 119),
                "R_Hand": _bone(-53, 0, 103),
                "R_Finger1": _bone(-58, -5, 100),
                "R_Finger4": _bone(-57, 5, 100),
            }),
            "segmentDotsAgainstReference": {
                "left_shoulder_hand": 0.98,
                "left_upper_lower": 0.99,
                "left_lower_hand": 0.85,
                "right_shoulder_hand": 0.98,
                "right_upper_lower": 0.99,
                "right_lower_hand": 0.85,
            },
            "segmentDotsAgainstSource": {
                "left_shoulder_hand": 0.999,
                "left_upper_lower": 0.997,
                "left_lower_hand": 0.999,
                "right_shoulder_hand": 0.999,
                "right_upper_lower": 0.997,
                "right_lower_hand": 0.999,
            },
        }

        with self.assertRaises(ValidationError):
            validate_retarget_pose_report(report)

    def test_retarget_pose_validation_rejects_wrist_basis_flip(self):
        report = {
            "bones": _augment_hand_basis_entries({
                "clavicle_l": _bone(2, 0, 150),
                "upperarm_l": _bone(14, 0, 138),
                "lowerarm_l": _bone(34, 0, 118),
                "hand_l": _bone(53, 0, 98),
                "index_metacarpal_l": _bone(57, -5, 95),
                "pinky_metacarpal_l": _bone(58, 5, 95),
                "clavicle_r": _bone(-2, 0, 150),
                "upperarm_r": _bone(-14, 0, 138),
                "lowerarm_r": _bone(-34, 0, 118),
                "hand_r": _bone(-53, 0, 98),
                "index_metacarpal_r": _bone(-57, 5, 95),
                "pinky_metacarpal_r": _bone(-58, -5, 95),
            }),
            "sourceBones": _augment_hand_basis_entries({
                "L_Shoulder": _bone(17, 0, 140),
                "L_UpperArm": _bone(17, 0, 139),
                "L_Forearm": _bone(37, 0, 119),
                "L_Hand": _bone(53, 0, 103),
                "L_Finger1": _bone(58, 5, 100),
                "L_Finger4": _bone(57, -5, 100),
                "R_Shoulder": _bone(-17, 0, 140),
                "R_UpperArm": _bone(-17, 0, 139),
                "R_Forearm": _bone(-37, 0, 119),
                "R_Hand": _bone(-53, 0, 103),
                "R_Finger1": _bone(-58, -5, 100),
                "R_Finger4": _bone(-57, 5, 100),
            }),
            "segmentDotsAgainstReference": {
                "left_shoulder_hand": 0.98,
                "left_upper_lower": 0.99,
                "left_lower_hand": 0.85,
                "right_shoulder_hand": 0.98,
                "right_upper_lower": 0.99,
                "right_lower_hand": 0.85,
            },
            "segmentDotsAgainstSource": {
                "left_shoulder_hand": 0.999,
                "left_upper_lower": 0.997,
                "left_lower_hand": 0.999,
                "right_shoulder_hand": 0.999,
                "right_upper_lower": 0.997,
                "right_lower_hand": 0.999,
            },
        }

        with self.assertRaises(ValidationError):
            validate_retarget_pose_report(report)

    def test_retarget_animation_validation_accepts_aligned_samples(self):
        source_samples = {
            "frames": [
                _frame(0, {
                    "L_Shoulder": (10, 0, 100),
                    "L_UpperArm": (20, 0, 90),
                    "L_Forearm": (35, 0, 75),
                    "L_Hand": (50, 0, 60),
                    "L_Finger1": (55, 5, 58),
                    "L_Finger4": (54, -5, 58),
                    "R_Shoulder": (-10, 0, 100),
                    "R_UpperArm": (-20, 0, 90),
                    "R_Forearm": (-35, 0, 75),
                    "R_Hand": (-50, 0, 60),
                    "R_Finger1": (-55, -5, 58),
                    "R_Finger4": (-54, 5, 58),
                }),
                _frame(10, {
                    "L_Shoulder": (11, 0, 101),
                    "L_UpperArm": (21, 1, 91),
                    "L_Forearm": (36, 1, 76),
                    "L_Hand": (51, 1, 61),
                    "L_Finger1": (56, 6, 59),
                    "L_Finger4": (55, -4, 59),
                    "R_Shoulder": (-11, 0, 101),
                    "R_UpperArm": (-21, -1, 91),
                    "R_Forearm": (-36, -1, 76),
                    "R_Hand": (-51, -1, 61),
                    "R_Finger1": (-56, -6, 59),
                    "R_Finger4": (-55, 4, 59),
                }),
            ]
        }
        target_samples = {
            "frames": [
                _frame(0, {
                    "clavicle_l": (10, 0, 100),
                    "upperarm_l": (20, 0, 90),
                    "lowerarm_l": (35, 0, 75),
                    "hand_l": (50, 0, 60),
                    "index_metacarpal_l": (55, 5, 58),
                    "pinky_metacarpal_l": (54, -5, 58),
                    "clavicle_r": (-10, 0, 100),
                    "upperarm_r": (-20, 0, 90),
                    "lowerarm_r": (-35, 0, 75),
                    "hand_r": (-50, 0, 60),
                    "index_metacarpal_r": (-55, -5, 58),
                    "pinky_metacarpal_r": (-54, 5, 58),
                }),
                _frame(10, {
                    "clavicle_l": (11, 0, 101),
                    "upperarm_l": (21, 1, 91),
                    "lowerarm_l": (36, 1, 76),
                    "hand_l": (51, 1, 61),
                    "index_metacarpal_l": (56, 6, 59),
                    "pinky_metacarpal_l": (55, -4, 59),
                    "clavicle_r": (-11, 0, 101),
                    "upperarm_r": (-21, -1, 91),
                    "lowerarm_r": (-36, -1, 76),
                    "hand_r": (-51, -1, 61),
                    "index_metacarpal_r": (-56, -6, 59),
                    "pinky_metacarpal_r": (-55, 4, 59),
                }),
            ]
        }

        summary = validate_retarget_animation_samples(source_samples, target_samples)

        self.assertTrue(summary["success"])
        self.assertGreaterEqual(summary["minimumDirectionDots"]["left_shoulder_hand"], 0.95)

    def test_retarget_animation_validation_rejects_misaligned_samples(self):
        source_samples = {
            "frames": [
                _frame(0, {
                    "L_Shoulder": (10, 0, 100),
                    "L_UpperArm": (20, 0, 90),
                    "L_Forearm": (35, 0, 75),
                    "L_Hand": (50, 0, 60),
                    "L_Finger1": (55, 5, 58),
                    "L_Finger4": (54, -5, 58),
                    "R_Shoulder": (-10, 0, 100),
                    "R_UpperArm": (-20, 0, 90),
                    "R_Forearm": (-35, 0, 75),
                    "R_Hand": (-50, 0, 60),
                    "R_Finger1": (-55, -5, 58),
                    "R_Finger4": (-54, 5, 58),
                }),
            ]
        }
        target_samples = {
            "frames": [
                _frame(0, {
                    "clavicle_l": (10, 0, 100),
                    "upperarm_l": (0, 25, 90),
                    "lowerarm_l": (-15, 40, 75),
                    "hand_l": (-30, 55, 60),
                    "index_metacarpal_l": (-25, 60, 58),
                    "pinky_metacarpal_l": (-26, 50, 58),
                    "clavicle_r": (-10, 0, 100),
                    "upperarm_r": (0, -25, 90),
                    "lowerarm_r": (15, -40, 75),
                    "hand_r": (30, -55, 60),
                    "index_metacarpal_r": (25, -60, 58),
                    "pinky_metacarpal_r": (26, -50, 58),
                }),
            ]
        }

        with self.assertRaises(ValidationError):
            validate_retarget_animation_samples(source_samples, target_samples)

    def test_retarget_animation_validation_rejects_wrist_basis_flip(self):
        source_samples = {
            "frames": [
                _frame(0, {
                    "L_Shoulder": (10, 0, 100),
                    "L_UpperArm": (20, 0, 90),
                    "L_Forearm": (35, 0, 75),
                    "L_Hand": (50, 0, 60),
                    "L_Finger1": (55, 5, 58),
                    "L_Finger4": (54, -5, 58),
                    "R_Shoulder": (-10, 0, 100),
                    "R_UpperArm": (-20, 0, 90),
                    "R_Forearm": (-35, 0, 75),
                    "R_Hand": (-50, 0, 60),
                    "R_Finger1": (-55, -5, 58),
                    "R_Finger4": (-54, 5, 58),
                }),
            ]
        }
        target_samples = {
            "frames": [
                _frame(0, {
                    "clavicle_l": (10, 0, 100),
                    "upperarm_l": (20, 0, 90),
                    "lowerarm_l": (35, 0, 75),
                    "hand_l": (50, 0, 60),
                    "index_metacarpal_l": (54, -5, 58),
                    "pinky_metacarpal_l": (55, 5, 58),
                    "clavicle_r": (-10, 0, 100),
                    "upperarm_r": (-20, 0, 90),
                    "lowerarm_r": (-35, 0, 75),
                    "hand_r": (-50, 0, 60),
                    "index_metacarpal_r": (-54, 5, 58),
                    "pinky_metacarpal_r": (-55, -5, 58),
                }),
            ]
        }

        with self.assertRaises(ValidationError):
            validate_retarget_animation_samples(source_samples, target_samples)

    def test_retarget_animation_validation_requires_overlapping_frames(self):
        source_samples = {
            "frames": [
                _frame(0, {
                    "L_Shoulder": (10, 0, 100),
                    "L_UpperArm": (20, 0, 90),
                    "L_Forearm": (35, 0, 75),
                    "L_Hand": (50, 0, 60),
                    "R_Shoulder": (-10, 0, 100),
                    "R_UpperArm": (-20, 0, 90),
                    "R_Forearm": (-35, 0, 75),
                    "R_Hand": (-50, 0, 60),
                }),
            ]
        }
        target_samples = {
            "frames": [
                _frame(10, {
                    "clavicle_l": (10, 0, 100),
                    "upperarm_l": (20, 0, 90),
                    "lowerarm_l": (35, 0, 75),
                    "hand_l": (50, 0, 60),
                    "clavicle_r": (-10, 0, 100),
                    "upperarm_r": (-20, 0, 90),
                    "lowerarm_r": (-35, 0, 75),
                    "hand_r": (-50, 0, 60),
                }),
            ]
        }

        with self.assertRaises(ValidationError):
            validate_retarget_animation_samples(source_samples, target_samples)

    def test_ue_import_report_validation_accepts_post_import_branch_basis_checks(self):
        report = {
            "success": True,
            "visiblePoseValidated": True,
            "skeletonImported": True,
            "hasPhysicsAsset": True,
            "relationships": {
                "skeletalMeshBound": True,
                "skeletonBound": True,
                "animationsCoverageComplete": True,
            },
            "postImportChecks": {
                "animationBranchBasisValidated": True,
                "rtgDiagnosticsRequiredForSuccess": False,
                "rtgDiagnosticsRole": "post-import-only",
            },
            "animationValidation": {
                "clips": [
                    {
                        "clip": "a000_201030",
                        "success": True,
                        "issues": [],
                        "minimumDirectionDots": {
                            "left_shoulder_hand": 0.999,
                            "left_upper_lower": 0.999,
                            "left_lower_hand": 0.999,
                            "right_shoulder_hand": 0.999,
                            "right_upper_lower": 0.999,
                            "right_lower_hand": 0.999,
                        },
                        "minimumHandBasisDots": {
                            "left_hand_palm_forward": 0.995,
                            "left_hand_palm_lateral": 0.995,
                            "left_hand_palm_normal": 0.995,
                            "right_hand_palm_forward": 0.995,
                            "right_hand_palm_lateral": 0.995,
                            "right_hand_palm_normal": 0.995,
                        },
                    }
                ]
            },
        }

        summary = validate_ue_import_report(report)

        self.assertTrue(summary["success"])
        self.assertEqual(summary["validatedClipCount"], 1)

    def test_ue_import_report_validation_rejects_rtg_required_success(self):
        report = {
            "success": True,
            "visiblePoseValidated": True,
            "skeletonImported": True,
            "hasPhysicsAsset": True,
            "relationships": {
                "skeletalMeshBound": True,
                "skeletonBound": True,
                "animationsCoverageComplete": True,
            },
            "postImportChecks": {
                "animationBranchBasisValidated": True,
                "rtgDiagnosticsRequiredForSuccess": True,
                "rtgDiagnosticsRole": "post-import-only",
            },
            "animationValidation": {
                "clips": [
                    {
                        "clip": "a000_201030",
                        "success": True,
                        "issues": [],
                        "minimumDirectionDots": {
                            "left_shoulder_hand": 0.999,
                            "left_upper_lower": 0.999,
                            "left_lower_hand": 0.999,
                            "right_shoulder_hand": 0.999,
                            "right_upper_lower": 0.999,
                            "right_lower_hand": 0.999,
                        },
                        "minimumHandBasisDots": {
                            "left_hand_palm_forward": 0.995,
                            "left_hand_palm_lateral": 0.995,
                            "left_hand_palm_normal": 0.995,
                            "right_hand_palm_forward": 0.995,
                            "right_hand_palm_lateral": 0.995,
                            "right_hand_palm_normal": 0.995,
                        },
                    }
                ]
            },
        }

        with self.assertRaises(ValidationError):
            validate_ue_import_report(report)


if __name__ == "__main__":
    unittest.main()