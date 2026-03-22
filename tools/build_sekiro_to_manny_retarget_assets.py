import json
import math

import unreal


SOURCE_MESH_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Mesh/c0000"
TARGET_MESH_PATH = "/Game/GhostSamurai_Bundle/Demo/Characters/Mannequins/Meshes/SKM_Manny"

SOURCE_RIG_PATH = "/Game/Retargeted/SekiroToManny/Rigs/IKR_Sekiro_c0000"
TARGET_RIG_PATH = "/Game/Retargeted/SekiroToManny/Rigs/IKR_Manny"
RETARGETER_PATH = "/Game/Retargeted/SekiroToManny/RTG_SekiroToManny"
RETARGETED_ANIMATIONS_PATH = "/Game/Retargeted/SekiroToManny/Animations"

SOURCE_ANIMATIONS_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Animations"
SOURCE_ANIMATION_NAMES = [
    "a000_201030",
    "a000_201050",
    "a000_202010",
    "a000_202011",
    "a000_202012",
    "a000_202035",
    "a000_202100",
    "a000_202110",
    "a000_202112",
    "a000_202300",
    "a000_202310",
    "a000_202400",
    "a000_202410",
    "a000_202600",
    "a000_202610",
    "a000_202700",
    "a000_202710",
]

SOURCE_ROOT_BONE = "Pelvis"
TARGET_ROOT_BONE = "pelvis"
SOURCE_CHAINS = [
    ("Root", "Master", "Pelvis"),
    ("Pelvis", "Pelvis", "Spine"),
    ("Spine", "Spine", "Spine2"),
    ("Neck", "Spine2", "Neck"),
    ("Head", "Neck", "Head"),
    ("Arm_L", "L_Clavicle", "L_Hand"),
    ("Thumb_L", "L_Finger0", "L_Finger02"),
    ("Index_L", "L_Finger1", "L_Finger12"),
    ("Middle_L", "L_Finger2", "L_Finger22"),
    ("Ring_L", "L_Finger3", "L_Finger32"),
    ("Pinky_L", "L_Finger4", "L_Finger42"),
    ("Arm_R", "R_Clavicle", "R_Hand"),
    ("Thumb_R", "R_Finger0", "R_Finger02"),
    ("Index_R", "R_Finger1", "R_Finger12"),
    ("Middle_R", "R_Finger2", "R_Finger22"),
    ("Ring_R", "R_Finger3", "R_Finger32"),
    ("Pinky_R", "R_Finger4", "R_Finger42"),
    ("Leg_L", "L_Thigh", "L_Foot"),
    ("Foot_L", "L_Foot", "L_Toe0"),
    ("Leg_R", "R_Thigh", "R_Foot"),
    ("Foot_R", "R_Foot", "R_Toe0"),
]

TARGET_CHAINS = [
    ("Root", "root", "pelvis"),
    ("Pelvis", "pelvis", "spine_01"),
    ("Spine", "spine_01", "spine_05"),
    ("Neck", "neck_01", "neck_02"),
    ("Head", "neck_02", "head"),
    ("Arm_L", "clavicle_l", "hand_l"),
    ("Thumb_L", "thumb_01_l", "thumb_03_l"),
    ("Index_L", "index_metacarpal_l", "index_03_l"),
    ("Middle_L", "middle_metacarpal_l", "middle_03_l"),
    ("Ring_L", "ring_metacarpal_l", "ring_03_l"),
    ("Pinky_L", "pinky_metacarpal_l", "pinky_03_l"),
    ("Arm_R", "clavicle_r", "hand_r"),
    ("Thumb_R", "thumb_01_r", "thumb_03_r"),
    ("Index_R", "index_metacarpal_r", "index_03_r"),
    ("Middle_R", "middle_metacarpal_r", "middle_03_r"),
    ("Ring_R", "ring_metacarpal_r", "ring_03_r"),
    ("Pinky_R", "pinky_metacarpal_r", "pinky_03_r"),
    ("Leg_L", "thigh_l", "foot_l"),
    ("Foot_L", "foot_l", "ball_l"),
    ("Leg_R", "thigh_r", "foot_r"),
    ("Foot_R", "foot_r", "ball_r"),
]

