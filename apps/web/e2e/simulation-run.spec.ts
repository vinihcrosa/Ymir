import { test, expect } from '@playwright/test'

test('start simulation and capture errors', async ({ page }) => {
  const errors: string[] = []
  const consoleErrors: string[] = []

  page.on('pageerror', e => errors.push(e.message + '\n' + e.stack))
  page.on('console', m => {
    if (m.type() === 'error') consoleErrors.push(m.text())
  })

  await page.goto('/')
  await page.waitForTimeout(1500)

  // Select area
  const select = page.getByRole('combobox')
  await expect(select).toBeVisible()
  const options = await select.locator('option').all()
  console.log('area options:', await Promise.all(options.map(o => o.textContent())))

  // Pick first real area (skip empty/loading placeholder)
  const optionValues = await Promise.all(options.map(o => o.getAttribute('value')))
  const realOption = optionValues.find(v => v && v !== '')
  console.log('selecting area value:', realOption)
  if (realOption) await select.selectOption(realOption)
  await page.waitForTimeout(1000)

  // Add vessel — open picker then click the vessel button inside it
  const addBtn = page.getByRole('button', { name: /\+ adicionar/i })
  await expect(addBtn).not.toBeDisabled()
  await addBtn.click()
  await page.waitForTimeout(500)

  // Click the VLCC vessel button that appears in the picker
  const vesselBtn = page.getByRole('button', { name: /VLCC/i })
  await expect(vesselBtn).toBeVisible({ timeout: 3000 })
  console.log('vessel button text:', await vesselBtn.textContent())
  await vesselBtn.click()
  await page.waitForTimeout(500)

  const vesselCount = await page.getByRole('spinbutton', { name: /rumo/i }).count()
  console.log('vessels added:', vesselCount)

  // Start simulation
  const startBtn = page.getByRole('button', { name: /iniciar simulação/i })
  await expect(startBtn).not.toBeDisabled()
  await startBtn.click()
  await page.waitForTimeout(4000)

  // Check status
  const bodyText = await page.locator('body').innerText()
  await page.screenshot({ path: 'test-results/simulation-running.png', fullPage: false })
  console.log('body text snippet:', bodyText.slice(0, 500))
  console.log('PAGE ERRORS:', errors)
  console.log('CONSOLE ERRORS:', consoleErrors)

  expect(errors, 'Page errors during simulation').toHaveLength(0)
  expect(consoleErrors, 'Console errors during simulation').toHaveLength(0)
  await expect(page.getByText(/rodando/i)).toBeVisible()
})
