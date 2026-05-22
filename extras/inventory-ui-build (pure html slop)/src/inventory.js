import { deleteInventoryItem, fetchInventoryText } from './api.js';
import { dom } from './dom.js';
import { conditions, state } from './state.js';

function toDateString(unixSeconds) {
  return new Date(Number(unixSeconds) * 1000).toLocaleDateString();
}

function matchesSearch(row) {
  const query = state.inventoryQuery.trim().toLowerCase();
  if (!query) return true;

  const fieldValue = state.inventorySearchField === 'box'
    ? String(row[1] ?? '')
    : String(row[2] ?? '');

  return fieldValue.toLowerCase().includes(query);
}

function matchesCondition(row) {
  if (state.inventoryConditionFilter === 'all') return true;
  return String(row[6] ?? '3').trim() === state.inventoryConditionFilter;
}

function getFilteredInventory() {
  return state.inventory.filter((row) => matchesSearch(row) && matchesCondition(row));
}

export function renderInventory() {
  const filtered = getFilteredInventory();

  if (!filtered.length) {
    dom.tbody.innerHTML = `
      <tr>
        <td colspan="6" class="inventory-empty">No matching inventory items.</td>
      </tr>
    `;
    return;
  }

  dom.tbody.innerHTML = filtered.map((row) => {
    const regDate = toDateString(row[3]);
    const expDate = toDateString(row[4]);
    const condKey = row[6]?.trim() || '3';

    return `
      <tr>
        <td>${row[1]}</td>
        <td>${row[2]}</td>
        <td>${regDate}</td>
        <td>${expDate}</td>
        <td class="cond-${condKey}">${conditions[condKey]}</td>
        <td style="text-align:right">
          <button class="trash-btn" type="button" data-delete-box="${row[1]}" aria-label="Delete ${row[2]} from ${row[1]}">🗑️</button>
        </td>
      </tr>
    `;
  }).join('');
}

export function setInventorySearchExpanded(expanded) {
  state.inventorySearchExpanded = expanded;
  dom.inventorySearchShell?.classList.toggle('expanded', expanded);
  dom.inventorySearchToggle?.setAttribute('aria-expanded', String(expanded));

  if (!expanded && dom.inventorySearchShell) {
    dom.inventorySearchShell.classList.add('closing');
    window.setTimeout(() => dom.inventorySearchShell?.classList.remove('closing'), 340);
  }

  if (expanded) {
    requestAnimationFrame(() => dom.inventorySearchInput?.focus());
  }
}

export function toggleInventorySearch() {
  const next = !state.inventorySearchExpanded;
  setInventorySearchExpanded(next);

  if (!next && !state.inventoryQuery) {
    dom.inventorySearchInput.value = '';
  }
}

export function setInventoryQuery(query) {
  state.inventoryQuery = query;
  renderInventory();
}

export function setInventorySearchField(field) {
  state.inventorySearchField = field;
  renderInventory();
}

export function setInventoryConditionFilter(value) {
  state.inventoryConditionFilter = value;

  dom.inventoryFilterButtons.forEach((button) => {
    button.classList.toggle('active', button.dataset.conditionFilter === value);
  });

  renderInventory();
}

export async function loadInventory() {
  try {
    const text = await fetchInventoryText();
    state.inventory = text
      .trim()
      .split('\n')
      .filter(Boolean)
      .map((line) => line.split(','));
    renderInventory();
  } catch (error) {
    console.error('Failed to load inventory:', error);
  }
}

export async function handleDelete(boxId) {
  if (!confirm(`Remove Box ${boxId}?`)) return;
  await deleteInventoryItem(boxId);
  await loadInventory();
}