REQUIRED_CHAIN_MAPPINGS = {
    "Pelvis",
    "Spine",
    "Neck",
    "Head",
    "Arm_L",
    "Thumb_L",
    "Index_L",
    "Middle_L",
    "Ring_L",
    "Pinky_L",
    "Arm_R",
    "Thumb_R",
    "Index_R",
    "Middle_R",
    "Ring_R",
    "Pinky_R",
    "Leg_L",
    "Foot_L",
    "Leg_R",
    "Foot_R",
}

FK_CHAIN_ROTATION_MODES = {
    "Arm_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Thumb_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Index_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Middle_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Ring_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Pinky_L": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Arm_R": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Thumb_R": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Index_R": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Middle_R": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Ring_R": unreal.FKChainRotationMode.MATCH_CHAIN,
    "Pinky_R": unreal.FKChainRotationMode.MATCH_CHAIN,
}

ARM_POSE_BONE_MAPPINGS = [
    ("L_Clavicle", "clavicle_l"),
    ("L_UpperArm", "upperarm_l"),
    ("L_Forearm", "lowerarm_l"),
    ("L_Hand", "hand_l"),
    ("R_Clavicle", "clavicle_r"),
    ("R_UpperArm", "upperarm_r"),
    ("R_Forearm", "lowerarm_r"),
    ("R_Hand", "hand_r"),
]

ARM_SEGMENT_DIRECTION_MAPPINGS = [
    ("L_Clavicle", "L_Hand", "clavicle_l", "hand_l"),
    ("L_UpperArm", "L_Hand", "upperarm_l", "hand_l"),
    ("L_Forearm", "L_Hand", "lowerarm_l", "hand_l"),
    ("R_Clavicle", "R_Hand", "clavicle_r", "hand_r"),
    ("R_UpperArm", "R_Hand", "upperarm_r", "hand_r"),
    ("R_Forearm", "R_Hand", "lowerarm_r", "hand_r"),
]

HAND_BASIS_MAPPINGS = [
    (
        "L_Hand",
        "L_Finger1",
        "L_Finger2",
        "L_Finger4",
        "hand_l",
        "index_metacarpal_l",
        "middle_metacarpal_l",
        "pinky_metacarpal_l",
    ),
    (
        "R_Hand",
        "R_Finger1",
        "R_Finger2",
        "R_Finger4",
        "hand_r",
        "index_metacarpal_r",
        "middle_metacarpal_r",
        "pinky_metacarpal_r",
    ),
]

FINGER_POSE_BONE_MAPPINGS = [
    ("L_Finger0", "thumb_01_l"),
    ("L_Finger1", "index_metacarpal_l"),
    ("L_Finger2", "middle_metacarpal_l"),
    ("L_Finger3", "ring_metacarpal_l"),
    ("L_Finger4", "pinky_metacarpal_l"),
    ("R_Finger0", "thumb_01_r"),
    ("R_Finger1", "index_metacarpal_r"),
    ("R_Finger2", "middle_metacarpal_r"),
    ("R_Finger3", "ring_metacarpal_r"),
    ("R_Finger4", "pinky_metacarpal_r"),
]

TARGET_AUTO_ALIGN_BONES = []


def log(message: str) -> None:
    unreal.log_warning(message)


def ensure_asset_directory(object_path: str) -> None:
    directory = object_path.rsplit("/", 1)[0]
    if not unreal.EditorAssetLibrary.does_directory_exist(directory):
        unreal.EditorAssetLibrary.make_directory(directory)


def recreate_directory(directory_path: str) -> None:
    if unreal.EditorAssetLibrary.does_directory_exist(directory_path):
        if not unreal.EditorAssetLibrary.delete_directory(directory_path):
            raise RuntimeError(f"failed to delete generated directory before rebuild: {directory_path}")
    if not unreal.EditorAssetLibrary.make_directory(directory_path):
        raise RuntimeError(f"failed to create generated directory: {directory_path}")


def load_required_asset(object_path: str):
    asset = unreal.load_asset(object_path)
    if asset is None:
        raise RuntimeError(f"required asset was not found: {object_path}")
    return asset


def parse_json(json_text: str):
    parsed = json.loads(json_text)
    if "error" in parsed:
        raise RuntimeError(parsed["error"])
    return parsed


