import * as THREE from 'https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js';
import { OrbitControls } from 'https://cdn.jsdelivr.net/npm/three@0.160.0/examples/jsm/controls/OrbitControls.js';
import createTinyGLTF from './tinygltf_v3.js';

const statusEl = document.getElementById('status');
const dropZone = document.getElementById('drop-zone');
const viewerEl = document.getElementById('viewer');
const fileInput = document.getElementById('file-input');
const sampleBtn = document.getElementById('sample-btn');

function setStatus(msg) {
  statusEl.textContent = msg;
  statusEl.title = msg;
}

let Module = null;
let renderer = null;
let scene = null;
let camera = null;
let controls = null;

/* ---------------------------------------------------------------- */
/* Scene construction                                                */
/* ---------------------------------------------------------------- */

// tg3w_prim layout in bytes: material, mode, vertex_count, index_count,
// pos_offset, nrm_offset, uv_offset, idx_offset (all 4-byte ints).
function readPrim(i) {
  const p = Module._tg3w_prim(i);
  const get = (off) => Module.getValue(p + off, 'i32');
  return {
    material: get(0),
    mode: get(4),
    vertexCount: get(8),
    indexCount: get(12),
    posOffset: get(16),
    nrmOffset: get(20),
    uvOffset: get(24),
    idxOffset: get(28),
  };
}

function copyF32(offsetFloats, count) {
  if (!offsetFloats || !count) return null;
  return Module.HEAPF32.subarray(offsetFloats, offsetFloats + count).slice();
}

function makeTexture(texIdx) {
  const src = Module._tg3w_texture_source(texIdx);
  if (src < 0) return null;
  let blobPromise = null;

  const bv = Module._tg3w_image_buffer_view(src);
  if (bv >= 0) {
    const ptr = Module._tg3w_image_bytes(src);
    const size = Module._tg3w_image_size(src);
    if (!ptr || !size) return null;
    const bytes = Module.HEAPU8.slice(ptr, ptr + size);
    const mime = Module.UTF8ToString(Module._tg3w_image_mime(src)) || 'image/png';
    blobPromise = Promise.resolve(new Blob([bytes], { type: mime }));
  } else {
    const uriPtr = Module._tg3w_image_uri(src);
    if (!uriPtr) return null;
    const uri = Module.UTF8ToString(uriPtr);
    const m = /^data:([^;,]+)(;base64)?,(.*)$/s.exec(uri);
    if (!m) return null; // external file URI — not resolvable without FS
    const bytes = m[2]
      ? Uint8Array.from(atob(m[3]), (c) => c.charCodeAt(0))
      : new TextEncoder().encode(decodeURIComponent(m[3]));
    blobPromise = Promise.resolve(new Blob([bytes], { type: m[1] }));
  }

  return blobPromise.then((blob) => createImageBitmap(blob)).then((bmp) => {
    const tex = new THREE.CanvasTexture(bmp);
    tex.colorSpace = THREE.SRGBColorSpace;
    return tex;
  });
}

function makeMaterial(mi) {
  const color = copyF32(Module._tg3w_material_base_color(mi) / 4, 4);
  const metalness = Module._tg3w_material_metallic(mi);
  const roughness = Module._tg3w_material_roughness(mi);
  const alphaMode = Module._tg3w_material_alpha_mode(mi);
  const alphaCutoff = Module._tg3w_material_alpha_cutoff(mi);
  const doubleSided = Module._tg3w_material_double_sided(mi);
  const mat = new THREE.MeshStandardMaterial({
    color: new THREE.Color(color[0], color[1], color[2]),
    metalness,
    roughness,
    transparent: alphaMode === 2,
    alphaTest: alphaMode === 1 ? alphaCutoff : 0,
    side: doubleSided ? THREE.DoubleSide : THREE.FrontSide,
  });
  const texIdx = Module._tg3w_material_base_color_texture(mi);
  if (texIdx >= 0) {
    return makeTexture(texIdx).then((tex) => {
      if (tex) mat.map = tex;
      return mat;
    });
  }
  return Promise.resolve(mat);
}

