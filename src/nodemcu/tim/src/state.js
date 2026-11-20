export const state = {
  inventory: [],
  presetPath: [],
  selectedPreset: null,
  inventoryQuery: '',
  inventorySearchField: 'item',
  inventoryConditionFilter: 'all',
  inventorySearchExpanded: false,
};

export const conditions = {
  '0': 'Fresh',
  '1': 'Going Bad',
  '2': 'Expired',
  '3': 'Unknown',
};