def get_skeleton(mesh):
    skeleton = mesh.skeleton
    if skeleton is None:
        raise RuntimeError(f"mesh has no skeleton: {mesh.get_path_name()}")
    return skeleton


def get_reference_pose_maps(mesh):
    branch_json = parse_json(unreal.SekiroRetargetDiagnosticsLibrary.dump_skeletal_mesh_reference_pose_to_json(mesh, []))
    local_rotations = {}
    local_translations = {}
    parent_bones = {}
    component_rotations = {}
    component_translations = {}
    for entry in branch_json["bones"]:
        bone_name = entry["bone"]
        local_rotations[bone_name] = quat_from_json(entry["refLocal"]["rotation"])
        local_translations[bone_name] = vec_from_json(entry["refLocal"]["translation"])
        parent_bones[bone_name] = entry["parentBone"] or None
        component_rotations[bone_name] = quat_from_json(entry["refComponent"]["rotation"])
        component_translations[bone_name] = vec_from_json(entry["refComponent"]["translation"])

    return local_rotations, local_translations, parent_bones, component_rotations, component_translations


def vec_from_json(vec_json):
    return (
        float(vec_json["x"]),
        float(vec_json["y"]),
        float(vec_json["z"]),
    )


def vec_dot(lhs, rhs):
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]


def vec_cross(lhs, rhs):
    return (
        lhs[1] * rhs[2] - lhs[2] * rhs[1],
        lhs[2] * rhs[0] - lhs[0] * rhs[2],
        lhs[0] * rhs[1] - lhs[1] * rhs[0],
    )


def vec_add(lhs, rhs):
    return (lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2])


def vec_sub(lhs, rhs):
    return (lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2])


def vec_scale(vec, scalar):
    return (vec[0] * scalar, vec[1] * scalar, vec[2] * scalar)


def vec_normalize(vec):
    length = (vec[0] ** 2 + vec[1] ** 2 + vec[2] ** 2) ** 0.5
    if length == 0:
        raise RuntimeError("cannot normalize zero vector")
    return (vec[0] / length, vec[1] / length, vec[2] / length)


def vec_project_onto_plane(vec, plane_normal):
    normal = vec_normalize(plane_normal)
    return vec_sub(vec, vec_scale(normal, vec_dot(vec, normal)))


def quat_rotate_vector(quat, vec):
    vec_quat = (vec[0], vec[1], vec[2], 0.0)
    rotated = quat_multiply(quat_multiply(quat, vec_quat), quat_inverse(quat))
    return (rotated[0], rotated[1], rotated[2])


def quat_between_vectors(current_vec, desired_vec):
    current_dir = vec_normalize(current_vec)
    desired_dir = vec_normalize(desired_vec)
    dot = max(-1.0, min(1.0, vec_dot(current_dir, desired_dir)))

    if dot > 0.999999:
        return (0.0, 0.0, 0.0, 1.0)

    if dot < -0.999999:
        if abs(current_dir[0]) < 0.9:
            orthogonal = (1.0, 0.0, 0.0)
        else:
            orthogonal = (0.0, 1.0, 0.0)
        axis = vec_normalize(vec_cross(current_dir, orthogonal))
        return (axis[0], axis[1], axis[2], 0.0)

    axis = vec_cross(current_dir, desired_dir)
    quat = (axis[0], axis[1], axis[2], 1.0 + dot)
    return quat_normalize(quat)


def quat_normalize(quat):
    length = (quat[0] ** 2 + quat[1] ** 2 + quat[2] ** 2 + quat[3] ** 2) ** 0.5
    if length == 0:
        raise RuntimeError("cannot normalize zero quaternion")
    return (quat[0] / length, quat[1] / length, quat[2] / length, quat[3] / length)


def quat_from_axis_angle(axis, angle_radians):
    normalized_axis = vec_normalize(axis)
    half_angle = angle_radians * 0.5
    sin_half_angle = unreal.MathLibrary.sin(half_angle)
    return quat_normalize(
        (
            normalized_axis[0] * sin_half_angle,
            normalized_axis[1] * sin_half_angle,
            normalized_axis[2] * sin_half_angle,
            unreal.MathLibrary.cos(half_angle),
        )
    )


