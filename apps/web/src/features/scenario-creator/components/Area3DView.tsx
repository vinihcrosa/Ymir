import { Suspense, useMemo, useRef, useEffect } from 'react'
import { Canvas, useThree } from '@react-three/fiber'
import { useGLTF, Clone } from '@react-three/drei'
import * as THREE from 'three'
import { useScenarioStore } from '../store'
import { useSimulationStore } from '../../../stores/simulationStore'
import { useVesselPanelStore } from '../../../stores/vesselPanelStore'
import { useViewStore, type CameraId } from '../../../stores/viewStore'
import { AREA_ROT_X, simToScene, headingToSceneYaw } from '../../../lib/geo3d'
import { tokens } from '../../../theme/tokens'

const AREA_BASE = '/assets/3d/baia_de_guanabara'
const VESSEL_URL = '/assets/3d/vessel/celso_furtado.glb'
const AREA_PARTS = [`${AREA_BASE}/batimetria.glb`, `${AREA_BASE}/ilhas.glb`, `${AREA_BASE}/ponte.glb`]

// Onboard cameras from vessel_config (celso_furtado): local offset (metres) +
// local yaw (deg) in the vessel frame.
const CAMERAS: Record<CameraId, { x: number; y: number; z: number; yawDeg: number }> = {
  Front: { x: 0, y: 31.5, z: -58.8, yawDeg: 0 },
  Back: { x: 0, y: 31.5, z: -80.94, yawDeg: 180 },
  Bridge: { x: 0, y: 31.5, z: -61.58, yawDeg: 0 },
  Starboard: { x: 16.32, y: 31.5, z: -61.3, yawDeg: 0 },
  Portside: { x: -16.32, y: 31.5, z: -61.3, yawDeg: 0 },
}

/** The Baía meshes are Z-up; rotate to Y-up and recentre horizontally on the sim
 *  origin so vessels (near 0,0) sit on the bay. */
function AreaScene() {
  const group = useRef<THREE.Group>(null)
  const parts = AREA_PARTS.map((u) => useGLTF(u))

  useEffect(() => {
    const g = group.current
    if (!g) return
    // Recentre on the horizontal bounding-box centre (keep vertical as-is).
    const box = new THREE.Box3().setFromObject(g)
    const c = box.getCenter(new THREE.Vector3())
    g.position.x -= c.x
    g.position.z -= c.z
  }, [])

  return (
    <group ref={group} rotation={[AREA_ROT_X, 0, 0]}>
      {parts.map((p, i) => (
        <primitive key={i} object={p.scene} />
      ))}
    </group>
  )
}

function VesselModel({ x, y, headingDeg }: { x: number; y: number; headingDeg: number }) {
  const { scene } = useGLTF(VESSEL_URL)
  const [px, py, pz] = simToScene(x, y)
  return (
    <group position={[px, py, pz]} rotation={[0, headingToSceneYaw((headingDeg * Math.PI) / 180), 0]}>
      <Clone object={scene} />
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
    const cam = CAMERAS[cameraId]
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

export function Area3DView() {
  const { vessels } = useScenarioStore()
  const { status, state } = useSimulationStore()
  const { selectedVesselId } = useVesselPanelStore()
  const running = status === 'running' || status === 'paused'

  // Resolve each vessel's current pose (live state while running, draft otherwise).
  const posed = useMemo(() => vessels.map((v) => {
    const live = running ? state?.vessels.find((s) => s.id === v.instanceId) : null
    return live
      ? { instanceId: v.instanceId, x: live.x, y: live.y, headingDeg: (live.psi * 180) / Math.PI }
      : { instanceId: v.instanceId, x: v.x, y: v.y, headingDeg: v.headingDeg }
  }), [vessels, running, state])

  const target = posed.find((p) => p.instanceId === selectedVesselId) ?? posed[0] ?? null
  const cameraId = useViewStore((s) => s.cameraId)

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
        <AreaScene />
        {posed.map((p) => (
          <VesselModel key={p.instanceId} x={p.x} y={p.y} headingDeg={p.headingDeg} />
        ))}
      </Suspense>
      {/* Water plane at sea level. */}
      <mesh rotation={[-Math.PI / 2, 0, 0]} position={[0, 0, 0]}>
        <planeGeometry args={[80000, 80000]} />
        <meshStandardMaterial color={tokens.color.accent} transparent opacity={0.55} />
      </mesh>
      <CameraRig target={target} cameraId={cameraId} />
    </Canvas>
  )
}

AREA_PARTS.forEach((u) => useGLTF.preload(u))
useGLTF.preload(VESSEL_URL)
