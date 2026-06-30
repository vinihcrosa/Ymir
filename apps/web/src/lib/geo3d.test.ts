import { describe, it, expect } from 'vitest'
import { simToScene, headingToSceneYaw, AREA_ROT_X } from './geo3d'

describe('geo3d', () => {
  it('maps sim origin to the scene origin', () => {
    expect(simToScene(0, 0)).toEqual([0, 0, 0])
  })

  it('maps East to +X and North to -Z', () => {
    expect(simToScene(100, 0)).toEqual([100, 0, 0]) // East
    expect(simToScene(0, 100)).toEqual([0, 0, -100]) // North → -Z
  })

  it('keeps an optional height on the Y axis', () => {
    expect(simToScene(10, 20, 5)).toEqual([10, 5, -20])
  })

  it('passes heading through as the scene yaw', () => {
    expect(headingToSceneYaw(Math.PI / 2)).toBeCloseTo(Math.PI / 2)
  })

  it('rotates the Z-up area meshes a quarter-turn onto Y-up', () => {
    expect(AREA_ROT_X).toBeCloseTo(-Math.PI / 2)
  })
})