def signed_angle_between_on_axis(current_vec, desired_vec, axis):
    projected_current = vec_project_onto_plane(current_vec, axis)
    projected_desired = vec_project_onto_plane(desired_vec, axis)
    normalized_current = vec_normalize(projected_current)
    normalized_desired = vec_normalize(projected_desired)
    clamped_dot = max(-1.0, min(1.0, vec_dot(normalized_current, normalized_desired)))
    signed_sin = vec_dot(axis, vec_cross(normalized_current, normalized_desired))
    return unreal.MathLibrary.atan2(signed_sin, clamped_dot)


def quat_inverse(quat):
    normalized = quat_normalize(quat)
    return (-normalized[0], -normalized[1], -normalized[2], normalized[3])


def quat_multiply(lhs, rhs):
    x1, y1, z1, w1 = lhs
    x2, y2, z2, w2 = rhs
    return (
        w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
        w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
        w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
        w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
    )


def quat_from_json(quat_json):
    return (
        float(quat_json["x"]),
        float(quat_json["y"]),
        float(quat_json["z"]),
        float(quat_json["w"]),
    )


def build_body_basis(component_translations, left_shoulder_bone, right_shoulder_bone, pelvis_bone, head_bone):
    lateral_axis = vec_normalize(vec_sub(component_translations[right_shoulder_bone], component_translations[left_shoulder_bone]))
    up_axis = vec_normalize(vec_sub(component_translations[head_bone], component_translations[pelvis_bone]))
    forward_axis = vec_normalize(vec_cross(lateral_axis, up_axis))
    orthonormal_lateral_axis = vec_normalize(vec_cross(up_axis, forward_axis))

    return {
        "forward": forward_axis,
        "lateral": orthonormal_lateral_axis,
        "up": up_axis,
    }


def remap_direction_between_body_bases(direction, source_basis, target_basis):
    source_forward = vec_dot(direction, source_basis["forward"])
    source_lateral = vec_dot(direction, source_basis["lateral"])
    source_up = vec_dot(direction, source_basis["up"])

    remapped_direction = vec_add(
        vec_add(
            vec_scale(target_basis["forward"], source_forward),
            vec_scale(target_basis["lateral"], source_lateral),
        ),
        vec_scale(target_basis["up"], source_up),
    )
    return vec_normalize(remapped_direction)


