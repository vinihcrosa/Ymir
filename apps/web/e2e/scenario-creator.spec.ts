import { test, expect } from '@playwright/test'

test.describe('Scenario Creator', () => {
  test.beforeEach(async ({ page }) => {
    await page.goto('/')
  })

  test('sidebar is visible on load', async ({ page }) => {
    await expect(page.getByRole('heading', { name: /criar cenário/i })).toBeVisible()
  })

  test('scenario name input is present', async ({ page }) => {
    await expect(page.getByRole('textbox')).toBeVisible()
  })

  test('area select is present', async ({ page }) => {
    await expect(page.getByRole('combobox')).toBeVisible()
  })

  test('start button is disabled initially', async ({ page }) => {
    const startBtn = page.getByRole('button', { name: /iniciar simulação/i })
    await expect(startBtn).toBeVisible()
    await expect(startBtn).toBeDisabled()
  })

  test('add vessel button is disabled without area', async ({ page }) => {
    const addBtn = page.getByRole('button', { name: /adicionar/i })
    await expect(addBtn).toBeVisible()
    await expect(addBtn).toBeDisabled()
  })

  test('map container is present', async ({ page }) => {
    // Leaflet renders a div with class leaflet-container
    await expect(page.locator('.leaflet-container')).toBeVisible({ timeout: 5000 })
  })
})
