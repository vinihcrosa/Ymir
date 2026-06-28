import Database from 'better-sqlite3';
import { drizzle } from 'drizzle-orm/better-sqlite3';
import { vessels, scenarios } from './schema.js';
import * as schema from './schema.js';
const DB_PATH = process.env.DB_PATH ?? './ymir.db';
const sqlite = new Database(DB_PATH);
sqlite.pragma('journal_mode = WAL');
export const db = drizzle(sqlite, { schema });
export function runMigrations() {
    sqlite.exec(`
    CREATE TABLE IF NOT EXISTS vessels (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      length REAL NOT NULL,
      beam REAL NOT NULL,
      draft REAL NOT NULL,
      mass REAL NOT NULL,
      created_at TEXT NOT NULL
    );
    CREATE TABLE IF NOT EXISTS scenarios (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      description TEXT,
      duration REAL NOT NULL,
      dt REAL NOT NULL DEFAULT 0.1,
      initial_conditions TEXT NOT NULL,
      created_at TEXT NOT NULL
    );
  `);
}
export function seedIfEmpty() {
    const count = db.select().from(vessels).all().length;
    if (count > 0)
        return;
    const now = new Date().toISOString();
    db.insert(vessels).values([
        { name: 'Rebocador RB-01', length: 32.0, beam: 10.5, draft: 4.2, mass: 850_000, createdAt: now },
        { name: 'AHTS Ymir-I', length: 75.0, beam: 18.0, draft: 6.5, mass: 4_200_000, createdAt: now },
    ]).run();
    db.insert(scenarios).values([
        {
            name: 'Berço em repouso',
            description: 'Dois vessels em repouso sem forças externas',
            duration: 60,
            dt: 0.1,
            initialConditions: JSON.stringify([
                { vesselId: 1, x: 0, y: 0, psi: 0, u: 0, v: 0, r: 0 },
                { vesselId: 2, x: 100, y: 0, psi: 0, u: 0, v: 0, r: 0 },
            ]),
            createdAt: now,
        },
    ]).run();
}
