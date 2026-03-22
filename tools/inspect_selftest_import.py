import json

import unreal


def format_vec(vec):
    return f"({vec.x:.3f}, {vec.y:.3f}, {vec.z:.3f})"


def print_header(title):
    print(f"=== {title} ===")


def parse_json(json_text):
    payload = json.loads(json_text)
    if "error" in payload:
        raise RuntimeError(payload["error"])
    return payload


def format_vec_json(vec):
    return f"({vec['x']:.3f}, {vec['y']:.3f}, {vec['z']:.3f})"


mesh_path = "/Game/SekiroAssets/SelfTest/c0000/Mesh/c0000"
skeleton_path = "/Game/SekiroAssets/SelfTest/c0000/Mesh/c0000_Skeleton"
anim_path = "/Game/SekiroAssets/SelfTest/c0000/Animations/a000_201030"

mesh = unreal.load_asset(mesh_path)
skeleton = unreal.load_asset(skeleton_path)
anim = unreal.load_asset(anim_path)

print_header("Loaded Assets")
print(f"mesh: {mesh}")
print(f"skeleton: {skeleton}")
print(f"anim: {anim}")

if not mesh or not skeleton:
    raise RuntimeError("required self-test assets were not found")

print_header("Skeleton Reference Pose")
reference_pose_dump = parse_json(unreal.SekiroRetargetDiagnosticsLibrary.dump_skeletal_mesh_reference_pose_to_json(mesh, []))
bone_entries = reference_pose_dump["bones"]
bone_names = [entry["bone"] for entry in bone_entries]
ref_pose_by_name = {entry["bone"]: entry["refLocal"] for entry in bone_entries}
world_pose = {entry["bone"]: entry["refComponent"] for entry in bone_entries}
parent_by_name = {entry["bone"]: (entry["parentBone"] or None) for entry in bone_entries}
print(f"bone_count={len(bone_names)}")

for required_bone in ["Master", "Pelvis", "Spine", "Spine1", "Spine2", "Neck", "Head", "L_Hand", "R_Hand", "face_root"]:
    print(f"has_{required_bone}={required_bone in bone_names}")

print_header("Hierarchy Checks")
expected_parents = {
    "face_root": "Head",
    "L_Clavicle": "Spine2",
    "R_Clavicle": "Spine2",
    "L_Shoulder": "L_Clavicle",
    "R_Shoulder": "R_Clavicle",
    "L_UpperArm": "L_Clavicle",
    "R_UpperArm": "R_Clavicle",
    "L_Forearm": "L_UpperArm",
    "R_Forearm": "R_UpperArm",
    "L_Hand": "L_Forearm",
    "R_Hand": "R_Forearm",
}

for bone_name, expected_parent in expected_parents.items():
    if bone_name in bone_names:
        parent_name = str(parent_by_name[bone_name])
        print(f"parent[{bone_name}]={parent_name}")
        print(f"parent_ok[{bone_name}]={parent_name == expected_parent}")

for bone_name in ["Pelvis", "Head", "L_Hand", "R_Hand", "L_Foot", "R_Foot", "face_root"]:
    if bone_name in bone_names:
        transform = ref_pose_by_name[bone_name]
        print(f"ref_pose[{bone_name}].translation={format_vec_json(transform['translation'])}")
        print(f"ref_pose[{bone_name}].rotation=({transform['rotation']['x']:.6f}, {transform['rotation']['y']:.6f}, {transform['rotation']['z']:.6f}, {transform['rotation']['w']:.6f})")
        print(f"ref_pose[{bone_name}].scale={format_vec_json(transform['scale'])}")
        world_transform = world_pose[bone_name]
        print(f"world_pose[{bone_name}].translation={format_vec_json(world_transform['translation'])}")

if "L_Hand" in world_pose and "R_Hand" in world_pose:
    left_hand = world_pose["L_Hand"]["translation"]
    right_hand = world_pose["R_Hand"]["translation"]
    print(
        "imported_lateral_delta="
        f"({left_hand['x'] - right_hand['x']:.3f}, {left_hand['y'] - right_hand['y']:.3f}, {left_hand['z'] - right_hand['z']:.3f})"
    )

print_header("Mesh Bounds")
bounds = mesh.get_bounds()
print(f"origin={format_vec(bounds.origin)}")
print(f"box_extent={format_vec(bounds.box_extent)}")
print(f"sphere_radius={bounds.sphere_radius:.3f}")

print_header("Mesh-Skeleton Relationship")
mesh_skeleton = mesh.skeleton
print(f"mesh_skeleton={mesh_skeleton.get_path_name() if mesh_skeleton else None}")
print(f"mesh_uses_target_skeleton={mesh_skeleton == skeleton}")

if hasattr(mesh, "get_ref_bases_inv_matrix"):
    try:
        ref_bases = mesh.get_ref_bases_inv_matrix()
        print(f"ref_bases_inv_matrix_count={len(ref_bases)}")
    except Exception as ex:
        print(f"ref_bases_inv_matrix_error={ex}")
else:
    print("ref_bases_inv_matrix_count=method_not_exposed")

if hasattr(mesh, "get_editor_property"):
    try:
        materials = mesh.get_editor_property("materials")
        print(f"material_slot_count={len(materials)}")
    except Exception as ex:
        print(f"material_slot_count_error={ex}")

print_header("Animation Binding")
if anim:
    anim_skeleton = anim.skeleton
    print(f"anim_skeleton={anim_skeleton.get_path_name() if anim_skeleton else None}")
    print(f"anim_uses_target_skeleton={anim_skeleton == skeleton}")
    if hasattr(anim, "number_of_sampled_keys"):
        print(f"sampled_keys={anim.number_of_sampled_keys}")
    if hasattr(anim, "target_frame_rate"):
        print(f"frame_rate={anim.target_frame_rate}")