// Convert triangle strip/fan index buffers to plain triangles.
function triangulate(indices, mode) {
  if (mode === 4) return indices;
  const out = [];
  const n = indices.length;
  if (mode === 5) { // TRIANGLE_STRIP
    for (let i = 2; i < n; i++) {
      if (i % 2 === 0) out.push(indices[i - 2], indices[i - 1], indices[i]);
      else out.push(indices[i - 1], indices[i - 2], indices[i]);
    }
  } else if (mode === 6) { // TRIANGLE_FAN
    for (let i = 2; i < n; i++) out.push(indices[0], indices[i - 1], indices[i]);
  }
  return out;
}

function makePrimitive(i) {
  const p = readPrim(i);
  const positions = copyF32(p.posOffset, p.vertexCount * 3);
  const geometry = new THREE.BufferGeometry();
  geometry.setAttribute('position', new THREE.BufferAttribute(positions, 3));
  if (p.nrmOffset >= 0) {
    geometry.setAttribute('normal', new THREE.BufferAttribute(copyF32(p.nrmOffset, p.vertexCount * 3), 3));
  }
  if (p.uvOffset >= 0) {
    geometry.setAttribute('uv', new THREE.BufferAttribute(copyF32(p.uvOffset, p.vertexCount * 2), 2));
  }
  if (p.indexCount) {
    const raw = Module.HEAPU32.subarray(p.idxOffset, p.idxOffset + p.indexCount).slice();
    geometry.setIndex(new THREE.BufferAttribute(new Uint32Array(triangulate(raw, p.mode)), 1));
  }

  return makeMaterial(p.material).then((mat) => {
    if (p.mode === 0) return new THREE.Points(geometry, mat);
    if (p.mode === 1 || p.mode === 2 || p.mode === 3) {
      const cls = p.mode === 1 ? THREE.Line : p.mode === 2 ? THREE.LineLoop : THREE.LineSegments;
      return new cls(geometry, new THREE.LineBasicMaterial({ color: mat.color }));
    }
    return new THREE.Mesh(geometry, mat);
  });
}

function buildNodeGraph(nodeIndices, parent) {
  const jobs = [];
  const visit = (nodeIdx, parentGroup) => {
    const trs = copyF32(Module._tg3w_node_trs(nodeIdx) / 4, 10);
    const group = new THREE.Group();
    group.position.set(trs[0], trs[1], trs[2]);
    group.quaternion.set(trs[3], trs[4], trs[5], trs[6]);
    group.scale.set(trs[7], trs[8], trs[9]);

    const meshIdx = Module._tg3w_node_mesh(nodeIdx);
    if (meshIdx >= 0) {
      const start = Module._tg3w_mesh_prim_start(meshIdx);
      const count = Module._tg3w_mesh_prim_count(meshIdx);
      for (let k = 0; k < count; k++) {
        jobs.push(makePrimitive(start + k).then((mesh) => {
          mesh.name = `mesh${meshIdx}[${k}]`;
          group.add(mesh);
        }));
      }
    }

    const childCount = Module._tg3w_node_child_count(nodeIdx);
    const childrenPtr = Module._tg3w_node_children(nodeIdx);
    for (let c = 0; c < childCount; c++) {
      const childIdx = Module.getValue(childrenPtr + c * 4, 'i32');
      visit(childIdx, group);
    }
    parentGroup.add(group);
  };
  for (const n of nodeIndices) visit(n, parent);
  return Promise.all(jobs).then(() => parent);
}

