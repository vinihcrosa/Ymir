import { Type } from '@sinclair/typebox';
export const InitialCondition = Type.Object({
    vesselId: Type.Number(),
    x: Type.Number({ default: 0, description: 'Initial surge position [m]' }),
    y: Type.Number({ default: 0, description: 'Initial sway position [m]' }),
    psi: Type.Number({ default: 0, description: 'Initial heading [rad]' }),
    u: Type.Number({ default: 0, description: 'Initial surge velocity [m/s]' }),
    v: Type.Number({ default: 0, description: 'Initial sway velocity [m/s]' }),
    r: Type.Number({ default: 0, description: 'Initial yaw rate [rad/s]' }),
});
export const ScenarioDTO = Type.Object({
    id: Type.Number(),
    name: Type.String(),
    description: Type.Optional(Type.String()),
    duration: Type.Number({ description: 'Scenario duration [s]' }),
    dt: Type.Number({ description: 'Time step [s]', default: 0.1 }),
    initialConditions: Type.Array(InitialCondition),
    createdAt: Type.String({ format: 'date-time' }),
});
export const CreateScenarioDTO = Type.Omit(ScenarioDTO, ['id', 'createdAt']);
//# sourceMappingURL=scenario.js.map