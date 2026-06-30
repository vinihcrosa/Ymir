import { Component, Suspense, useMemo, useEffect, type ReactNode } from 'react'
import { Canvas, useThree } from '@react-three/fiber'
import { useGLTF, Clone, FlyControls } from '@react-three/drei'
import * as THREE from 'three'
import { useScenarioStore } from '../store'
import { useSimulationStore } from '../../../stores/simulationStore'
import { useVesselPanelStore } from '../../../stores/vesselPanelStore'
import { useViewStore, type CameraId } from '../../../stores/viewStore'
import { useEnvironmentStore } from '../../../stores/environmentStore'
import { AREA_ROT_X, simToScene, headingToSceneYaw } from '../../../lib/geo3d'
import { tokens } from '../../../theme/tokens'
import { Ocean, deriveWaves, waveAt } from './Ocean'

const AREA_BASE = '/assets/3d/baia_de_guanabara'
const VESSEL_URL = '/assets/3d/vessel/celso_furtado.glb'
const AREA_PARTS = [`${AREA_BASE}/batimetria.glb`, `${AREA_BASE}/ilhas.glb`, `${AREA_BASE}/ponte.glb`]

// The celso_furtado mesh origin is at the KEEL (world Y 0→46.3 = keel→masthead),
// and its design waterline (vessel_config waterInteraction points) is ~4.45 m above
// the keel. Sink the model by that so the waterline sits on the sea plane (Y=0).
const VESSEL_WATERLINE = 4.45

// Onboard cameras from vessel_config (celso_furtado): local offset (metres) +
// local yaw (deg) in the vessel frame.
const CAMERAS: Record<Exclude<CameraId, 'Free'>, { x: number; y: number; z: number; yawDeg: number }> = {
  Front: { x: 0, y: 31.5, z: -58.8, yawDeg: 0 },
  Back: { x: 0, y: 31.5, z: -80.94, yawDeg: 180 },
  Bridge: { x: 0, y: 31.5, z: -61.58, yawDeg: 0 },
  Starboard: { x: 16.32, y: 31.5, z: -61.3, yawDeg: 0 },
  Portside: { x: -16.32, y: 31.5, z: -61.3, yawDeg: 0 },
}

/** Isolates a GLTF-loading subtree so a missing/4xx asset renders nothing instead
 *  of throwing up the tree and blanking the whole Canvas (ocean, lights, etc.).
 *  The .glb assets are gitignored (see .gitignore) and may be absent in some envs. */
class SceneErrorBoundary extends Component<{ children: ReactNode; label: string }, { failed: boolean }> {
  state = { failed: false }
  static getDerivedStateFromError() { return { failed: true } }
  componentDidCatch(err: unknown) {
    console.warn(`[Area3DView] ${this.props.label} failed to load — skipping`, err)
  }
  render() { return this.state.failed ? null : this.props.children }
}

/** The Baía meshes are Z-up; rotate to Y-up and recentre horizontally on the sim
 *  origin so vessels (near 0,0) sit on the bay. */
function AreaScene() {
  const parts = AREA_PARTS.map((u) => useGLTF(u))

  // The meshes are modelled about the area's reference origin (= sim 0,0) with Z
  // up and the water surface at Z=0; the -90° X rotation brings that to Y-up with
  // the waterline at Y=0. No bbox recentre — a stray vertex in ilhas.glb makes the
  // combined box bogus, and the native origin already matches the sim frame.
  return (
    <group rotation={[AREA_ROT_X, 0, 0]}>
      {parts.map((p, i) => (
        <primitive key={i} object={p.scene} />
      ))}
    </group>
  )
}

function VesselModel({ x, y, headingDeg, roll, pitch, heave }: {
  x: number; y: number; headingDeg: number; roll: number; pitch: number; heave: number
}) {
  const { scene } = useGLTF(VESSEL_URL)
  // Heave bobs the hull on the waterline (sim z is down-positive → up = -z).
  const [px, py, pz] = simToScene(x, y, -VESSEL_WATERLINE - heave)
  const yaw = headingToSceneYaw((headingDeg * Math.PI) / 180)
  return (
    // Outer: position + heading (yaw about scene up). Inner: attitude in the
    // heading-aligned frame — pitch about the beam axis (X), roll about the
    // longitudinal axis (Z). Signs are calibratable.
    <group position={[px, py, pz]} rotation={[0, yaw, 0]}>
      <group rotation={[pitch, 0, roll]}>
        <Clone object={scene} />
      </group>
    </group>
  )
}

/** Camera anchored to the selected vessel's chosen onboard camera; falls back to a
 *  bay overview when nothing is selected. */
