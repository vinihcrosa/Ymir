import { Type } from '@sinclair/typebox';
import { db } from '../db/index.js';
import { scenarios } from '../db/schema.js';
import { eq } from 'drizzle-orm';
import { ScenarioDTO, CreateScenarioDTO } from '@ymir/types';
function rowToScenario(row) {
    return {
        ...row,
        initialConditions: JSON.parse(row.initialConditions),
        description: row.description ?? undefined,
        createdAt: row.createdAt,
    };
}
export const scenarioRoutes = async (app) => {
    app.get('/scenarios', {
        schema: { response: { 200: Type.Array(ScenarioDTO) } },
    }, async () => {
        return db.select().from(scenarios).all().map(rowToScenario);
    });
    app.get('/scenarios/:id', {
        schema: { response: { 200: ScenarioDTO } },
    }, async (req, reply) => {
        const id = Number(req.params.id);
        const row = db.select().from(scenarios).where(eq(scenarios.id, id)).get();
        if (!row)
            return reply.status(404).send({ error: 'Scenario not found' });
        return rowToScenario(row);
    });
    app.post('/scenarios', {
        schema: { body: CreateScenarioDTO, response: { 201: ScenarioDTO } },
    }, async (req, reply) => {
        const now = new Date().toISOString();
        const result = db.insert(scenarios).values({
            ...req.body,
            initialConditions: JSON.stringify(req.body.initialConditions),
            createdAt: now,
        }).returning().get();
        return reply.status(201).send(rowToScenario(result));
    });
    app.delete('/scenarios/:id', async (req, reply) => {
        const id = Number(req.params.id);
        db.delete(scenarios).where(eq(scenarios.id, id)).run();
        return reply.status(204).send();
    });
};