def set_target_pose_from_reference_alignment(controller, source_mesh, target_mesh):
    source_local_rotations, _, _, _, source_component_translations = get_reference_pose_maps(source_mesh)
    target_local_rotations, target_local_translations, target_parent_bones, _, _ = get_reference_pose_maps(target_mesh)
    target_pose_name = controller.get_current_retarget_pose_name(unreal.RetargetSourceOrTarget.TARGET)
    controller.reset_retarget_pose(target_pose_name, [], unreal.RetargetSourceOrTarget.TARGET)

    resolved_target_local_rotations = {}
    resolved_target_component_rotations = {}
    resolved_target_component_translations = {}

    def clear_resolved_component_transforms():
        resolved_target_component_rotations.clear()
        resolved_target_component_translations.clear()

    def set_target_pose_bone(target_bone, desired_target_local_rotation, source_bone):
        delta_rotation = quat_normalize(
            quat_multiply(desired_target_local_rotation, quat_inverse(target_local_rotations[target_bone]))
        )
        controller.set_rotation_offset_for_retarget_pose_bone(
            unreal.Name(target_bone),
            unreal.Quat(delta_rotation[0], delta_rotation[1], delta_rotation[2], delta_rotation[3]),
            unreal.RetargetSourceOrTarget.TARGET,
        )
        resolved_target_local_rotations[target_bone] = desired_target_local_rotation
        clear_resolved_component_transforms()
        log(f"  target pose {target_bone} <= {source_bone}: {delta_rotation}")

    def get_current_target_local_rotation(target_bone):
        return resolved_target_local_rotations.get(target_bone, target_local_rotations[target_bone])

    def get_current_target_component_rotation(target_bone):
        if target_bone in resolved_target_component_rotations:
            return resolved_target_component_rotations[target_bone]

        target_parent_bone = target_parent_bones.get(target_bone)
        local_rotation = get_current_target_local_rotation(target_bone)
        if target_parent_bone and target_parent_bone in target_local_rotations:
            parent_component_rotation = get_current_target_component_rotation(target_parent_bone)
            component_rotation = quat_normalize(quat_multiply(local_rotation, parent_component_rotation))
        else:
            component_rotation = local_rotation

        resolved_target_component_rotations[target_bone] = component_rotation
        return component_rotation

    def get_current_target_component_translation(target_bone):
        if target_bone in resolved_target_component_translations:
            return resolved_target_component_translations[target_bone]

        target_parent_bone = target_parent_bones.get(target_bone)
        local_translation = target_local_translations[target_bone]
        if target_parent_bone and target_parent_bone in target_local_rotations:
            parent_component_rotation = get_current_target_component_rotation(target_parent_bone)
            parent_component_translation = get_current_target_component_translation(target_parent_bone)
            component_translation = vec_add(
                quat_rotate_vector(parent_component_rotation, local_translation),
                parent_component_translation,
            )
        else:
            component_translation = local_translation

        resolved_target_component_translations[target_bone] = component_translation
        return component_translation

    for source_bone, target_bone in ARM_POSE_BONE_MAPPINGS:
        if source_bone not in source_local_rotations:
            raise RuntimeError(f"source arm alignment bone missing from reference map: {source_bone}")
        if target_bone not in target_local_rotations:
            raise RuntimeError(f"target arm alignment bone missing from reference map: {target_bone}")
        set_target_pose_bone(target_bone, source_local_rotations[source_bone], source_bone)

    for source_bone, source_child_bone, target_bone, target_child_bone in ARM_SEGMENT_DIRECTION_MAPPINGS:
        if source_bone not in source_component_translations or source_child_bone not in source_component_translations:
            raise RuntimeError(f"source arm segment missing from reference map: {source_bone} -> {source_child_bone}")
        if target_bone not in target_local_rotations or target_child_bone not in target_local_rotations:
            raise RuntimeError(f"target arm segment missing from reference map: {target_bone} -> {target_child_bone}")

        desired_target_direction = vec_normalize(
            vec_sub(source_component_translations[source_child_bone], source_component_translations[source_bone])
        )
        current_target_component_rotation = get_current_target_component_rotation(target_bone)
        current_target_direction = vec_normalize(
            vec_sub(
                get_current_target_component_translation(target_child_bone),
                get_current_target_component_translation(target_bone),
            )
        )
        component_delta_rotation = quat_between_vectors(current_target_direction, desired_target_direction)
        desired_target_component_rotation = quat_normalize(
            quat_multiply(component_delta_rotation, current_target_component_rotation)
        )

        target_parent_bone = target_parent_bones.get(target_bone)
        if target_parent_bone and target_parent_bone in target_local_rotations:
            target_parent_component_rotation = get_current_target_component_rotation(target_parent_bone)
        else:
            target_parent_component_rotation = (0.0, 0.0, 0.0, 1.0)

        desired_target_local_rotation = quat_normalize(
            quat_multiply(desired_target_component_rotation, quat_inverse(target_parent_component_rotation))
        )
        set_target_pose_bone(target_bone, desired_target_local_rotation, f"{source_bone} -> {source_child_bone}")

    for (
        source_hand_bone,
        source_index_bone,
        source_middle_bone,
        source_pinky_bone,
        target_hand_bone,
        target_index_bone,
        target_middle_bone,
        target_pinky_bone,
    ) in HAND_BASIS_MAPPINGS:
        required_source_bones = [source_hand_bone, source_index_bone, source_middle_bone, source_pinky_bone]
        required_target_bones = [target_hand_bone, target_index_bone, target_middle_bone, target_pinky_bone]
        for source_bone in required_source_bones:
            if source_bone not in source_component_translations:
                raise RuntimeError(f"source hand basis bone missing from reference map: {source_bone}")
        for target_bone in required_target_bones:
            if target_bone not in target_local_rotations:
                raise RuntimeError(f"target hand basis bone missing from reference map: {target_bone}")

        desired_palm_forward = vec_normalize(
            vec_sub(source_component_translations[source_middle_bone], source_component_translations[source_hand_bone])
        )
        desired_palm_lateral = vec_normalize(
            vec_sub(source_component_translations[source_index_bone], source_component_translations[source_pinky_bone])
        )
        current_palm_forward = vec_normalize(
            vec_sub(
                get_current_target_component_translation(target_middle_bone),
                get_current_target_component_translation(target_hand_bone),
            )
        )
        current_palm_lateral = vec_normalize(
            vec_sub(
                get_current_target_component_translation(target_index_bone),
                get_current_target_component_translation(target_pinky_bone),
            )
        )

        palm_forward_alignment_rotation = quat_between_vectors(current_palm_forward, desired_palm_forward)
        aligned_target_palm_lateral = quat_rotate_vector(palm_forward_alignment_rotation, current_palm_lateral)
        palm_roll_angle = signed_angle_between_on_axis(
            aligned_target_palm_lateral,
            desired_palm_lateral,
            desired_palm_forward,
        )
        palm_roll_rotation = quat_from_axis_angle(desired_palm_forward, palm_roll_angle)
        current_target_component_rotation = get_current_target_component_rotation(target_hand_bone)
        desired_target_component_rotation = quat_normalize(
            quat_multiply(
                quat_multiply(palm_roll_rotation, palm_forward_alignment_rotation),
                current_target_component_rotation,
            )
        )

        target_parent_bone = target_parent_bones.get(target_hand_bone)
        if target_parent_bone and target_parent_bone in target_local_rotations:
            target_parent_component_rotation = get_current_target_component_rotation(target_parent_bone)
        else:
            target_parent_component_rotation = (0.0, 0.0, 0.0, 1.0)

        desired_target_local_rotation = quat_normalize(
            quat_multiply(desired_target_component_rotation, quat_inverse(target_parent_component_rotation))
        )
        set_target_pose_bone(
            target_hand_bone,
            desired_target_local_rotation,
            f"{source_hand_bone} palm basis ({source_middle_bone}, {source_index_bone}/{source_pinky_bone})",
        )

    for source_bone, target_bone in FINGER_POSE_BONE_MAPPINGS:
        if source_bone not in source_local_rotations:
            raise RuntimeError(f"source alignment bone missing from reference map: {source_bone}")
        if target_bone not in target_local_rotations:
            raise RuntimeError(f"target alignment bone missing from reference map: {target_bone}")
        set_target_pose_bone(target_bone, source_local_rotations[source_bone], source_bone)

    if TARGET_AUTO_ALIGN_BONES:
        controller.auto_align_bones(
            [unreal.Name(bone_name) for bone_name in TARGET_AUTO_ALIGN_BONES],
            unreal.RetargetAutoAlignMethod.CHAIN_TO_CHAIN,
            unreal.RetargetSourceOrTarget.TARGET,
        )
        log(f"  auto aligned target hand/finger bones: {TARGET_AUTO_ALIGN_BONES}")


