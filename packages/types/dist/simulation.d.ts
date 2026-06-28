import { Static } from '@sinclair/typebox';
export declare const VesselStateDTO: import("@sinclair/typebox").TObject<{
    id: import("@sinclair/typebox").TNumber;
    x: import("@sinclair/typebox").TNumber;
    y: import("@sinclair/typebox").TNumber;
    z: import("@sinclair/typebox").TNumber;
    phi: import("@sinclair/typebox").TNumber;
    theta: import("@sinclair/typebox").TNumber;
    psi: import("@sinclair/typebox").TNumber;
    u: import("@sinclair/typebox").TNumber;
    v: import("@sinclair/typebox").TNumber;
    r: import("@sinclair/typebox").TNumber;
}>;
export type VesselStateDTO = Static<typeof VesselStateDTO>;
export declare const SimulationStateDTO: import("@sinclair/typebox").TObject<{
    t: import("@sinclair/typebox").TNumber;
    vessels: import("@sinclair/typebox").TArray<import("@sinclair/typebox").TObject<{
        id: import("@sinclair/typebox").TNumber;
        x: import("@sinclair/typebox").TNumber;
        y: import("@sinclair/typebox").TNumber;
        z: import("@sinclair/typebox").TNumber;
        phi: import("@sinclair/typebox").TNumber;
        theta: import("@sinclair/typebox").TNumber;
        psi: import("@sinclair/typebox").TNumber;
        u: import("@sinclair/typebox").TNumber;
        v: import("@sinclair/typebox").TNumber;
        r: import("@sinclair/typebox").TNumber;
    }>>;
}>;
export type SimulationStateDTO = Static<typeof SimulationStateDTO>;
export declare const WorkerMessageDTO: import("@sinclair/typebox").TUnion<[import("@sinclair/typebox").TObject<{
    type: import("@sinclair/typebox").TLiteral<"ready">;
}>, import("@sinclair/typebox").TObject<{
    type: import("@sinclair/typebox").TLiteral<"state">;
    payload: import("@sinclair/typebox").TObject<{
        t: import("@sinclair/typebox").TNumber;
        vessels: import("@sinclair/typebox").TArray<import("@sinclair/typebox").TObject<{
            id: import("@sinclair/typebox").TNumber;
            x: import("@sinclair/typebox").TNumber;
            y: import("@sinclair/typebox").TNumber;
            z: import("@sinclair/typebox").TNumber;
            phi: import("@sinclair/typebox").TNumber;
            theta: import("@sinclair/typebox").TNumber;
            psi: import("@sinclair/typebox").TNumber;
            u: import("@sinclair/typebox").TNumber;
            v: import("@sinclair/typebox").TNumber;
            r: import("@sinclair/typebox").TNumber;
        }>>;
    }>;
}>, import("@sinclair/typebox").TObject<{
    type: import("@sinclair/typebox").TLiteral<"error">;
    message: import("@sinclair/typebox").TString;
}>]>;
export type WorkerMessageDTO = Static<typeof WorkerMessageDTO>;
//# sourceMappingURL=simulation.d.ts.map