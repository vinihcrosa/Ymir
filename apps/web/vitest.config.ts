import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'
import { resolve } from 'path'

export default defineConfig({
  plugins: [react()],
  test: {
    environment: 'jsdom',
    globals: true,
    setupFiles: ['./src/test/setup.ts'],
    include: ['src/**/*.{test,spec}.{ts,tsx}'],
    exclude: ['e2e/**', 'node_modules/**'],
    coverage: {
      provider: 'v8',
      reporter: ['text', 'lcov', 'html'],
      reportsDirectory: './coverage',
      include: ['src/**/*.{ts,tsx}'],
      exclude: [
        'src/test/**',
        'src/main.tsx',
        'src/App.tsx',
        'src/**/*.d.ts',
        'src/**/index.ts',
        // Leaflet + Three.js scene components — require canvas/WebGL, tested via E2E
        'src/features/scenario-creator/components/AreaMapView.tsx',
        'src/features/scenario-creator/components/VesselMarker.tsx',
        'src/features/scenario-creator/components/Area3DView.tsx',
        // Pure layout composition — no logic to unit test
        'src/features/scenario-creator/components/Sidebar.tsx',
        // Web Workers — not runnable in jsdom
        'src/workers/**',
      ],
      thresholds: {
        lines: 80,
        functions: 80,
        branches: 80,
        statements: 80,
      },
    },
  },
  resolve: {
    alias: {
      '@ymir/types': resolve(__dirname, '../../packages/types/src/index.ts'),
    },
  },
})
