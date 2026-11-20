import { dom } from './dom.js';
import { loadInventory } from './inventory.js';

export async function showTab(tabName) {
  dom.tabContents.forEach((content) => content.classList.remove('active'));
  dom.navItems.forEach((item) => item.classList.remove('active'));

  document.getElementById(`tab-${tabName}`).classList.add('active');
  dom.navItems.find((item) => item.dataset.tab === tabName)?.classList.add('active');

  if (tabName === 'list') {
    await loadInventory();
  }
}
