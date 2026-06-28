import { sqliteTable, integer, real, text } from 'drizzle-orm/sqlite-core';
export const vessels = sqliteTable('vessels', {
    id: integer('id').primaryKey({ autoIncrement: true }),
    name: text('name').notNull(),
    length: real('length').notNull(),
    beam: real('beam').notNull(),
    draft: real('draft').notNull(),
    mass: real('mass').notNull(),
    createdAt: text('created_at').notNull(),
});
export const scenarios = sqliteTable('scenarios', {
    id: integer('id').primaryKey({ autoIncrement: true }),
    name: text('name').notNull(),
    description: text('description'),
    duration: real('duration').notNull(),
    dt: real('dt').notNull().default(0.1),
    initialConditions: text('initial_conditions').notNull(), // JSON string
    createdAt: text('created_at').notNull(),
});
