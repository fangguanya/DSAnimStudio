import json
from pathlib import Path

import unreal


SOURCE_MESH_PATH = "/Game/SekiroAssets/Export17LeftArmFinal_Z180/c0000/Mesh/c0000"
TARGET_MESH_PATH = "/Game/GhostSamurai_Bundle/Demo/Characters/Mannequins/Meshes/SKM_Manny"
RETARGETER_PATH = "/Game/Retargeted/SekiroToManny/RTG_SekiroToManny"
OUTPUT_PATH = Path(r"E:\Sekiro\DSAnimStudio\_ExportCheck\UserVerifyRun\HandRetargetDiagnostic\target_retarget_pose_preview.json")
ARM_CHAINS = [
	("left_shoulder_hand", "clavicle_l", "hand_l"),
	("left_upper_lower", "upperarm_l", "lowerarm_l"),
	("left_lower_hand", "lowerarm_l", "hand_l"),
	("right_shoulder_hand", "clavicle_r", "hand_r"),
	("right_upper_lower", "upperarm_r", "lowerarm_r"),
	("right_lower_hand", "lowerarm_r", "hand_r"),
]
SOURCE_ARM_CHAINS = [
	("left_shoulder_hand", "L_Shoulder", "L_Hand"),
	("left_upper_lower", "L_UpperArm", "L_Forearm"),
	("left_lower_hand", "L_Forearm", "L_Hand"),
	("right_shoulder_hand", "R_Shoulder", "R_Hand"),
	("right_upper_lower", "R_UpperArm", "R_Forearm"),
	("right_lower_hand", "R_Forearm", "R_Hand"),
]
SOURCE_HAND_BASIS_BONES = ["L_Finger1", "L_Finger2", "L_Finger4", "R_Finger1", "R_Finger2", "R_Finger4"]


def parse_json(json_text: str):
	parsed = json.loads(json_text)
	if "error" in parsed:
		raise RuntimeError(parsed["error"])
	return parsed


def quat_from_json(quat_json):
	return (
		float(quat_json["x"]),
		float(quat_json["y"]),
		float(quat_json["z"]),
		float(quat_json["w"]),
	)


def vec_from_json(vec_json):
	return (
		float(vec_json["x"]),
		float(vec_json["y"]),
		float(vec_json["z"]),
	)


def quat_normalize(quat):
	length = (quat[0] ** 2 + quat[1] ** 2 + quat[2] ** 2 + quat[3] ** 2) ** 0.5
	if length == 0:
		raise RuntimeError("cannot normalize zero quaternion")
	return (quat[0] / length, quat[1] / length, quat[2] / length, quat[3] / length)


def quat_multiply(lhs, rhs):
	x1, y1, z1, w1 = lhs
	x2, y2, z2, w2 = rhs
	return (
		w1 * x2 + x1 * w2 + y1 * z2 - z1 * y2,
		w1 * y2 - x1 * z2 + y1 * w2 + z1 * x2,
		w1 * z2 + x1 * y2 - y1 * x2 + z1 * w2,
		w1 * w2 - x1 * x2 - y1 * y2 - z1 * z2,
	)


def quat_inverse(quat):
	normalized = quat_normalize(quat)
	return (-normalized[0], -normalized[1], -normalized[2], normalized[3])


def quat_rotate_vector(quat, vec):
	vec_quat = (vec[0], vec[1], vec[2], 0.0)
	rotated = quat_multiply(quat_multiply(quat, vec_quat), quat_inverse(quat))
	return (rotated[0], rotated[1], rotated[2])


def vec_add(lhs, rhs):
	return (lhs[0] + rhs[0], lhs[1] + rhs[1], lhs[2] + rhs[2])


def vec_sub(lhs, rhs):
	return (lhs[0] - rhs[0], lhs[1] - rhs[1], lhs[2] - rhs[2])


def vec_normalize(vec):
	length = (vec[0] ** 2 + vec[1] ** 2 + vec[2] ** 2) ** 0.5
	if length == 0:
		return (0.0, 0.0, 0.0)
	return (vec[0] / length, vec[1] / length, vec[2] / length)


def vec_dot(lhs, rhs):
	return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2]


source_mesh = unreal.load_asset(SOURCE_MESH_PATH)
target_mesh = unreal.load_asset(TARGET_MESH_PATH)
retargeter = unreal.load_asset(RETARGETER_PATH)
if source_mesh is None:
	raise RuntimeError(f"missing source mesh: {SOURCE_MESH_PATH}")
if target_mesh is None:
	raise RuntimeError(f"missing target mesh: {TARGET_MESH_PATH}")
if retargeter is None:
	raise RuntimeError(f"missing retargeter: {RETARGETER_PATH}")

source_pose_json = parse_json(unreal.SekiroRetargetDiagnosticsLibrary.dump_skeletal_mesh_reference_pose_to_json(source_mesh, []))
target_pose_json = parse_json(unreal.SekiroRetargetDiagnosticsLibrary.dump_skeletal_mesh_reference_pose_to_json(target_mesh, []))
pose_json = parse_json(
	unreal.SekiroRetargetDiagnosticsLibrary.dump_retarget_pose_to_json(
		retargeter,
		unreal.RetargetSourceOrTarget.TARGET,
		[],
	)
)

delta_by_bone = {}
for entry in pose_json["bones"]:
	delta_by_bone[entry["bone"]] = quat_from_json(entry["deltaRotation"])