def load_or_create_asset(object_path: str, asset_class, factory_class):
    asset = unreal.load_asset(object_path)
    if asset is not None:
        return asset

    ensure_asset_directory(object_path)
    asset_name = object_path.rsplit("/", 1)[1]
    package_path = object_path.rsplit("/", 1)[0]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    asset = asset_tools.create_asset(asset_name, package_path, asset_class, factory_class())
    if asset is None:
        raise RuntimeError(f"failed to create asset: {object_path}")
    return asset


def recreate_asset(object_path: str, asset_class, factory_class):
    if unreal.EditorAssetLibrary.does_asset_exist(object_path):
        if not unreal.EditorAssetLibrary.delete_asset(object_path):
            raise RuntimeError(f"failed to delete generated asset before rebuild: {object_path}")
    return load_or_create_asset(object_path, asset_class, factory_class)


def clear_retarget_chains(controller) -> None:
    existing_chain_names = [chain.chain_name for chain in controller.get_retarget_chains()]
    for chain_name in existing_chain_names:
        if not controller.remove_retarget_chain(chain_name):
            raise RuntimeError(f"failed to remove preexisting retarget chain '{chain_name}'")


def configure_rig(rig_path: str, mesh_path: str, retarget_root: str, chain_defs):
    mesh = load_required_asset(mesh_path)
    rig = recreate_asset(rig_path, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory)
    controller = unreal.IKRigController.get_controller(rig)

    if controller is None:
        raise RuntimeError(f"failed to get IKRigController for {rig_path}")

    if not controller.set_skeletal_mesh(mesh):
        raise RuntimeError(f"failed to assign preview skeletal mesh {mesh_path} to {rig_path}")

    clear_retarget_chains(controller)

    if not controller.set_retarget_root(retarget_root):
        raise RuntimeError(f"failed to set retarget root '{retarget_root}' on {rig_path}")

    actual_chain_names = {}
    for chain_name, start_bone, end_bone in chain_defs:
        created_name = controller.add_retarget_chain(chain_name, start_bone, end_bone, "")
        actual_chain_names[chain_name] = str(created_name)
        if str(created_name) != chain_name:
            log(f"rig {rig_path}: requested chain '{chain_name}' created as '{created_name}'")
        if not controller.set_retarget_chain_start_bone(created_name, start_bone):
            raise RuntimeError(f"failed to set start bone '{start_bone}' for chain '{created_name}' on {rig_path}")
        if not controller.set_retarget_chain_end_bone(created_name, end_bone):
            raise RuntimeError(f"failed to set end bone '{end_bone}' for chain '{created_name}' on {rig_path}")

    if not unreal.EditorAssetLibrary.save_loaded_asset(rig):
        raise RuntimeError(f"failed to save IK rig asset: {rig_path}")

    log(f"Configured IK rig: {rig_path}")
    for chain_name, start_bone, end_bone in chain_defs:
        actual_name = actual_chain_names[chain_name]
        actual_start = controller.get_retarget_chain_start_bone(actual_name)
        actual_end = controller.get_retarget_chain_end_bone(actual_name)
        log(f"  {chain_name}: {actual_start} -> {actual_end}")

    return rig, mesh, actual_chain_names


