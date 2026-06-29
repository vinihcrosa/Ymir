import { Type, Static } from '@sinclair/typebox'

export const EnvironmentConditionDTO = Type.Object({
  t:       Type.Number({ description: 'keyframe time [s]' }),
  speed:   Type.Number({ minimum: 0, description: 'speed [m/s]' }),
  dirNaut: Type.Number({ minimum: 0, maximum: 360, description: 'nautical direction [deg]' }),
})
export type EnvironmentConditionDTO = Static<typeof EnvironmentConditionDTO>

export const WaveConditionDTO = Type.Object({
  t:        Type.Number({ description: 'keyframe time [s]' }),
  Hs:       Type.Number({ minimum: 0, description: 'significant wave height [m]' }),
  Tp:       Type.Number({ minimum: 0, description: 'peak wave period [s]' }),
  dirNaut:  Type.Number({ minimum: 0, maximum: 360, description: 'nautical direction [deg]' }),
  spectrum: Type.Union([
    Type.Literal('JONSWAP'),
    Type.Literal('PIERSON'),
    Type.Literal('REGULAR'),
  ]),
  gamma:    Type.Number({ minimum: 0, default: 3.3, description: 'JONSWAP peak enhancement factor' }),
})
export type WaveConditionDTO = Static<typeof WaveConditionDTO>

export const EnvironmentProfileDTO = Type.Object({
  currentSeries: Type.Array(Type.Array(EnvironmentConditionDTO)),
  windSeries:    Type.Array(Type.Array(EnvironmentConditionDTO)),
  waveSeries:    Type.Array(WaveConditionDTO),
})
export type EnvironmentProfileDTO = Static<typeof EnvironmentProfileDTO>
