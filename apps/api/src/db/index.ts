import Database from 'better-sqlite3'
import { drizzle } from 'drizzle-orm/better-sqlite3'
import * as schema from './schema.js'

const DB_PATH = process.env.DB_PATH ?? './ymir.db'

const sqlite = new Database(DB_PATH)
sqlite.pragma('journal_mode = WAL')
sqlite.pragma('foreign_keys = ON')

export const db = drizzle(sqlite, { schema })

export function runMigrations() {
  sqlite.exec(`
    CREATE TABLE IF NOT EXISTS areas (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      name TEXT NOT NULL,
      description TEXT,
      origin_lat REAL NOT NULL,
      origin_lng REAL NOT NULL,
      polygon TEXT NOT NULL,
      gravity REAL NOT NULL,
      magnetic_correction REAL NOT NULL,
      water_density REAL NOT NULL,
      air_density REAL NOT NULL,
      created_at TEXT NOT NULL
    );

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
      area_id INTEGER REFERENCES areas(id),
      duration REAL NOT NULL,
      dt REAL NOT NULL DEFAULT 0.1,
      initial_conditions TEXT NOT NULL,
      created_at TEXT NOT NULL
    );



    CREATE TABLE IF NOT EXISTS vessel_physical_props (
      vessel_id INTEGER PRIMARY KEY REFERENCES vessels(id),
      displacement_volumetric REAL NOT NULL,
      displacement_weight REAL NOT NULL,
      float_center TEXT NOT NULL,
      block_coefficient REAL NOT NULL,
      mass_center TEXT NOT NULL,
      mass_matrix TEXT NOT NULL,
      added_mass TEXT NOT NULL,
      hydrostatic_restoring TEXT NOT NULL,
      damping_linear_coeff REAL NOT NULL,
      damping_linear TEXT NOT NULL,
      damping_potential TEXT NOT NULL,
      damping_quadratic TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS vessel_current (
      vessel_id INTEGER PRIMARY KEY REFERENCES vessels(id),
      frontal_area REAL NOT NULL,
      frontal_height REAL NOT NULL,
      lateral_area REAL NOT NULL,
      lateral_height REAL NOT NULL,
      midship_distance REAL NOT NULL,
      coefficients TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS vessel_connection_points (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      vessel_id INTEGER NOT NULL REFERENCES vessels(id),
      point_id INTEGER NOT NULL,
      name TEXT NOT NULL,
      type TEXT NOT NULL,
      x REAL NOT NULL,
      y REAL NOT NULL,
      z REAL NOT NULL
    );

    CREATE TABLE IF NOT EXISTS vessel_rudders (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      vessel_id INTEGER NOT NULL REFERENCES vessels(id),
      rudder_id INTEGER NOT NULL,
      name TEXT NOT NULL,
      angle_maximum REAL NOT NULL,
      area REAL NOT NULL,
      speed REAL NOT NULL,
      associated_thruster INTEGER NOT NULL,
      position TEXT NOT NULL,
      force_table TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS vessel_thrusters (
      id INTEGER PRIMARY KEY AUTOINCREMENT,
      vessel_id INTEGER NOT NULL REFERENCES vessels(id),
      thruster_id INTEGER NOT NULL,
      name TEXT NOT NULL,
      position TEXT NOT NULL,
      azimuth_speed REAL NOT NULL,
      diameter REAL NOT NULL,
      rotation_speed REAL NOT NULL,
      rotation_speed_max REAL NOT NULL,
      rotation_time REAL NOT NULL,
      maximum_power REAL NOT NULL,
      mechanical_efficiency REAL NOT NULL,
      astern_efficiency REAL NOT NULL,
      hull_efficiency REAL NOT NULL,
      pitch_diameter_relation REAL NOT NULL,
      form_factor REAL NOT NULL,
      maximum_transversal_speed REAL NOT NULL,
      paddle_effect TEXT NOT NULL,
      force_table TEXT NOT NULL,
      eot_table TEXT NOT NULL
    );

    CREATE TABLE IF NOT EXISTS vessel_waves (
      vessel_id INTEGER PRIMARY KEY REFERENCES vessels(id),
      omega TEXT NOT NULL,
      angle TEXT NOT NULL,
      origin_position TEXT NOT NULL,
      forces_amplitude TEXT NOT NULL
    );
  `)

  // Guard: add area_id to scenarios for databases created before this column existed.
  const cols = sqlite.pragma('table_info(scenarios)') as Array<{ name: string }>
  if (!cols.some(c => c.name === 'area_id')) {
    sqlite.exec('ALTER TABLE scenarios ADD COLUMN area_id INTEGER REFERENCES areas(id);')
  }
}
