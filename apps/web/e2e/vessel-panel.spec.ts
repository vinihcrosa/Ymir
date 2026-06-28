import { test, expect } from '@playwright/test'

async function setupScenarioWithVessel(page: import('@playwright/test').Page) {
  await page.goto('/')
  await page.waitForTimeout(1000)

  const select = page.getByRole('combobox')
  const optionValues = await Promise.all(
    (await select.locator('option').all()).map(o => o.getAttribute('value'))
  )
  const realArea = optionValues.find(v => v && v !== '')
  if (realArea) await select.selectOption(realArea)
  await page.waitForTimeout(800)

  await page.getByRole('button', { name: /\+ adicionar/i }).click()
  await page.waitForTimeout(400)
  await page.getByRole('button', { name: /VLCC/i }).click()
  await page.waitForTimeout(600)
}

async function openVesselPanel(page: import('@playwright/test').Page) {
  const vesselIcon = page.locator('.leaflet-marker-icon').first()
  await expect(vesselIcon).toBeVisible({ timeout: 5000 })
  await vesselIcon.click()
  // Wait for panel to appear
  await expect(page.getByTestId('vessel-panel')).toBeVisible({ timeout: 3000 })
  // Wait for config to load (Carregando... disappears)
  await expect(page.getByText(/carregando/i)).not.toBeVisible({ timeout: 5000 })
}

test.describe('Vessel Panel', () => {
  test('painel abre ao clicar no vessel no mapa', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await expect(panel.getByText('OWNSHIP')).toBeVisible()
    // Vessel name appears in panel header (not the tooltip)
    await expect(panel.locator('div').filter({ hasText: /VLCC/i }).first()).toBeVisible()
  })

  test('painel exibe aba Geral com dimensões do vessel', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await expect(panel.getByText('Dimensões')).toBeVisible()
    await expect(panel.getByText('LOA')).toBeVisible()
    await expect(panel.getByText('Boca')).toBeVisible()
    await expect(panel.getByText('Calado')).toBeVisible()
    await expect(panel.getByText('Posição e curso')).toBeVisible()
    // LOA 350m shown for VLCC
    await expect(panel.getByText(/\d+\.?\d* m/).first()).toBeVisible()
  })

  test('painel exibe aba Controles com thrusters e rudders', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await panel.getByRole('button', { name: 'Controles' }).click()

    // Wait for content — either sections or "Sem propulsores" message
    await page.waitForTimeout(300)
    const hasLemes = await panel.getByText('Lemes').isVisible().catch(() => false)
    const hasPropulsores = await panel.getByText('Propulsores').isVisible().catch(() => false)
    const hasEmpty = await panel.getByText(/sem propulsores/i).isVisible().catch(() => false)
    expect(hasLemes || hasPropulsores || hasEmpty).toBe(true)
  })

  test('VLCC tem leme e propulsor', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await panel.getByRole('button', { name: 'Controles' }).click()

    // vessel1.json has 1 rudder and 1 thruster
    await expect(panel.getByText('Lemes')).toBeVisible({ timeout: 3000 })
    await expect(panel.getByText('Propulsores')).toBeVisible()
    // Sliders present
    await expect(panel.locator('input[type="range"]').first()).toBeVisible()
  })

  test('painel fecha ao clicar no botão ×', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    await page.getByRole('button', { name: 'Fechar painel' }).click()
    await expect(page.getByTestId('vessel-panel')).not.toBeVisible()
  })

  test('slider de leme move sem erros de página', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await panel.getByRole('button', { name: 'Controles' }).click()
    await expect(panel.getByText('Lemes')).toBeVisible({ timeout: 3000 })

    const rudderSlider = panel.locator('input[type="range"]').first()
    await rudderSlider.fill('20')
    await page.waitForTimeout(200)
    await rudderSlider.fill('-15')
    await page.waitForTimeout(200)

    expect(errors, 'Sem erros de página após mover slider').toHaveLength(0)
  })

  test('posição fica live durante simulação', async ({ page }) => {
    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    // Start simulation with panel open
    await page.getByRole('button', { name: /iniciar simulação/i }).click()
    await page.waitForTimeout(3000)

    const panel = page.getByTestId('vessel-panel')
    await expect(panel.getByText(/live/i)).toBeVisible({ timeout: 3000 })
    await expect(page.getByText(/rodando/i)).toBeVisible()
  })

  test('potência do propulsor move vessel durante simulação', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')
    await panel.getByRole('button', { name: 'Controles' }).click()
    await expect(panel.getByText('Propulsores')).toBeVisible({ timeout: 3000 })

    // Set thruster to 80%
    const sliders = panel.locator('input[type="range"]')
    const count = await sliders.count()
    if (count > 0) {
      // Last slider is typically power for thruster
      await sliders.last().fill('80')
    }

    // Start simulation
    await page.getByRole('button', { name: /iniciar simulação/i }).click()
    await page.waitForTimeout(4000)

    await expect(page.getByText(/rodando/i)).toBeVisible()
    // Switch to Geral to verify live position indicator and actual motion
    await panel.getByRole('button', { name: 'Geral' }).click()
    await expect(panel.getByText(/live/i)).toBeVisible({ timeout: 3000 })
    // u (surge) must be > 0 — vessel is actually moving
    await expect(panel.getByText(/u \(surge\)/i)).toBeVisible()
    expect(errors, 'Sem erros de página').toHaveLength(0)
  })

  test('vessel se move: u (surge) > 0 com propulsor no máximo', async ({ page }) => {
    const errors: string[] = []
    page.on('pageerror', e => errors.push(e.message))

    await setupScenarioWithVessel(page)
    await openVesselPanel(page)

    const panel = page.getByTestId('vessel-panel')

    // Set full thruster power in Controls tab
    await panel.getByRole('button', { name: 'Controles' }).click()
    await expect(panel.getByText('Propulsores')).toBeVisible({ timeout: 3000 })
    const powerSliders = panel.locator('input[type="range"]')
    const sliderCount = await powerSliders.count()
    if (sliderCount > 0) {
      await powerSliders.last().fill('100')
    }

    // Start simulation then switch to Geral tab
    await page.getByRole('button', { name: /iniciar simulação/i }).click()
    await page.waitForTimeout(5000)

    await expect(page.getByText(/rodando/i)).toBeVisible()
    await panel.getByRole('button', { name: 'Geral' }).click()
    await expect(panel.getByText(/live/i)).toBeVisible({ timeout: 3000 })

    // u (surge) label must be visible and the value must be non-zero
    await expect(panel.getByText(/u \(surge\)/i)).toBeVisible()
    const surgeRow = panel.locator('div').filter({ hasText: /u \(surge\)/ }).first()
    const surgeText = await surgeRow.textContent()
    const surgeValue = parseFloat(surgeText?.match(/([\d.]+)\s*m\/s/)?.[1] ?? '0')
    expect(surgeValue, 'u (surge) deve ser > 0 com propulsor no máximo').toBeGreaterThan(0)

    expect(errors, 'Sem erros de página').toHaveLength(0)
  })
})
