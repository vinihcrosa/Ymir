import { Static } from '@sinclair/typebox';
export declare const VesselDTO: import("@sinclair/typebox").TObject<{
    id: import("@sinclair/typebox").TNumber;
    name: import("@sinclair/typebox").TString;
    length: import("@sinclair/typebox").TNumber;
    beam: import("@sinclair/typebox").TNumber;
    draft: import("@sinclair/typebox").TNumber;
    mass: import("@sinclair/typebox").TNumber;
    createdAt: import("@sinclair/typebox").TString;
}>;
export type VesselDTO = Static<typeof VesselDTO>;
export declare const CreateVesselDTO: import("@sinclair/typebox").TObject<{
    name: import("@sinclair/typebox").TString;
    length: import("@sinclair/typebox").TNumber;
    beam: import("@sinclair/typebox").TNumber;
    draft: import("@sinclair/typebox").TNumber;
    mass: import("@sinclair/typebox").TNumber;
}>;
export type CreateVesselDTO = Static<typeof CreateVesselDTO>;
//# sourceMappingURL=vessel.d.ts.map