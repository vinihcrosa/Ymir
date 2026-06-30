import { useMemo, useRef } from 'react'
import { useFrame, useThree } from '@react-three/fiber'
import * as THREE from 'three'
import type { WaveConditionDTO } from '@ymir/types'

const G = 9.81 // gravity [m/s²], deep-water dispersion ω² = g·k
const NWAVES = 4 // Gerstner components summed per fragment

// Detail patch: a Gerstner-displaced grid that follows the camera in XZ so the
// resolution is always spent where the viewer is. Beyond it a flat far-sea plane
// fills the horizon (swell of a few metres is invisible at that distance anyway).
const PATCH_SIZE = 8000 // metres
const PATCH_SEGS = 400 // 8000/400 = 20 m cells — resolves a ~150 m swell well
const FAR_SIZE = 200000 // metres — horizon filler

// Amplitude split across the NWAVES components (dominant + harmonics). Sums to 1;
// the total crest height is anchored to Hs/2 in deriveWaves so crest-to-trough ≈ Hs.
const AMP_WEIGHTS = [0.5, 0.25, 0.15, 0.1]
// Directional spread of the components around the dominant wave direction [rad].
const DIR_SPREAD = [0, 0.35, -0.5, 0.7]
// Wavelength multipliers relative to the peak-period wavelength.
const LEN_MULT = [1.0, 0.78, 1.3, 0.6]

interface GerstnerWave { dirX: number; dirZ: number; steepness: number; wavelength: number; amplitude: number }

/** Pick the wave keyframe active at sim time `t` (piecewise-constant, step-hold). */
export function waveAt(series: WaveConditionDTO[], t: number): WaveConditionDTO | null {
  if (series.length === 0) return null
  let active = series[0]
  for (const kf of series) if (kf.t <= t) active = kf
  return active
}

/**
 * Expand a single sea state (Hs, Tp, nautical direction) into a small bank of
 * Gerstner components. Wavelength comes from the deep-water dispersion relation
 * λ = g·Tp²/2π; amplitudes are distributed so the summed crest height ≈ Hs/2.
 *
 * dirNaut is the nautical bearing the waves COME FROM (deg clockwise from North).
 * Scene axes: +X = East, -Z = North (see geo3d). The propagation direction is the
 * reverse of the "from" bearing. Flip PROP_SIGN if the visual swell runs backwards
 * against the physics — the WASM sim owns the true motion; this is calibratable.
 */
const PROP_SIGN = -1
export function deriveWaves(wave: WaveConditionDTO | null): GerstnerWave[] {
  if (!wave || wave.Hs <= 0 || wave.Tp <= 0) return []
  const lambda0 = (G * wave.Tp * wave.Tp) / (2 * Math.PI)
  const ampTotal = wave.Hs / 2
  const rad = (wave.dirNaut * Math.PI) / 180
  return Array.from({ length: NWAVES }, (_, i) => {
    const dir = rad + DIR_SPREAD[i]
    const fromEast = Math.sin(dir)
    const fromNorth = Math.cos(dir)
    // "From" bearing → scene vector (East, -North), then reverse for propagation.
    const dirX = PROP_SIGN * fromEast
    const dirZ = PROP_SIGN * -fromNorth
    const len = Math.hypot(dirX, dirZ) || 1
    return {
      dirX: dirX / len,
      dirZ: dirZ / len,
      wavelength: Math.max(2, lambda0 * LEN_MULT[i]),
      amplitude: ampTotal * AMP_WEIGHTS[i],
      steepness: 0.6, // 0..1 crest sharpness; clamped per-wave in the shader
    }
  })
}

const VERT = /* glsl */ `
  uniform float uTime;
  uniform vec2  uOffset;          // world XZ of the patch centre (camera follow)
  uniform vec4  uWaveA[${NWAVES}]; // dirX, dirZ, wavelength, amplitude
  uniform float uSteep[${NWAVES}];
  varying float vHeight;          // displacement height, for foam
  varying vec3  vNormal;
  varying vec3  vWorldPos;

  // Gerstner displacement + analytic normal for the grid point at world XZ = p.
  vec3 gerstner(vec2 p, out vec3 nrm) {
    vec3 disp = vec3(0.0);
    vec3 tangent = vec3(1.0, 0.0, 0.0);
    vec3 binormal = vec3(0.0, 0.0, 1.0);
    for (int i = 0; i < ${NWAVES}; i++) {
      vec2  d = vec2(uWaveA[i].x, uWaveA[i].y);
      float wavelength = uWaveA[i].z;
      float a = uWaveA[i].w;
      float k = 6.2831853 / wavelength;     // wavenumber
      float c = sqrt(9.81 / k);             // deep-water phase speed
      float q = uSteep[i] / (k * a * float(${NWAVES})); // keep crests non-looping
      float f = k * dot(d, p) - c * k * uTime;
      float s = sin(f);
      float co = cos(f);
      disp.x += q * a * d.x * co;
      disp.z += q * a * d.y * co;
      disp.y += a * s;
      float wa = k * a;
      tangent  += vec3(-q * d.x * d.x * wa * s, d.x * wa * co, -q * d.x * d.y * wa * s);
      binormal += vec3(-q * d.x * d.y * wa * s, d.y * wa * co, -q * d.y * d.y * wa * s);
    }
    nrm = normalize(cross(binormal, tangent));
    return disp;
  }

  void main() {
    vec2 world = position.xz + uOffset;     // geometry pre-rotated into the XZ plane
    vec3 nrm;
    vec3 disp = gerstner(world, nrm);
    vec3 pos = vec3(position.x + disp.x, disp.y, position.z + disp.z);
    vHeight = disp.y;
    vNormal = nrm;
    vWorldPos = vec3(world.x + disp.x, disp.y, world.y + disp.z);
    gl_Position = projectionMatrix * modelViewMatrix * vec4(pos, 1.0);
  }
`

