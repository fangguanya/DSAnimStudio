import unreal


def format_vec(vec):
    return f"({vec.x:.3f}, {vec.y:.3f}, {vec.z:.3f})"


def print_header(title):
    print(f"=== {title} ===")


def build_world_transforms(ref_skel, ref_pose, bone_names):
    world_by_name = {}
    for bone_name in bone_names:
        transform = ref_pose.get_ref_bone_pose(bone_name)
        parent_name = ref_skel.get_parent_name(bone_name)
        if parent_name and str(parent_name) != "None":
            parent_transform = world_by_name[parent_name]
            world_by_name[bone_name] = unreal.MathLibrary.compose_transforms(transform, parent_transform)
        else:
            world_by_name[bone_name] = transform
    return world_by_name


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
ref_pose = skeleton.get_reference_pose()
bone_names = list(ref_pose.get_bone_names())
print(f"bone_count={len(bone_names)}")
world_pose = build_world_transforms(ref_pose, ref_pose, bone_names)

for required_bone in ["Master", "Pelvis", "Spine", "Spine1", "Spine2", "Neck", "Head", "L_Hand", "R_Hand", "face_root"]:
    print(f"has_{required_bone}={required_bone in bone_names}")

for bone_name in ["Pelvis", "Head", "L_Hand", "R_Hand", "L_Foot", "R_Foot", "face_root"]:
    if bone_name in bone_names:
        transform = ref_pose.get_ref_bone_pose(bone_name)
        print(f"ref_pose[{bone_name}].translation={format_vec(transform.translation)}")
        print(f"ref_pose[{bone_name}].rotation=({transform.rotation.x:.6f}, {transform.rotation.y:.6f}, {transform.rotation.z:.6f}, {transform.rotation.w:.6f})")
        print(f"ref_pose[{bone_name}].scale={format_vec(transform.scale3d)}")
        world_transform = world_pose[bone_name]
        print(f"world_pose[{bone_name}].translation={format_vec(world_transform.translation)}")

if "L_Hand" in world_pose and "R_Hand" in world_pose:
    left_hand = world_pose["L_Hand"].translation
    right_hand = world_pose["R_Hand"].translation
    lateral_delta = unreal.Vector(left_hand.x - right_hand.x, left_hand.y - right_hand.y, left_hand.z - right_hand.z)
    print(f"imported_lateral_delta={format_vec(lateral_delta)}")

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