ref_local_rotation_by_bone = {}
ref_local_translation_by_bone = {}
parent_by_bone = {}
reference_component_rotation_by_bone = {}
reference_component_translation_by_bone = {}
for entry in target_pose_json["bones"]:
	bone = entry["bone"]
	parent_by_bone[bone] = entry["parentBone"] or None
	ref_local_rotation_by_bone[bone] = quat_from_json(entry["refLocal"]["rotation"])
	ref_local_translation_by_bone[bone] = vec_from_json(entry["refLocal"]["translation"])
	reference_component_rotation_by_bone[bone] = quat_from_json(entry["refComponent"]["rotation"])
	reference_component_translation_by_bone[bone] = vec_from_json(entry["refComponent"]["translation"])

posed_local_rotation_by_bone = {}
for bone, ref_local_rotation in ref_local_rotation_by_bone.items():
	delta_rotation = delta_by_bone.get(bone, (0.0, 0.0, 0.0, 1.0))
	posed_local_rotation_by_bone[bone] = quat_normalize(quat_multiply(delta_rotation, ref_local_rotation))

component_rotation_by_bone = {}
component_translation_by_bone = {}


def resolve_component_transform(bone):
	if bone in component_rotation_by_bone:
		return component_rotation_by_bone[bone], component_translation_by_bone[bone]

	parent = parent_by_bone.get(bone)
	local_rotation = posed_local_rotation_by_bone[bone]
	local_translation = ref_local_translation_by_bone[bone]

	if parent is None or parent not in parent_by_bone:
		component_rotation = local_rotation
		component_translation = local_translation
	else:
		parent_rotation, parent_translation = resolve_component_transform(parent)
		component_rotation = quat_normalize(quat_multiply(local_rotation, parent_rotation))
		component_translation = vec_add(quat_rotate_vector(parent_rotation, local_translation), parent_translation)

	component_rotation_by_bone[bone] = component_rotation
	component_translation_by_bone[bone] = component_translation
	return component_rotation, component_translation


for bone_name in ref_local_rotation_by_bone:
	resolve_component_transform(bone_name)

result = {
	"retargeter": RETARGETER_PATH,
	"sourceMesh": SOURCE_MESH_PATH,
	"targetMesh": TARGET_MESH_PATH,
	"rootBone": "refSkeleton",
	"sourceBones": {},
	"bones": {},
	"segmentDotsAgainstReference": {},
	"segmentDotsAgainstSource": {},
}

for bone, translation in component_translation_by_bone.items():
	result["bones"][bone] = {
		"componentTranslation": {"x": translation[0], "y": translation[1], "z": translation[2]},
		"componentRotation": {
			"x": component_rotation_by_bone[bone][0],
			"y": component_rotation_by_bone[bone][1],
			"z": component_rotation_by_bone[bone][2],
			"w": component_rotation_by_bone[bone][3],
		},
	}

source_component_translation_by_bone = {}
source_component_rotation_by_bone = {}
for entry in source_pose_json["bones"]:
	bone = entry["bone"]
	source_component_translation_by_bone[bone] = vec_from_json(entry["refComponent"]["translation"])
	source_component_rotation_by_bone[bone] = quat_from_json(entry["refComponent"]["rotation"])

requested_source_bones = {bone for _, start_bone, end_bone in SOURCE_ARM_CHAINS for bone in (start_bone, end_bone)}
requested_source_bones.update(SOURCE_HAND_BASIS_BONES)
for bone in sorted(requested_source_bones):
	if bone not in source_component_translation_by_bone:
		continue
	translation = source_component_translation_by_bone[bone]
	rotation = source_component_rotation_by_bone[bone]
	result["sourceBones"][bone] = {
		"componentTranslation": {"x": translation[0], "y": translation[1], "z": translation[2]},
		"componentRotation": {"x": rotation[0], "y": rotation[1], "z": rotation[2], "w": rotation[3]},
	}

for metric_name, start_bone, end_bone in ARM_CHAINS:
	posed_direction = vec_normalize(vec_sub(component_translation_by_bone[end_bone], component_translation_by_bone[start_bone]))
	reference_direction = vec_normalize(
		vec_sub(reference_component_translation_by_bone[end_bone], reference_component_translation_by_bone[start_bone])
	)
	result["segmentDotsAgainstReference"][metric_name] = vec_dot(posed_direction, reference_direction)

for metric_name, start_bone, end_bone in SOURCE_ARM_CHAINS:
	if start_bone not in source_component_translation_by_bone or end_bone not in source_component_translation_by_bone:
		continue
	source_direction = vec_normalize(vec_sub(source_component_translation_by_bone[end_bone], source_component_translation_by_bone[start_bone]))
	result["segmentDotsAgainstSource"][metric_name] = vec_dot(
		vec_normalize(vec_sub(component_translation_by_bone[ARM_CHAINS[[name for name, _, _ in ARM_CHAINS].index(metric_name)][2]], component_translation_by_bone[ARM_CHAINS[[name for name, _, _ in ARM_CHAINS].index(metric_name)][1]])),
		source_direction,
	)
OUTPUT_PATH.write_text(json.dumps(result, indent=2), encoding="utf-8")
unreal.log_warning(f"wrote pose preview diagnostic: {OUTPUT_PATH}")
