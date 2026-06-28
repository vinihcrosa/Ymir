import { test, expect } from '@playwright/test'

test('app loads without errors', async ({ page }) => {
  await page.goto('/')
  await expect(page).not.toHaveURL(/error/)
  // Basic smoke: page title or root element renders
  await expect(page.locator('body')).toBeVisible()
})