function CameraRig({ target, cameraId }: { target: { x: number; y: number; headingDeg: number } | null; cameraId: CameraId }) {
  const { camera } = useThree()
  useEffect(() => {
    if (!target) {
      camera.position.set(0, 1200, 1800)
      camera.lookAt(0, 0, 0)
      return
    }
    const cam = CAMERAS[cameraId as Exclude<CameraId, 'Free'>]
    if (!cam) return
    const vesselYaw = headingToSceneYaw((target.headingDeg * Math.PI) / 180)
    const [bx, by, bz] = simToScene(target.x, target.y)
    const up = new THREE.Vector3(0, 1, 0)
    // Camera position: local offset rotated by the vessel heading.
    const off = new THREE.Vector3(cam.x, cam.y, cam.z).applyAxisAngle(up, vesselYaw)
    camera.position.set(bx + off.x, by + off.y, bz + off.z)
    // Look direction: vessel heading + this camera's own local yaw, aimed slightly
    // down toward the water so the sea + horizon are visible.
    const aimYaw = vesselYaw + (cam.yawDeg * Math.PI) / 180
    const ahead = new THREE.Vector3(0, -250, -1500).applyAxisAngle(up, aimYaw)
    camera.lookAt(bx + off.x + ahead.x, by + off.y + ahead.y, bz + off.z + ahead.z)
  }, [camera, target, cameraId])
  return null
}

/** Free-fly camera: WASD to move, R/F up-down, drag the mouse to look. Positioned
 *  once near the target on entry, then driven entirely by the user. */
function FreeCamera({ target }: { target: { x: number; y: number; headingDeg: number } | null }) {
  const { camera } = useThree()
  useEffect(() => {
    const [bx, , bz] = target ? simToScene(target.x, target.y) : [0, 0, 0]
    camera.position.set(bx, 600, bz + 900)
    camera.lookAt(bx, 0, bz)
    // run once on entering free mode
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [])
  return <FlyControls movementSpeed={300} rollSpeed={0.4} dragToLook makeDefault />
}

export function Area3DView() {
  const { vessels } = useScenarioStore()
  const { status, state } = useSimulationStore()
  const { selectedVesselId } = useVesselPanelStore()
  const running = status === 'running' || status === 'paused'

  // Resolve each vessel's current pose (live state while running, draft otherwise).
  const posed = useMemo(() => vessels.map((v) => {
    const live = running ? state?.vessels.find((s) => s.id === v.instanceId) : null
    return live
      ? { instanceId: v.instanceId, x: live.x, y: live.y, headingDeg: (live.psi * 180) / Math.PI, roll: live.phi, pitch: live.theta, heave: live.z }
      : { instanceId: v.instanceId, x: v.x, y: v.y, headingDeg: v.headingDeg, roll: 0, pitch: 0, heave: 0 }
  }), [vessels, running, state])

  const target = posed.find((p) => p.instanceId === selectedVesselId) ?? posed[0] ?? null
  const cameraId = useViewStore((s) => s.cameraId)

  // Sea state for the ocean shader: the wave keyframe active at the current sim
  // time, expanded into Gerstner components. Re-derived only when it changes.
  const waveSeries = useEnvironmentStore((s) => s.waveSeries)
  const simTime = running ? state?.t ?? 0 : 0
  const waves = useMemo(() => deriveWaves(waveAt(waveSeries, simTime)), [waveSeries, simTime])

  return (
    <Canvas
      camera={{ fov: 55, near: 1, far: 100000, position: [0, 1200, 1800] }}
      style={{ width: '100%', height: '100%', background: '#0b1f33' }}
      data-testid="area-3d-view"
    >
      <ambientLight intensity={0.6} />
      <hemisphereLight args={['#bcd8ff', '#24435f', 1.2]} />
      <directionalLight position={[5000, 8000, 4000]} intensity={2.2} />
      <Suspense fallback={null}>
        <SceneErrorBoundary label="area mesh">
          <AreaScene />
        </SceneErrorBoundary>
        {posed.map((p) => (
          <SceneErrorBoundary key={p.instanceId} label={`vessel ${p.instanceId}`}>
            <VesselModel x={p.x} y={p.y} headingDeg={p.headingDeg} roll={p.roll} pitch={p.pitch} heave={p.heave} />
          </SceneErrorBoundary>
        ))}
      </Suspense>
      {/* Animated Gerstner sea at the waterline (Y=0), synced to the sim sea state. */}
      <Ocean waves={waves} />
      {cameraId === 'Free'
        ? <FreeCamera target={target} />
        : <CameraRig target={target} cameraId={cameraId} />}
    </Canvas>
  )
}

AREA_PARTS.forEach((u) => useGLTF.preload(u))
useGLTF.preload(VESSEL_URL)
