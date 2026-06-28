import { Static } from '@sinclair/typebox';
export declare const InitialCondition: import("@sinclair/typebox").TObject<{
    vesselId: import("@sinclair/typebox").TNumber;
    x: import("@sinclair/typebox").TNumber;
    y: import("@sinclair/typebox").TNumber;
    psi: import("@sinclair/typebox").TNumber;
    u: import("@sinclair/typebox").TNumber;
    v: import("@sinclair/typebox").TNumber;
    r: import("@sinclair/typebox").TNumber;
}>;
export type InitialCondition = Static<typeof InitialCondition>;
export declare const ScenarioDTO: import("@sinclair/typebox").TObject<{
    id: import("@sinclair/typebox").TNumber;
    name: import("@sinclair/typebox").TString;
    description: import("@sinclair/typebox").TOptional<import("@sinclair/typebox").TString>;
    duration: import("@sinclair/typebox").TNumber;
    dt: import("@sinclair/typebox").TNumber;
    initialConditions: import("@sinclair/typebox").TArray<import("@sinclair/typebox").TObject<{
        vesselId: import("@sinclair/typebox").TNumber;
        x: import("@sinclair/typebox").TNumber;
        y: import("@sinclair/typebox").TNumber;
        psi: import("@sinclair/typebox").TNumber;
        u: import("@sinclair/typebox").TNumber;
        v: import("@sinclair/typebox").TNumber;
        r: import("@sinclair/typebox").TNumber;
    }>>;
    createdAt: import("@sinclair/typebox").TString;
}>;
export type ScenarioDTO = Static<typeof ScenarioDTO>;
export declare const CreateScenarioDTO: import("@sinclair/typebox").TObject<{
    name: import("@sinclair/typebox").TString;
    description: import("@sinclair/typebox").TOptional<import("@sinclair/typebox").TString>;
    duration: import("@sinclair/typebox").TNumber;
    dt: import("@sinclair/typebox").TNumber;
    initialConditions: import("@sinclair/typebox").TArray<import("@sinclair/typebox").TObject<{
        vesselId: import("@sinclair/typebox").TNumber;
        x: import("@sinclair/typebox").TNumber;
        y: import("@sinclair/typebox").TNumber;
        psi: import("@sinclair/typebox").TNumber;
        u: import("@sinclair/typebox").TNumber;
        v: import("@sinclair/typebox").TNumber;
        r: import("@sinclair/typebox").TNumber;
    }>>;
}>;
export type CreateScenarioDTO = Static<typeof CreateScenarioDTO>;
//# sourceMappingURL=scenario.d.ts.map