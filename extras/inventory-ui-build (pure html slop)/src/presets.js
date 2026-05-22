export const ProfileID = {
  PROFILE_UNKNOWN: 0,
  FRUIT_FAST: 1,
  FRUIT_MEDIUM: 2,
  FRUIT_SLOW: 3,
  EXPERIMENTAL: 4,
  TIME_ONLY: 5,
};

const withProfile = (profile) => ({ monitoring: { profile } });
const supported = (profile) => withProfile(profile);
const unsupported = () => withProfile(ProfileID.TIME_ONLY);

export const presetTree = {
  id: 'presets',
  label: 'Presets',
  children: [
    {
      id: 'fruit',
      label: 'Fruit',
      children: [
        {
          id: 'apple',
          label: 'Apple',
          children: [
            { id: 'apple-red', label: 'Red Apple', shelfDays: 21, itemName: 'Red Apple', ...supported(ProfileID.FRUIT_MEDIUM) },
            { id: 'apple-green', label: 'Green Apple', shelfDays: 18, itemName: 'Green Apple', ...supported(ProfileID.FRUIT_MEDIUM) },
            { id: 'apple-custom', label: 'Other Apple', shelfDays: 20, itemName: 'Apple', ...supported(ProfileID.FRUIT_MEDIUM) },
          ],
        },
        {
          id: 'banana',
          label: 'Banana',
          children: [
            { id: 'banana-ripe', label: 'Ripe Banana', shelfDays: 5, itemName: 'Banana', ...supported(ProfileID.FRUIT_FAST) },
            { id: 'banana-green', label: 'Green Banana', shelfDays: 8, itemName: 'Green Banana', ...supported(ProfileID.FRUIT_FAST) },
          ],
        },
        {
          id: 'berries',
          label: 'Berries',
          children: [
            { id: 'strawberry', label: 'Strawberry', shelfDays: 4, itemName: 'Strawberry', ...supported(ProfileID.FRUIT_FAST) },
            { id: 'grape', label: 'Grape', shelfDays: 10, itemName: 'Grape', ...supported(ProfileID.FRUIT_MEDIUM) },
          ],
        },
        {
          id: 'soft-fruit',
          label: 'Soft Fruit',
          children: [
            { id: 'mango', label: 'Mango', shelfDays: 6, itemName: 'Mango', ...supported(ProfileID.FRUIT_FAST) },
            { id: 'pear', label: 'Pear', shelfDays: 9, itemName: 'Pear', ...supported(ProfileID.FRUIT_MEDIUM) },
            { id: 'avocado', label: 'Avocado', shelfDays: 5, itemName: 'Avocado', ...supported(ProfileID.EXPERIMENTAL) },
          ],
        },
      ],
    },
    {
      id: 'vegetable',
      label: 'Vegetable',
      children: [
        {
          id: 'root-veg',
          label: 'Root',
          children: [
            { id: 'potato', label: 'Potato', shelfDays: 30, itemName: 'Potato', ...unsupported() },
            { id: 'onion', label: 'Onion', shelfDays: 21, itemName: 'Onion', ...unsupported() },
          ],
        },
        {
          id: 'greens',
          label: 'Greens',
          children: [
            { id: 'spinach', label: 'Spinach', shelfDays: 5, itemName: 'Spinach', ...unsupported() },
            { id: 'lettuce', label: 'Lettuce', shelfDays: 7, itemName: 'Lettuce', ...unsupported() },
          ],
        },
        {
          id: 'other-veg',
          label: 'Other Vegetable',
          children: [
            { id: 'tomato', label: 'Tomato', shelfDays: 10, itemName: 'Tomato', ...supported(ProfileID.FRUIT_MEDIUM) },
            { id: 'cucumber', label: 'Cucumber', shelfDays: 7, itemName: 'Cucumber', ...unsupported() },
            { id: 'pepper', label: 'Bell Pepper', shelfDays: 8, itemName: 'Bell Pepper', ...unsupported() },
          ],
        },
      ],
    },
    {
      id: 'custom-item', 
      label: 'Custom Item',
      shelfDays: 7,
      itemName: '',
      isCustom: true,
      ...unsupported() 
      },
  ],
};

function collectLeaves(node, leaves = []) {
  if (!node) return leaves;
  if (!node.children || node.children.length === 0) {
    leaves.push(node);
    return leaves;
  }

  node.children.forEach((child) => collectLeaves(child, leaves));
  return leaves;
}

export const presetLeaves = collectLeaves(presetTree);

export function findPresetByItemName(name) {
  const normalized = String(name || '').trim().toLowerCase();
  if (!normalized) return null;

  return presetLeaves.find((leaf) => {
    const candidates = [leaf.itemName, leaf.label].filter(Boolean).map((value) => value.trim().toLowerCase());
    return candidates.includes(normalized);
  }) || null;
}
