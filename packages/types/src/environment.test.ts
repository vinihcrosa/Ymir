import { describe, it, expect } from 'vitest'
import { Value } from '@sinclair/typebox/value'
import { FormatRegistry } from '@sinclair/typebox'
import {
  EnvironmentConditionDTO,
  WaveConditionDTO,
  EnvironmentProfileDTO,
} from './environment.js'
import { ScenarioDTO, CreateScenarioDTO } from './scenario.js'

// Register date-time format so ScenarioDTO validation works
FormatRegistry.Set('date-time', (v) => !isNaN(Date.parse(v)))

describe('EnvironmentConditionDTO', () => {
  it('passes validation for a valid condition', () => {
    expect(Value.Check(EnvironmentConditionDTO, { t: 0, speed: 1.0, dirNaut: 90 })).toBe(true)
  })

  it('fails validation when speed is negative', () => {
    expect(Value.Check(EnvironmentConditionDTO, { t: 0, speed: -1, dirNaut: 90 })).toBe(false)
  })

  it('fails validation when dirNaut exceeds 360', () => {
    expect(Value.Check(EnvironmentConditionDTO, { t: 0, speed: 1, dirNaut: 361 })).toBe(false)
  })

  it('fails validation when dirNaut is negative', () => {
    expect(Value.Check(EnvironmentConditionDTO, { t: 0, speed: 1, dirNaut: -1 })).toBe(false)
  })

  it('passes validation with t=0, speed=0, dirNaut=0', () => {
    expect(Value.Check(EnvironmentConditionDTO, { t: 0, speed: 0, dirNaut: 0 })).toBe(true)
  })
})

describe('WaveConditionDTO', () => {
  const valid = {
    t: 0, Hs: 2.0, Tp: 10.0, dirNaut: 270, spectrum: 'JONSWAP' as const, gamma: 3.3,
  }

  it('passes validation for a valid JONSWAP wave', () => {
    expect(Value.Check(WaveConditionDTO, valid)).toBe(true)
  })

  it('passes validation for PIERSON spectrum', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, spectrum: 'PIERSON' })).toBe(true)
  })

  it('passes validation for REGULAR spectrum', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, spectrum: 'REGULAR' })).toBe(true)
  })

  it('fails validation for invalid spectrum string', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, spectrum: 'INVALID' })).toBe(false)
  })

  it('fails validation when Hs is negative', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, Hs: -0.1 })).toBe(false)
  })

  it('fails validation when Tp is negative', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, Tp: -1 })).toBe(false)
  })

  it('fails validation when gamma is negative', () => {
    expect(Value.Check(WaveConditionDTO, { ...valid, gamma: -0.1 })).toBe(false)
  })
})

describe('EnvironmentProfileDTO', () => {
  const empty = { currentSeries: [], windSeries: [], waveSeries: [] }

  it('passes validation with empty arrays for all three series', () => {
    expect(Value.Check(EnvironmentProfileDTO, empty)).toBe(true)
  })

  it('passes validation with populated series', () => {
    const profile = {
      currentSeries: [[{ t: 0, speed: 0.5, dirNaut: 90 }]],
      windSeries: [[{ t: 0, speed: 10.0, dirNaut: 0 }]],
      waveSeries: [{ t: 0, Hs: 2.0, Tp: 10.0, dirNaut: 270, spectrum: 'JONSWAP' as const, gamma: 3.3 }],
    }
    expect(Value.Check(EnvironmentProfileDTO, profile)).toBe(true)
  })

  it('fails validation when a current condition has negative speed', () => {
    const invalid = {
      ...empty,
      currentSeries: [[{ t: 0, speed: -1, dirNaut: 90 }]],
    }
    expect(Value.Check(EnvironmentProfileDTO, invalid)).toBe(false)
  })
})

describe('ScenarioDTO with environment field', () => {
  const base = {
    id: 1,
    name: 'Test',
    duration: 3600,
    dt: 0.1,
    initialConditions: [],
    createdAt: '2026-01-01T00:00:00.000Z',
  }

  it('passes validation without environment field (optional)', () => {
    expect(Value.Check(ScenarioDTO, base)).toBe(true)
  })

  it('passes validation with a valid environment object', () => {
    const withEnv = {
      ...base,
      environment: { currentSeries: [], windSeries: [], waveSeries: [] },
    }
    expect(Value.Check(ScenarioDTO, withEnv)).toBe(true)
  })

  it('fails validation when environment contains invalid data', () => {
    const withBadEnv = {
      ...base,
      environment: {
        currentSeries: [[{ t: 0, speed: -5, dirNaut: 90 }]],
        windSeries: [],
        waveSeries: [],
      },
    }
    expect(Value.Check(ScenarioDTO, withBadEnv)).toBe(false)
  })
})

describe('CreateScenarioDTO', () => {
  it('omits id and createdAt', () => {
    const schema = CreateScenarioDTO
    const props = Object.keys(schema.properties)
    expect(props).not.toContain('id')
    expect(props).not.toContain('createdAt')
  })

  it('retains environment field', () => {
    const props = Object.keys(CreateScenarioDTO.properties)
    expect(props).toContain('environment')
  })

  it('passes validation with environment included', () => {
    const dto = {
      name: 'Test',
      duration: 3600,
      dt: 0.1,
      initialConditions: [],
      environment: { currentSeries: [], windSeries: [], waveSeries: [] },
    }
    expect(Value.Check(CreateScenarioDTO, dto)).toBe(true)
  })

  it('passes validation without environment (optional)', () => {
    const dto = {
      name: 'Test',
      duration: 3600,
      dt: 0.1,
      initialConditions: [],
    }
    expect(Value.Check(CreateScenarioDTO, dto)).toBe(true)
  })
})