async function buildScene() {
  scene = new THREE.Scene();
  scene.background = new THREE.Color(0x14161a);
  scene.add(new THREE.HemisphereLight(0xffffff, 0x223344, 1.0));
  const dirLight = new THREE.DirectionalLight(0xffffff, 1.2);
  dirLight.position.set(5, 8, 6);
  scene.add(dirLight);

  const root = new THREE.Group();
  scene.add(root);

  const sceneCount = Module._tg3w_scene_count();
  let nodes = [];
  const defaultScene = Module._tg3w_default_scene();
  if (defaultScene >= 0 && defaultScene < sceneCount) {
    const n = Module._tg3w_scene_node_count(defaultScene);
    const p = Module._tg3w_scene_nodes(defaultScene);
    for (let i = 0; i < n; i++) nodes.push(Module.getValue(p + i * 4, 'i32'));
  } else {
    for (let i = 0; i < Module._tg3w_node_count(); i++) nodes.push(i);
  }
  await buildNodeGraph(nodes, root);

  // Frame the camera on the geometry.
  const box = new THREE.Box3().setFromObject(root);
  if (!box.isEmpty()) {
    const center = box.getCenter(new THREE.Vector3());
    const size = box.getSize(new THREE.Vector3()).length() || 1;
    camera.position.copy(center).add(new THREE.Vector3(size, size * 0.8, size));
    camera.lookAt(center);
    controls.target.copy(center);
    controls.update();
  }

  viewerEl.hidden = false;
  dropZone.hidden = true;
  setStatus(`Rendered: ${Module._tg3w_prim_count()} primitives, ${Module._tg3w_node_count()} nodes`);
}

/* ---------------------------------------------------------------- */
/* Loading                                                           */
/* ---------------------------------------------------------------- */

async function loadModel(bytes) {
  setStatus('Parsing with tinygltf v3 (C/WASM)…');
  const ptr = Module._malloc(bytes.length);
  Module.HEAPU8.set(bytes, ptr);
  const rc = Module._tg3w_parse(ptr, bytes.length);
  Module._free(ptr);

  if (rc !== 0) {
    const first = Module.UTF8ToString(Module._tg3w_error_message(0));
    const last = Module.UTF8ToString(Module._tg3w_last_error());
    setStatus(`Parse failed (rc=${rc}): ${first || last}`);
    return;
  }
  const warn = Module._tg3w_error_count();
  await buildScene();
  if (warn > 0) {
    setStatus(`Loaded with ${warn} warning(s) — see console`);
    for (let i = 0; i < warn; i++) {
      console.warn(`[tg3] ${Module.UTF8ToString(Module._tg3w_error_message(i))}`);
    }
  }
}

async function handleFile(file) {
  if (!file) return;
  const buf = await file.arrayBuffer();
  await loadModel(new Uint8Array(buf));
}

/* ---------------------------------------------------------------- */
/* Setup                                                             */
/* ---------------------------------------------------------------- */

async function init() {
  renderer = new THREE.WebGLRenderer({ antialias: true });
  renderer.setPixelRatio(window.devicePixelRatio);
  renderer.setSize(viewerEl.clientWidth, viewerEl.clientHeight);
  viewerEl.appendChild(renderer.domElement);

  camera = new THREE.PerspectiveCamera(50, viewerEl.clientWidth / viewerEl.clientHeight, 0.01, 10000);
  controls = new OrbitControls(camera, renderer.domElement);

  window.addEventListener('resize', () => {
    const w = viewerEl.clientWidth;
    const h = viewerEl.clientHeight;
    camera.aspect = w / h;
    camera.updateProjectionMatrix();
    renderer.setSize(w, h);
  });

  renderer.setAnimationLoop(() => {
    if (scene) renderer.render(scene, camera);
  });

  try {
    Module = await createTinyGLTF();
  } catch (e) {
    setStatus(`Failed to load WASM: ${e}`);
    return;
  }
  setStatus('WASM ready — drop a .glb/.gltf file');

  fileInput.addEventListener('change', () => handleFile(fileInput.files[0]));
  sampleBtn.addEventListener('click', async () => {
    try {
      const res = await fetch('Cube.glb');
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      await loadModel(new Uint8Array(await res.arrayBuffer()));
    } catch (e) {
      setStatus(`Sample load failed: ${e}`);
    }
  });
  sampleBtn.hidden = false;

  for (const ev of ['dragenter', 'dragover']) {
    dropZone.addEventListener(ev, (e) => { e.preventDefault(); dropZone.classList.add('dragover'); });
  }
  for (const ev of ['dragleave', 'drop']) {
    dropZone.addEventListener(ev, (e) => { e.preventDefault(); dropZone.classList.remove('dragover'); });
  }
  dropZone.addEventListener('drop', (e) => {
    const f = e.dataTransfer.files && e.dataTransfer.files[0];
    if (f) handleFile(f);
  });
}

init();
