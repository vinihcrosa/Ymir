import { defineConfig } from 'vitest/config'

export default defineConfig({
  test: {
    include: ['src/**/*.test.ts'],
    // Real WASM engine boots asynchronously and runs physics — allow headroom.
    testTimeout: 30000,
    hookTimeout: 30000,
    coverage: {
      provider: 'v8',
      reporter: ['text', 'lcov'],
      include: ['src/simulation/**/*.ts'],
      exclude: [
        'src/**/*.test.ts',
        // Worker/thread entrypoints need a live worker_thread; the logic they
        // wire is covered via MessageChannel unit tests.
        'src/simulation/simulation-worker.entry.ts',
      ],
      thresholds: {
        lines: 80,
        functions: 80,
        branches: 80,
        statements: 80,
      },
    },
  },
})