def configure_retargeter(source_rig, target_rig, source_mesh, target_mesh, source_chain_names, target_chain_names):
    retargeter = recreate_asset(RETARGETER_PATH, unreal.IKRetargeter, unreal.IKRetargetFactory)
    controller = unreal.IKRetargeterController.get_controller(retargeter)

    if controller is None:
        raise RuntimeError(f"failed to get IKRetargeterController for {RETARGETER_PATH}")

    controller.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source_mesh)
    controller.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target_mesh)
    controller.remove_all_ops()
    controller.add_default_ops()
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.SOURCE, source_rig)
    controller.assign_ik_rig_to_all_ops(unreal.RetargetSourceOrTarget.TARGET, target_rig)
    for op_index in range(controller.get_num_retarget_ops()):
        controller.run_op_initial_setup(op_index)
    controller.auto_map_chains(unreal.AutoMapChainType.EXACT, True)

    for chain_name, _, _ in TARGET_CHAINS:
        source_chain_name = source_chain_names[chain_name]
        target_chain_name = target_chain_names[chain_name]
        mapped = controller.set_source_chain(source_chain_name, target_chain_name)
        if not mapped and chain_name in REQUIRED_CHAIN_MAPPINGS:
            raise RuntimeError(f"failed to map target chain '{target_chain_name}' to source chain '{source_chain_name}'")
        if not mapped:
            log(f"  optional chain mapping skipped: {chain_name}")

    fk_op_controller = None
    for op_index in range(controller.get_num_retarget_ops()):
        candidate = controller.get_op_controller(op_index)
        if isinstance(candidate, unreal.IKRetargetFKChainsController):
            fk_op_controller = candidate
            break

    if fk_op_controller is None:
        raise RuntimeError("failed to locate FK Chains retarget op controller")

    fk_op_settings = fk_op_controller.get_settings()
    fk_chain_settings = list(fk_op_settings.get_editor_property("chains_to_retarget"))
    updated_chain_names = set()
    for chain_settings in fk_chain_settings:
        target_chain_name = str(chain_settings.get_editor_property("target_chain_name"))
        if target_chain_name not in FK_CHAIN_ROTATION_MODES:
            continue
        rotation_mode = FK_CHAIN_ROTATION_MODES[target_chain_name]
        chain_settings.set_editor_property("rotation_mode", rotation_mode)
        updated_chain_names.add(target_chain_name)
        log(f"  FK op settings {target_chain_name}: rotation_mode={rotation_mode.name}")

    if updated_chain_names != set(FK_CHAIN_ROTATION_MODES):
        missing_chain_names = sorted(set(FK_CHAIN_ROTATION_MODES) - updated_chain_names)
        raise RuntimeError(f"failed to find FK chain settings for: {missing_chain_names}")

    fk_op_settings.set_editor_property("chains_to_retarget", fk_chain_settings)
    fk_op_controller.set_settings(fk_op_settings)

    set_target_pose_from_reference_alignment(controller, source_mesh, target_mesh)

    if not unreal.EditorAssetLibrary.save_loaded_asset(retargeter):
        raise RuntimeError(f"failed to save IK retargeter asset: {RETARGETER_PATH}")

    log(f"Configured IK retargeter: {RETARGETER_PATH}")
    for chain_name, _, _ in TARGET_CHAINS:
        target_chain_name = target_chain_names[chain_name]
        source_chain = controller.get_source_chain(target_chain_name)
        log(f"  {chain_name}: {source_chain} -> {target_chain_name}")

    if str(controller.get_source_chain(target_chain_names["Spine"])) != source_chain_names["Spine"]:
        raise RuntimeError("retargeter spine chain was not mapped to source Spine")

    return retargeter


