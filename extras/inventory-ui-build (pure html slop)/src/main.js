import { initBackground } from './background.js';
import { dom } from './dom.js';
import {
  adjustValue,
  handlePresetClick,
  handleWheelAdjust,
  handleWheelLetter,
  handleWheelNumber,
  initPresetTree,
  populateBoxSelectors,
  submitNewItem,
  updateExpiry,
} from './form.js';
import {
  handleDelete,
  loadInventory,
  setInventoryConditionFilter,
  setInventoryQuery,
  setInventorySearchExpanded,
  setInventorySearchField,
  toggleInventorySearch,
} from './inventory.js';
import { showTab } from './tabs.js';

function bindEvents() {
  dom.navItems.forEach((item) => {
    item.addEventListener('click', () => {
      showTab(item.dataset.tab);
    });
  });

  dom.adjustButtons.forEach((button) => {
    button.addEventListener('click', () => {
      adjustValue(button.dataset.adjustTarget, Number(button.dataset.adjustDelta));
    });
  });

  dom.wheelWraps.forEach((wrap) => {
    wrap.addEventListener('wheel', (event) => handleWheelAdjust(event, wrap.dataset.wheelAdjust), { passive: false });
  });

  dom.boxLetter.addEventListener('wheel', handleWheelLetter, { passive: false });
  dom.boxNumber.addEventListener('wheel', handleWheelNumber, { passive: false });
  dom.shelf.addEventListener('input', updateExpiry);
  dom.submitAdd.addEventListener('click', submitNewItem);

  dom.tbody.addEventListener('click', (event) => {
    const button = event.target.closest('[data-delete-box]');
    if (button) {
      handleDelete(button.dataset.deleteBox);
    }
  });

  dom.presetTree.addEventListener('click', (event) => {
    const button = event.target.closest('.preset-node');
    if (button) {
      handlePresetClick(button);
    }
  });

  dom.inventorySearchToggle?.addEventListener('click', () => {
    toggleInventorySearch();
  });

  dom.inventorySearchInput?.addEventListener('input', (event) => {
    setInventoryQuery(event.target.value);
  });

  dom.inventorySearchInput?.addEventListener('keydown', (event) => {
    if (event.key === 'Escape') {
      event.preventDefault();
      event.target.value = '';
      setInventoryQuery('');
      setInventorySearchExpanded(false);
      dom.inventorySearchToggle?.focus();
    }
  });

  dom.inventorySearchField?.addEventListener('change', (event) => {
    setInventorySearchField(event.target.value);
  });

  dom.inventoryFilterButtons.forEach((button) => {
    button.addEventListener('click', () => {
      setInventoryConditionFilter(button.dataset.conditionFilter);
    });
  });
}

async function init() {
  if (dom.recommendationModal) dom.recommendationModal.hidden = true;
  populateBoxSelectors();
  bindEvents();
  initPresetTree();
  updateExpiry();
  initBackground();
  setInventoryConditionFilter('all');
  await loadInventory();
}

init();
