import { useState, type CSSProperties, type HTMLAttributes, type ReactNode } from 'react'
import { tokens } from '../theme/tokens'

export interface TabItem {
  id: string
  label: string
  content: ReactNode
}

export interface TabsProps extends Omit<HTMLAttributes<HTMLDivElement>, 'onChange'> {
  /** Tab definitions: id, label and panel content. */
  tabs: TabItem[]
  /** Controlled active tab id. When provided, internal state is ignored. */
  activeId?: string
  /** Initial active tab id when uncontrolled. Defaults to the first tab. */
  defaultActiveId?: string
  /** Fired with the tab id whenever a tab is selected. */
  onChange?: (id: string) => void
}

const stripStyle: CSSProperties = {
  display: 'flex',
  gap: tokens.space.lg,
  borderBottom: `1px solid ${tokens.color.border}`,
}

function tabStyle(active: boolean): CSSProperties {
  return {
    appearance: 'none',
    background: 'none',
    border: 'none',
    cursor: 'pointer',
    padding: `${tokens.space.sm}px 0`,
    marginBottom: -1,
    fontFamily: tokens.font.sans,
    fontSize: tokens.fontSize.body,
    fontWeight: active ? tokens.fontWeight.semibold : tokens.fontWeight.medium,
    color: active ? tokens.color.accentText : tokens.color.textSubtle,
    borderBottom: active ? `2px solid ${tokens.color.accent}` : '2px solid transparent',
  }
}

const panelStyle: CSSProperties = {
  paddingTop: tokens.space.md,
  fontFamily: tokens.font.sans,
  fontSize: tokens.fontSize.body,
  color: tokens.color.textPrimary,
}

/** Tab bar plus the active tab's panel. Controlled via `activeId` or self-managed. */
export function Tabs({ tabs, activeId, defaultActiveId, onChange, style, ...rest }: TabsProps) {
  const [internalId, setInternalId] = useState<string>(
    () => defaultActiveId ?? tabs[0]?.id,
  )
  const isControlled = activeId !== undefined
  const currentId = isControlled ? activeId : internalId

  const handleSelect = (id: string) => {
    if (!isControlled) setInternalId(id)
    onChange?.(id)
  }

  const activeTab = tabs.find((t) => t.id === currentId) ?? tabs[0]

  return (
    <div style={style} {...rest}>
      <div role="tablist" style={stripStyle}>
        {tabs.map((tab) => {
          const active = tab.id === activeTab?.id
          return (
            <button
              key={tab.id}
              type="button"
              role="tab"
              aria-selected={active}
              id={`tab-${tab.id}`}
              aria-controls={`tabpanel-${tab.id}`}
              tabIndex={active ? 0 : -1}
              style={tabStyle(active)}
              onClick={() => handleSelect(tab.id)}
            >
              {tab.label}
            </button>
          )
        })}
      </div>
      {activeTab && (
        <div
          role="tabpanel"
          id={`tabpanel-${activeTab.id}`}
          aria-labelledby={`tab-${activeTab.id}`}
          style={panelStyle}
        >
          {activeTab.content}
        </div>
      )}
    </div>
  )
}