def get_asset_data(asset_path: str):
    asset_data = unreal.EditorAssetLibrary.find_asset_data(asset_path)
    if not asset_data.is_valid():
        raise RuntimeError(f"missing asset data for {asset_path}")
    return asset_data


def move_retargeted_assets(asset_data_list, destination_path: str):
    ensure_asset_directory(f"{destination_path}/_placeholder")
    moved_asset_paths = []
    for asset_data in asset_data_list:
        source_asset_path = str(asset_data.package_name)
        asset_name = str(asset_data.asset_name)
        destination_asset_path = f"{destination_path}/{asset_name}"

        if unreal.EditorAssetLibrary.does_asset_exist(destination_asset_path):
            if not unreal.EditorAssetLibrary.delete_asset(destination_asset_path):
                raise RuntimeError(f"failed to delete existing retargeted animation: {destination_asset_path}")

        if source_asset_path != destination_asset_path:
            if not unreal.EditorAssetLibrary.rename_asset(source_asset_path, destination_asset_path):
                raise RuntimeError(f"failed to move retargeted animation from {source_asset_path} to {destination_asset_path}")

        moved_asset_paths.append(destination_asset_path)

    return moved_asset_paths


def retarget_animation_assets(source_mesh, target_mesh, retargeter):
    recreate_directory(RETARGETED_ANIMATIONS_PATH)
    source_assets = [get_asset_data(f"{SOURCE_ANIMATIONS_PATH}/{animation_name}") for animation_name in SOURCE_ANIMATION_NAMES]
    retargeted_assets = unreal.IKRetargetBatchOperation.duplicate_and_retarget(
        source_assets,
        source_mesh,
        target_mesh,
        retargeter,
        "",
        "",
        "",
        "_Manny",
        False,
        True,
    )

    if len(retargeted_assets) != len(SOURCE_ANIMATION_NAMES):
        raise RuntimeError(f"expected {len(SOURCE_ANIMATION_NAMES)} retargeted animations but received {len(retargeted_assets)}")

    moved_asset_paths = move_retargeted_assets(retargeted_assets, RETARGETED_ANIMATIONS_PATH)
    log(f"Configured retargeted animations: {RETARGETED_ANIMATIONS_PATH}")
    for asset_path in moved_asset_paths:
        log(f"  {asset_path}")


def main() -> None:
    source_rig, source_mesh, source_chain_names = configure_rig(SOURCE_RIG_PATH, SOURCE_MESH_PATH, SOURCE_ROOT_BONE, SOURCE_CHAINS)
    target_rig, target_mesh, target_chain_names = configure_rig(TARGET_RIG_PATH, TARGET_MESH_PATH, TARGET_ROOT_BONE, TARGET_CHAINS)
    retargeter = configure_retargeter(source_rig, target_rig, source_mesh, target_mesh, source_chain_names, target_chain_names)
    retarget_animation_assets(source_mesh, target_mesh, retargeter)
    log("Sekiro-to-Manny retarget assets updated successfully.")


main()