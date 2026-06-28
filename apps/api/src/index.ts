import Fastify from 'fastify'
import cors from '@fastify/cors'
import { healthRoutes } from './routes/health.js'
import { vesselRoutes } from './routes/vessels.js'
import { scenarioRoutes } from './routes/scenarios.js'
import { runMigrations } from './db/index.js'
import { seedVessel1, seedPlaceholdersIfEmpty } from './db/seed-vessel1.js'

const app = Fastify({ logger: true })

await app.register(cors, { origin: true })
await app.register(healthRoutes)
await app.register(vesselRoutes)
await app.register(scenarioRoutes)

const port = Number(process.env.PORT ?? 3000)
const host = process.env.HOST ?? '0.0.0.0'

try {
  runMigrations()
  seedVessel1()
  seedPlaceholdersIfEmpty()
  await app.listen({ port, host })
} catch (err) {
  app.log.error(err)
  process.exit(1)
}
