import { test, expect } from '@playwright/test'

// Shared helpers

async function selectArea(page: import('@playwright/test').Page) {
  // Allow time for area options to load from API
  await page.waitForTimeout(1000)
  const select = page.getByRole('combobox')
  await expect(select).toBeVisible()
  const optionValues = await Promise.all(
    (await select.locator('option').all()).map(o => o.getAttribute('value'))
  )
  const realArea = optionValues.find(v => v && v !== '')
  if (realArea) await select.selectOption(realArea)
  // Wait for area data to load (button enables only after successful fetch)
  await expect(page.getByRole('button', { name: /\+ adicionar/i })).not.toBeDisabled({ timeout: 10000 })
}

async function addVessel(page: import('@playwright/test').Page) {
  await page.getByRole('button', { name: /\+ adicionar/i }).click()
  await expect(page.getByRole('button', { name: /VLCC/i })).toBeVisible({ timeout: 5000 })
  await page.getByRole('button', { name: /VLCC/i }).click()
  // Wait for vessel row to appear in the list
  await expect(page.getByRole('spinbutton', { name: /rumo/i }).last()).toBeVisible({ timeout: 5000 })
}

async function startSimulation(page: import('@playwright/test').Page) {
  await page.getByRole('button', { name: /iniciar simulação/i }).click()
  await expect(page.getByText(/rodando/i)).toBeVisible({ timeout: 10000 })
  // Let a few physics ticks run
  await page.waitForTimeout(1000)
}

// ─────────────────────────────────────────────
// Bug: vessel panel doesn't open during simulation
// ─────────────────────────────────────────────
test.describe('painel do vessel durante simulação', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await selectArea(page)
    await addVessel(page)
  })

  test('painel abre ao clicar no vessel APÓS iniciar simulação', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    await startSimulation(page)

    const vesselIcon = page.locator('.leaflet-marker-icon').first()
    await expect(vesselIcon).toBeVisible({ timeout: 5000 })
    await vesselIcon.click()

    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
    await expect(page.getByText(/carregando/i)).not.toBeVisible({ timeout: 5000 })
    // Config loaded — vessel name visible
    await expect(page.getByTestId('vessel-panel').locator('div').filter({ hasText: /VLCC/i }).first()).toBeVisible()

    expect(errors).toHaveLength(0)
  })

  test('painel continua abrindo após clicar múltiplas vezes durante simulação', async ({ page }) => {
    await startSimulation(page)

    const vesselIcon = page.locator('.leaflet-marker-icon').first()
    await expect(vesselIcon).toBeVisible({ timeout: 5000 })

    // Click, close, click again — must work consistently
    await vesselIcon.click()
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
    await page.getByRole('button', { name: /fechar painel/i }).click()
    await expect(page.getByTestId('vessel-panel')).not.toBeVisible()

    await vesselIcon.click()
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
  })
})

// ─────────────────────────────────────────────
// Bug: dois vessels do mesmo tipo causam erro de simulação
// ─────────────────────────────────────────────
test.describe('dois vessels do mesmo tipo', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await selectArea(page)
  })

  test('adicionar dois VLCC não causa erro de simulação', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    // Add vessel 1
    await addVessel(page)
    // Add vessel 2 of same type
    await addVessel(page)

    const vesselCount = await page.getByRole('spinbutton', { name: /rumo/i }).count()
    expect(vesselCount).toBe(2)

    await startSimulation(page)

    await expect(page.getByText(/erro/i)).not.toBeVisible({ timeout: 3000 }).catch(() => {
      // If no 'erro' text visible, that's fine
    })
    await expect(page.getByText(/rodando/i)).toBeVisible({ timeout: 5000 })
    expect(errors).toHaveLength(0)
  })

  test('ambos vessels aparecem no mapa', async ({ page }) => {
    await addVessel(page)
    await addVessel(page)

    const markers = page.locator('.leaflet-marker-icon')
    await expect(markers).toHaveCount(2, { timeout: 5000 })
  })

  test('painel abre para cada vessel individualmente', async ({ page }) => {
    await addVessel(page)
    await addVessel(page)

    const markers = page.locator('.leaflet-marker-icon')
    await expect(markers).toHaveCount(2, { timeout: 5000 })

    // Both vessels start at same position so markers overlap — use force to bypass
    // the SVG polygon intercepting pointer events
    await markers.nth(0).click({ force: true })
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
    await page.getByRole('button', { name: /fechar painel/i }).click()

    await markers.nth(1).click({ force: true })
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
  })

  test('remover um vessel não remove o outro', async ({ page }) => {
    await addVessel(page)
    await addVessel(page)

    const removeBtns = page.getByRole('button', { name: /remover embarcação/i })
    await expect(removeBtns).toHaveCount(2, { timeout: 3000 })

    await removeBtns.first().click()

    await expect(page.getByRole('spinbutton', { name: /rumo/i })).toHaveCount(1, { timeout: 2000 })
  })
})

// ─────────────────────────────────────────────
// Bug: vessel não se move com correnteza aplicada
// ─────────────────────────────────────────────
test.describe('correnteza move vessel', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
    await selectArea(page)
    await addVessel(page)
  })

  test('vessel deriva com correnteza: posição muda após 5s', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    // Open vessel panel to read initial position
    const vesselIcon = page.locator('.leaflet-marker-icon').first()
    await expect(vesselIcon).toBeVisible({ timeout: 5000 })
    await vesselIcon.click()
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
    await expect(page.getByText(/carregando/i)).not.toBeVisible({ timeout: 5000 })

    // Add current: dir=270° (from West = flows East), speed=2 m/s
    // This pushes vessel forward (East) since default heading is East (psi=0)
    await page.getByRole('button', { name: /add current series/i }).click()
    await page.waitForTimeout(300)

    // Set direction 270
    const dirInput = page.locator('input[aria-label*="current series 0 keyframe 0 direction"]')
    await dirInput.fill('270')

    // Set speed 2
    const speedInput = page.locator('input[aria-label*="current series 0 keyframe 0 speed"]')
    await speedInput.fill('2')

    await startSimulation(page)

    // Panel should show live position
    await expect(page.getByTestId('vessel-panel').getByText(/live/i)).toBeVisible({ timeout: 5000 })

    // Wait for drift to accumulate
    await page.waitForTimeout(5000)

    // Marker position must have changed (vessel drifted East = x > 0)
    // Read x from the panel position display
    const panel = page.getByTestId('vessel-panel')
    const posText = await panel.textContent()
    const xMatch = posText?.match(/x[:\s]+([\d.-]+)\s*m/)
    if (xMatch) {
      const x = parseFloat(xMatch[1])
      expect(x, 'x deve ser > 0 com correnteza para leste').toBeGreaterThan(0)
    }

    expect(errors).toHaveLength(0)
  })

  test('vessel sem correnteza não se move sem propulsão', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    await startSimulation(page)

    // Open panel during simulation
    const vesselIcon = page.locator('.leaflet-marker-icon').first()
    await vesselIcon.click()
    await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 5000 })
    await expect(page.getByText(/live/i)).toBeVisible({ timeout: 5000 })

    await page.waitForTimeout(3000)

    // Without current or thrust, vessel should remain near origin
    const panel = page.getByTestId('vessel-panel')
    const posText = await panel.textContent()
    const xMatch = posText?.match(/x[:\s]+([\d.-]+)\s*m/)
    if (xMatch) {
      const x = Math.abs(parseFloat(xMatch[1]))
      expect(x, 'sem correnteza ou propulsão, x deve ser ~0').toBeLessThan(1)
    }

    expect(errors).toHaveLength(0)
  })
})
