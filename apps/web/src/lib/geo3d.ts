/**
 * Maps the simulation's local frame (metres, x=East, y=North, ψ=heading as a math
 * angle: 0=East, CCW positive) into the Three.js scene (Y-up, right-handed).
 *
 * The Baía de Guanabara meshes are exported Z-up (the thin vertical axis is Z),
 * so the area group is rotated -90° about X to bring Z→Y. After that rotation the
 * horizontal plane is X (East) / Z, with North pointing along -Z. Hence:
 *   sceneX =  x(East)        sceneZ = -y(North)        sceneY = up
 *
 * Sign/offset constants are isolated here so the scene can be calibrated against a
 * known landmark (e.g. the Rio-Niterói bridge) without touching component code.
 */
export const AREA_ROT_X = -Math.PI / 2 // rotate area meshes from Z-up to Y-up

/** Convert sim ground coordinates (metres) to a Three.js position on the water plane. */
export function simToScene(x: number, y: number, height = 0): [number, number, number] {
  return [x, height, 0 - y] // `0 - y` avoids a negative-zero on the North axis
}

/**
 * Convert a sim heading ψ (radians, 0=East, CCW) to a Three.js Y-axis rotation.
 * In the scene, +X is East and -Z is North; a yaw of 0 should point the model East.
 * Three's Y rotation is CCW looking down the +Y axis, which corresponds to the
 * East→North sense after the Z flip, so the heading maps directly.
 */
export function headingToSceneYaw(psiRad: number): number {
  return psiRad
}
