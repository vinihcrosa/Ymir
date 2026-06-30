import { Type, Static } from '@sinclair/typebox'
import { EnvironmentProfileDTO } from './environment.js'

export const InitialCondition = Type.Object({
  /** Per-scenario body identifier used by the physics engine. */
  instanceId: Type.Number(),
  /** Vessel TYPE id — used to load the vessel config when the scenario is reopened. */
  vesselId: Type.Number(),
  x: Type.Number({ default: 0, description: 'Initial surge position [m]' }),
  y: Type.Number({ default: 0, description: 'Initial sway position [m]' }),
  psi: Type.Number({ default: 0, description: 'Initial heading [rad]' }),
  u: Type.Number({ default: 0, description: 'Initial surge velocity [m/s]' }),
  v: Type.Number({ default: 0, description: 'Initial sway velocity [m/s]' }),
  r: Type.Number({ default: 0, description: 'Initial yaw rate [rad/s]' }),
})
export type InitialCondition = Static<typeof InitialCondition>

export const ScenarioDTO = Type.Object({
  id: Type.Number(),
  name: Type.String(),
  description: Type.Optional(Type.String()),
  /** Area where the simulation takes place — required for new scenarios */
  areaId: Type.Optional(Type.Number()),
  duration: Type.Number({ description: 'Scenario duration [s]' }),
  dt: Type.Number({ description: 'Time step [s]', default: 0.1 }),
  initialConditions: Type.Array(InitialCondition),
  createdAt: Type.String({ format: 'date-time' }),
  environment: Type.Optional(EnvironmentProfileDTO),
})
export type ScenarioDTO = Static<typeof ScenarioDTO>

export const CreateScenarioDTO = Type.Omit(ScenarioDTO, ['id', 'createdAt'])
export type CreateScenarioDTO = Static<typeof CreateScenarioDTO>
