const fs = require('fs');
const path = require('path');

const gltfPath = process.argv[2] || 'E:/Sekiro/DSAnimStudio/_ExportCheck/UserVerifyRun/Export_17_Preview/Model/c0000.gltf';
const gltf = JSON.parse(fs.readFileSync(gltfPath, 'utf8'));
const baseDir = path.dirname(gltfPath);

function loadBuffer(bufferIndex) {
  const uri = gltf.buffers[bufferIndex].uri;
  return fs.readFileSync(path.join(baseDir, uri));
}

const buffers = gltf.buffers.map((_, idx) => loadBuffer(idx));

function numComponents(type) {
  switch (type) {
    case 'SCALAR': return 1;
    case 'VEC2': return 2;
    case 'VEC3': return 3;
    case 'VEC4': return 4;
    case 'MAT4': return 16;
    default: throw new Error(`Unsupported accessor type: ${type}`);
  }
}

function componentSize(componentType) {
  switch (componentType) {
    case 5121: return 1; // UNSIGNED_BYTE
    case 5123: return 2; // UNSIGNED_SHORT
    case 5126: return 4; // FLOAT
    default: throw new Error(`Unsupported component type: ${componentType}`);
  }
}

function readComponent(viewBuffer, offset, componentType) {
  switch (componentType) {
    case 5121: return viewBuffer.readUInt8(offset);
    case 5123: return viewBuffer.readUInt16LE(offset);
    case 5126: return viewBuffer.readFloatLE(offset);
    default: throw new Error(`Unsupported component type: ${componentType}`);
  }
}

function readAccessor(accessorIndex) {
  const accessor = gltf.accessors[accessorIndex];
  const bufferView = gltf.bufferViews[accessor.bufferView];
  const buffer = buffers[bufferView.buffer];
  const count = accessor.count;
  const comps = numComponents(accessor.type);
  const stride = bufferView.byteStride || (componentSize(accessor.componentType) * comps);
  const byteOffset = (bufferView.byteOffset || 0) + (accessor.byteOffset || 0);
  const values = [];
  for (let i = 0; i < count; i++) {
    const entry = [];
    const base = byteOffset + i * stride;
    for (let c = 0; c < comps; c++) {
      entry.push(readComponent(buffer, base + c * componentSize(accessor.componentType), accessor.componentType));
    }
    values.push(entry);
  }
  return values;
}

const materialIndexByName = new Map((gltf.materials || []).map((mat, idx) => [mat.name || `material_${idx}`, idx]));
const artificialArmMaterialIndex = materialIndexByName.get('artificialarm');
if (artificialArmMaterialIndex === undefined) {
  console.error('Material artificialarm not found');
  process.exit(1);
}

const skin = gltf.skins[0];
const jointNames = skin.joints.map((nodeIndex) => gltf.nodes[nodeIndex].name || `node_${nodeIndex}`);

const summaries = [];
for (const mesh of gltf.meshes) {
  for (const primitive of mesh.primitives) {
    if (primitive.material !== artificialArmMaterialIndex) {
      continue;
    }

    const joints = readAccessor(primitive.attributes.JOINTS_0);
    const weights = readAccessor(primitive.attributes.WEIGHTS_0);
    const influenceByBone = new Map();
    let totalWeight = 0;

    for (let i = 0; i < joints.length; i++) {
      for (let j = 0; j < 4; j++) {
        const jointIndex = joints[i][j];
        const weight = weights[i][j];
        if (weight <= 0) continue;
        const boneName = jointNames[jointIndex] || `joint_${jointIndex}`;
        influenceByBone.set(boneName, (influenceByBone.get(boneName) || 0) + weight);
        totalWeight += weight;
      }
    }

    const topInfluences = [...influenceByBone.entries()]
      .sort((a, b) => b[1] - a[1])
      .slice(0, 12)
      .map(([bone, weight]) => ({ bone, normalizedWeight: +(weight / totalWeight).toFixed(4) }));

    summaries.push({
      meshName: mesh.name || '<unnamed_mesh>',
      topInfluences
    });
  }
}

console.log(JSON.stringify({ artificialArmMaterialIndex, summaries }, null, 2));