const FRAG = /* glsl */ `
  uniform vec3  uDeep;
  uniform vec3  uShallow;
  uniform vec3  uSky;
  uniform vec3  uSunDir;
  uniform vec3  uCamPos;
  uniform float uAmp;     // crest amplitude scale (Hs/2), for foam thresholding
  varying float vHeight;
  varying vec3  vNormal;
  varying vec3  vWorldPos;

  void main() {
    vec3 n = normalize(vNormal);
    vec3 viewDir = normalize(uCamPos - vWorldPos);
    // Fresnel: grazing angles reflect the sky, steep angles show deep water.
    float fres = pow(1.0 - max(dot(n, viewDir), 0.0), 3.0);
    float facing = clamp(dot(n, normalize(uSunDir)) * 0.5 + 0.5, 0.0, 1.0);
    vec3 water = mix(uDeep, uShallow, facing);
    vec3 col = mix(water, uSky, clamp(fres, 0.0, 1.0));
    // Specular sun glint.
    vec3 h = normalize(viewDir + normalize(uSunDir));
    float spec = pow(max(dot(n, h), 0.0), 120.0);
    col += vec3(1.0, 0.97, 0.85) * spec * 0.6;
    // Foam on the upper crests (only meaningful once there is real swell).
    if (uAmp > 0.05) {
      float foam = smoothstep(uAmp * 0.55, uAmp * 0.95, vHeight);
      col = mix(col, vec3(0.92, 0.95, 0.98), foam * 0.7);
    }
    gl_FragColor = vec4(col, 1.0);
  }
`

/**
 * Animated Gerstner ocean. Geometry is pre-rotated into the XZ plane so the shader
 * can displace along world-up (Y); the patch follows the camera (snapped to a cell
 * so the grid doesn't swim) and the wave phase is keyed off the simulation clock.
 */
export function Ocean({ waves }: { waves: GerstnerWave[] }) {
  const meshRef = useRef<THREE.Mesh>(null)
  const { camera } = useThree()

  // Plane lives in XY by default; bake a -90° X rotation into the buffer so the
  // grid sits in XZ with position.y ≈ 0 and the shader owns vertical displacement.
  const geometry = useMemo(() => {
    const g = new THREE.PlaneGeometry(PATCH_SIZE, PATCH_SIZE, PATCH_SEGS, PATCH_SEGS)
    g.rotateX(-Math.PI / 2)
    return g
  }, [])

  const material = useMemo(() => new THREE.ShaderMaterial({
    vertexShader: VERT,
    fragmentShader: FRAG,
    uniforms: {
      uTime: { value: 0 },
      uOffset: { value: new THREE.Vector2(0, 0) },
      uWaveA: { value: Array.from({ length: NWAVES }, () => new THREE.Vector4()) },
      uSteep: { value: new Float32Array(NWAVES) },
      uAmp: { value: 0 },
      uDeep: { value: new THREE.Color('#0a2740') },
      uShallow: { value: new THREE.Color('#15577a') },
      uSky: { value: new THREE.Color('#bcd8ff') },
      uSunDir: { value: new THREE.Vector3(5000, 8000, 4000).normalize() },
      uCamPos: { value: new THREE.Vector3() },
    },
  }), [])

  // Push the current sea state into the shader whenever it changes.
  useMemo(() => {
    const u = material.uniforms
    const arr = u.uWaveA.value as THREE.Vector4[]
    const steep = u.uSteep.value as Float32Array
    let crest = 0
    for (let i = 0; i < NWAVES; i++) {
      const w = waves[i]
      if (w) {
        arr[i].set(w.dirX, w.dirZ, w.wavelength, w.amplitude)
        steep[i] = w.steepness
        crest += w.amplitude
      } else {
        arr[i].set(0, 0, 1, 0)
        steep[i] = 0
      }
    }
    u.uAmp.value = crest
  }, [material, waves])

  useFrame((s) => {
    const mesh = meshRef.current
    if (!mesh) return
    // Snap the patch to its grid so vertices keep landing on the same world cells
    // as the camera moves — otherwise the surface visibly swims under the viewer.
    const cell = PATCH_SIZE / PATCH_SEGS
    const ox = Math.round(camera.position.x / cell) * cell
    const oz = Math.round(camera.position.z / cell) * cell
    mesh.position.set(ox, 0, oz)
    const u = material.uniforms
    ;(u.uOffset.value as THREE.Vector2).set(ox, oz)
    ;(u.uCamPos.value as THREE.Vector3).copy(camera.position)
    // Animate off the render clock — smooth regardless of sim play/pause state.
    // Sea state (Hs/Tp/dir) is what's synced to the sim; phase is visual-only.
    u.uTime.value = s.clock.elapsedTime
  })

  return (
    <>
      {/* Far flat sea for the horizon; just below 0 to avoid z-fighting the patch. */}
      <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, -0.2, 0]}>
        <planeGeometry args={[FAR_SIZE, FAR_SIZE]} />
        <meshStandardMaterial color="#0a2740" metalness={0.1} roughness={0.6} />
      </mesh>
      <mesh ref={meshRef} geometry={geometry} material={material} />
    </>
  )
}
