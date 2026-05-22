import { addInventoryItem } from './api.js';
import { dom } from './dom.js';
import { findPresetByItemName, presetTree, ProfileID } from './presets.js';
import { state } from './state.js';
import { loadInventory } from './inventory.js';
import { showTab } from './tabs.js';

const DEFAULT_SHELF_DAYS = 7;
const PHYSICAL_BOXES = ['A0', 'A1'];

function getNodeAtPath(path) {
  let node = presetTree;
  for (const nodeId of path) {
    node = (node.children || []).find((child) => child.id === nodeId);
    if (!node) return null;
  }
  return node;
}

function getChildrenForPath(path) {
  const node = path.length ? getNodeAtPath(path) : presetTree;
  return node?.children || [];
}

function isLeaf(node) {
  return !node?.children || node.children.length === 0;
}

function getNodeTagClass(node) {
  const profile = node?.monitoring?.profile;
  if (profile === ProfileID.EXPERIMENTAL) return 'experimental';
  if (profile && profile !== ProfileID.TIME_ONLY) return 'supported';
  return '';
}

function updateLegend() {
  const supportedLegend = dom.presetLegend.querySelector('.legend-item.supported-legend');
  const experimentalLegend = dom.presetLegend.querySelector('.legend-item.experimental-legend');
  const visibleNodes = Array.from(dom.presetTree.querySelectorAll('.preset-node.supported, .preset-node.experimental'));
  const shellRect = dom.presetTreeShell.getBoundingClientRect();
  const isVisible = (node) => {
    const rect = node.getBoundingClientRect();
    return rect.width > 0 && rect.right > shellRect.left && rect.left < shellRect.right && rect.bottom > shellRect.top && rect.top < shellRect.bottom;
  };
  const hasSupported = visibleNodes.some((n) => n.classList.contains('supported') && isVisible(n));
  const hasExperimental = visibleNodes.some((n) => n.classList.contains('experimental') && isVisible(n));

  supportedLegend.hidden = !hasSupported;
  experimentalLegend.hidden = !hasExperimental;
  dom.presetLegend.classList.toggle('visible', hasSupported || hasExperimental);
}

function makeNodeButton(node, kind = 'option') {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = `preset-node ${kind}`;

  const tagClass = getNodeTagClass(node);
  if (tagClass) button.classList.add(tagClass);
  if (isLeaf(node)) button.classList.add('leaf');

  button.dataset.nodeId = node.id;
  button.textContent = node.label;
  return button;
}

function makeRootButton() {
  const button = document.createElement('button');
  button.type = 'button';
  button.className = 'preset-node root active';
  button.dataset.nodeId = '__root__';
  button.textContent = 'Presets';
  return button;
}

function updateSelectedItemDisplay() {
  const selected = state.selectedPreset;

  if (!selected) {
    if (!dom.item.value.trim()) dom.item.placeholder = 'Type an item or browse presets';
    dom.itemHint.textContent = 'Click the item field to browse presets, or type your own item name.';
    return;
  }

  dom.item.value = selected.itemName || selected.label;
  dom.item.placeholder = 'Item name';
  dom.itemHint.textContent = `Selected preset: ${selected.label}`;
}

function applyLeafSelection(node) {
  state.selectedPreset = node;
  if (typeof node.shelfDays === 'number') {
    dom.shelf.value = String(node.shelfDays);
    updateExpiry();
  }
  updateSelectedItemDisplay();
}

function renderPresetTrail() {
  const fragments = [
    '<button type="button" class="preset-crumb root" data-preset-path-index="-1">Presets</button>',
  ];

  state.presetPath.forEach((nodeId, index) => {
    const label = getNodeAtPath(state.presetPath.slice(0, index + 1))?.label;
    if (!label) return;
    fragments.push('<span class="preset-crumb-sep" aria-hidden="true">→</span>');
    fragments.push(`<button type="button" class="preset-crumb" data-preset-path-index="${index}">${label}</button>`);
  });

  dom.presetTrail.innerHTML = fragments.join('');
}


function makeColumn(title, contentEl, classes = '') {
  const col = document.createElement('div');
  col.className = `preset-column ${classes}`.trim();
  if (title) {
    const heading = document.createElement('div');
    heading.className = 'preset-column-title';
    heading.textContent = title;
    col.appendChild(heading);
  }
  col.appendChild(contentEl);
  return col;
}

function buildRootColumn() {
  const stack = document.createElement('div');
  stack.className = 'preset-column-stack single';
  stack.appendChild(makeRootButton());
  return makeColumn('', stack, 'root-column');
}

function buildSelectedColumn(node, title) {
  const stack = document.createElement('div');
  stack.className = 'preset-column-stack single';
  stack.appendChild(makeNodeButton(node, 'active'));
  return makeColumn(title, stack, 'selected-column');
}

function buildOptionsColumn(title, options) {
  const stack = document.createElement('div');
  stack.className = 'preset-column-stack';
  options.forEach((node) => stack.appendChild(makeNodeButton(node, 'option')));
  return makeColumn(title, stack, 'options-column current-column');
}

function getCurrentColumnTitle() {
  if (state.presetPath.length === 0) return 'Choose Category';
  const currentNode = getNodeAtPath(state.presetPath);
  return currentNode?.label || 'Choose';
}


function adjustPresetHeights() {
  if (!dom.presetPicker || !dom.presetTreeShell) return;
  const stage = dom.presetTree.querySelector('.preset-stage');
  if (!stage) return;

  const expanded = state.presetExpanded || state.presetPath.length > 0;

  if (!expanded) {
    dom.presetTreeShell.style.height = '74px';
    dom.presetTreeShell.style.minHeight = '74px';
    dom.presetPicker.style.minHeight = '96px';
    return;
  }

  const columns = Array.from(stage.querySelectorAll('.preset-column'));

  let contentHeight = 0;
  columns.forEach((col) => {
    const title = col.querySelector('.preset-column-title');
    const stack = col.querySelector('.preset-column-stack');

    const titleHeight = title ? Math.ceil(title.getBoundingClientRect().height) : 0;
    const stackHeight = stack ? Math.ceil(stack.scrollHeight) : 0;
    const gap = title && stack ? 12 : 0;

    contentHeight = Math.max(contentHeight, titleHeight + gap + stackHeight);
  });

  const shellHeight = Math.max(420, contentHeight + 40);
  dom.presetTreeShell.style.height = `${shellHeight}px`;
  dom.presetTreeShell.style.minHeight = `${shellHeight}px`;

  const headerHeight = dom.presetPickerHeader
    ? Math.ceil(dom.presetPickerHeader.getBoundingClientRect().height)
    : 0;

  dom.presetPicker.style.minHeight = `${Math.max(520, shellHeight + headerHeight + 40)}px`;
}

function centerPresetStage() {
  const stage = dom.presetTree.querySelector('.preset-stage');
  if (!stage || !dom.presetTreeShell) return;

  const shellWidth = dom.presetTreeShell.clientWidth;
  const focusColumn = stage.querySelector('.current-column') || stage.querySelector('.selected-column:last-of-type') || stage.querySelector('.root-column');
  if (!focusColumn) return;

  const focusCenter = focusColumn.offsetLeft + focusColumn.offsetWidth / 2;
  const stageWidth = stage.scrollWidth;
  let delta = shellWidth / 2 - focusCenter;
  const minDelta = Math.min(0, shellWidth - stageWidth - 12);
  const maxDelta = 12;
  delta = Math.max(minDelta, Math.min(maxDelta, delta));
  stage.style.transform = `translateX(${delta}px)`;
}

function drawCurve(svg, x1, y1, x2, y2) {
  const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
  const controlOffset = Math.max(34, Math.abs(x2 - x1) * 0.4);
  path.setAttribute('d', `M ${x1} ${y1} C ${x1 + controlOffset} ${y1}, ${x2 - controlOffset} ${y2}, ${x2} ${y2}`);
  path.setAttribute('class', 'preset-connector');
  svg.appendChild(path);
}

function connectElements(treeRect, fromEl, toEl) {
  const from = fromEl.getBoundingClientRect();
  const to = toEl.getBoundingClientRect();
  drawCurve(
    dom.presetLines,
    from.right - treeRect.left,
    from.top - treeRect.top + from.height / 2,
    to.left - treeRect.left,
    to.top - treeRect.top + to.height / 2,
  );
}

function drawPresetLines() {
  const treeRect = dom.presetTree.getBoundingClientRect();
  dom.presetLines.setAttribute('width', String(treeRect.width));
  dom.presetLines.setAttribute('height', String(treeRect.height));
  dom.presetLines.setAttribute('viewBox', `0 0 ${treeRect.width} ${treeRect.height}`);
  dom.presetLines.innerHTML = '';

  const stage = dom.presetTree.querySelector('.preset-stage');
  if (!stage) return;

  const selectedCols = Array.from(stage.querySelectorAll('.selected-column .preset-node, .root-column .preset-node'));
  for (let i = 0; i < selectedCols.length - 1; i += 1) {
    connectElements(treeRect, selectedCols[i], selectedCols[i + 1]);
  }

  const lastSelected = selectedCols[selectedCols.length - 1];
  const options = Array.from(stage.querySelectorAll('.current-column .preset-node.option'));
  if (lastSelected && options.length) {
    options.forEach((node) => connectElements(treeRect, lastSelected, node));
  }
}

function syncPickerClasses() {
  dom.presetPicker.classList.toggle('expanded', state.presetExpanded || state.presetPath.length > 0);
}

export function renderPresetTree() {
  dom.presetTree.innerHTML = '';
  syncPickerClasses();

  const stage = document.createElement('div');
  stage.className = 'preset-stage';
  stage.appendChild(buildRootColumn());

  let runningPath = [];
  state.presetPath.forEach((nodeId, index) => {
    runningPath = [...runningPath, nodeId];
    const node = getNodeAtPath(runningPath);
    if (!node) return;
    const title = index === 0 ? 'Choose Category' : '';
    stage.appendChild(buildSelectedColumn(node, title));
  });

  const nextOptions = state.presetExpanded ? getChildrenForPath(state.presetPath) : [];
  if (nextOptions.length) {
    stage.appendChild(buildOptionsColumn(getCurrentColumnTitle(), nextOptions));
  }

  dom.presetTree.appendChild(stage);
  renderPresetTrail();

  requestAnimationFrame(() => {
    adjustPresetHeights();
    centerPresetStage();
    requestAnimationFrame(() => {
      adjustPresetHeights();
      centerPresetStage();
      drawPresetLines();
      updateLegend();
    });
  });
}

function resetPresetSelection() {
  state.presetPath = [];
  state.selectedPreset = null;
  state.presetExpanded = false;
  dom.item.value = '';
  dom.item.placeholder = 'Type an item or browse presets';
  dom.itemHint.textContent = 'Click the item field to browse presets, or type your own item name.';
  dom.shelf.value = String(DEFAULT_SHELF_DAYS);
  updateExpiry();
}

export function handlePresetClick(button) {
  const nodeId = button.dataset.nodeId;

  if (!nodeId) return;

  if (nodeId === '__root__') {
    state.selectedPreset = null;
    state.presetPath = [];
    state.presetExpanded = true;
    dom.item.value = '';
    renderPresetTree();
    updateSelectedItemDisplay();
    return;
  }

  const existingIndex = state.presetPath.indexOf(nodeId);
  if (existingIndex !== -1) {
    state.presetPath = state.presetPath.slice(0, existingIndex + 1);

    const selectedPathNode = getNodeAtPath(state.presetPath);
    if (selectedPathNode && isLeaf(selectedPathNode)) {
      applyLeafSelection(selectedPathNode);
    } else {
      state.selectedPreset = null;
      dom.item.value = '';
      dom.itemHint.textContent = selectedPathNode
        ? `Selected branch: ${selectedPathNode.label}`
        : 'Click the item field to browse presets, or type your own item name.';
    }

    state.presetExpanded = true;
    renderPresetTree();
    updateSelectedItemDisplay();
    return;
  }

  const options = getChildrenForPath(state.presetPath);
  const selectedNode = options.find((node) => node.id === nodeId);
  if (!selectedNode) return;

  state.presetPath = [...state.presetPath, nodeId];

  if (isLeaf(selectedNode)) {
    applyLeafSelection(selectedNode);
  } else {
    state.selectedPreset = null;
    dom.item.value = '';
    dom.itemHint.textContent = `Selected branch: ${selectedNode.label}`;
  }

  state.presetExpanded = true;
  renderPresetTree();
  updateSelectedItemDisplay();
}

export function populateBoxSelectors() {
  for (let i = 65; i <= 90; i += 1) dom.boxLetter.add(new Option(String.fromCharCode(i)));
  for (let i = 0; i <= 9; i += 1) dom.boxNumber.add(new Option(i));
}

export function updateExpiry() {
  const days = parseInt(dom.shelf.value, 10) || 0;
  const date = new Date();
  date.setDate(date.getDate() + days);
  dom.expiry.value = date.toISOString().split('T')[0];
}

function triggerFeedback(id, direction) {
  const wrap = document.getElementById(id).parentElement;
  const buttonIndex = direction > 0 ? 2 : 1;
  const span = wrap.querySelector(`button:nth-child(${buttonIndex + 1}) span`);
  const className = direction > 0 ? 'flash-plus' : 'flash-minus';

  if (span) {
    span.classList.add(className);
    setTimeout(() => span.classList.remove(className), 300);
  }
}

export function adjustValue(id, delta) {
  const input = document.getElementById(id);
  input.value = Math.max(1, parseInt(input.value, 10) + delta);
  triggerFeedback(id, delta);
  if (id === 'shelf') updateExpiry();
}

export function handleWheelAdjust(event, id) {
  event.preventDefault();
  const delta = event.deltaY < 0 ? 1 : -1;
  adjustValue(id, delta);
}

export function handleWheelLetter(event) {
  event.preventDefault();
  const step = event.deltaY < 0 ? -1 : 1;
  let next = dom.boxLetter.selectedIndex + step;
  if (next < 0) next = 25;
  if (next > 25) next = 0;
  dom.boxLetter.selectedIndex = next;
}

export function handleWheelNumber(event) {
  event.preventDefault();
  const step = event.deltaY < 0 ? -1 : 1;
  dom.boxNumber.selectedIndex = Math.max(0, Math.min(9, dom.boxNumber.selectedIndex + step));
}

function getSelectedBoxId() {
  return `${dom.boxLetter.value}${dom.boxNumber.value}`;
}

function setSelectedBoxId(boxId) {
  if (!boxId || boxId.length < 2) return;
  dom.boxLetter.value = boxId[0];
  dom.boxNumber.value = boxId.slice(1);
}

function getMonitoringMetaForRow(row) {
  const explicitProfile = row[7];
  if (explicitProfile !== undefined) {
    const profile = Number.parseInt(explicitProfile, 10);
    return {
      profile: Number.isNaN(profile) ? ProfileID.TIME_ONLY : profile,
      supported: !Number.isNaN(profile) && profile !== ProfileID.TIME_ONLY,
    };
  }

  const inferredPreset = findPresetByItemName(row[2]);
  const profile = inferredPreset?.monitoring?.profile ?? ProfileID.TIME_ONLY;
  return { profile, supported: profile !== ProfileID.TIME_ONLY };
}

function chooseRecommendedPhysicalBox() {
  const boxRows = PHYSICAL_BOXES.map((boxId) => ({
    boxId,
    row: state.inventory.find((row) => row[1] === boxId) || null,
  }));

  const emptyBox = boxRows.find(({ row }) => !row);
  if (emptyBox) return { boxId: emptyBox.boxId, reason: `${emptyBox.boxId} is free right now.` };

  const unsupportedBox = boxRows.find(({ row }) => row && !getMonitoringMetaForRow(row).supported);
  if (unsupportedBox?.row) {
    return {
      boxId: unsupportedBox.boxId,
      reason: `${unsupportedBox.boxId} currently contains ${unsupportedBox.row[2]}, which is not sensor-supported.`,
    };
  }
  return null;
}

function openRecommendationModal({ title, message, acceptLabel, declineLabel }) {
  return new Promise((resolve) => {
    dom.recommendationTitle.textContent = title;
    dom.recommendationText.textContent = message;
    dom.recommendationAccept.textContent = acceptLabel;
    dom.recommendationDecline.textContent = declineLabel;
    dom.recommendationModal.hidden = false;

    const cleanup = () => {
      dom.recommendationModal.hidden = true;
      dom.recommendationAccept.removeEventListener('click', onAccept);
      dom.recommendationDecline.removeEventListener('click', onDecline);
    };

    const onAccept = () => { cleanup(); resolve(true); };
    const onDecline = () => { cleanup(); resolve(false); };

    dom.recommendationAccept.addEventListener('click', onAccept);
    dom.recommendationDecline.addEventListener('click', onDecline);
    dom.recommendationAccept.focus();
  });
}

async function maybeRecommendPhysicalBox(selectedPreset, currentBoxId, itemName) {
  const profile = selectedPreset?.monitoring?.profile ?? ProfileID.TIME_ONLY;

  if (profile === ProfileID.TIME_ONLY && PHYSICAL_BOXES.includes(currentBoxId)) {
    const keepPhysical = await openRecommendationModal({
      title: 'Unsupported in monitored slot',
      message: `${itemName} is not supported by the monitored boxes. It would be better to keep ${currentBoxId} free for sensor-supported items. Store it there anyway?`,
      acceptLabel: 'Keep this box',
      declineLabel: 'Choose another box',
    });
    return keepPhysical ? currentBoxId : null;
  }

  if (profile === ProfileID.TIME_ONLY) return currentBoxId;
  if (PHYSICAL_BOXES.includes(currentBoxId)) return currentBoxId;

  const recommendation = chooseRecommendedPhysicalBox();
  if (!recommendation) return currentBoxId;

  const accepted = await openRecommendationModal({
    title: 'Suggested physical slot',
    message: `${itemName} is sensor-supported, and it can be monitored in ${recommendation.boxId}. ${recommendation.reason} Consider placing it there instead?`,
    acceptLabel: '✓ Move there',
    declineLabel: '✕ Keep current box',
  });

  if (accepted) {
    setSelectedBoxId(recommendation.boxId);
    return recommendation.boxId;
  }

  return currentBoxId;
}

export async function submitNewItem() {
  const item = dom.item.value.trim();
  if (!item) {
    alert('Please choose a preset or enter a custom item name.');
    return;
  }

  let boxId = getSelectedBoxId();
  boxId = await maybeRecommendPhysicalBox(state.selectedPreset, boxId, item);
  if (!boxId) return;

  const duplicate = state.inventory.some((row) => row[1] === boxId);
  if (duplicate) {
    alert(`Error: Box ${boxId} is already in use. Please delete the existing entry first.`);
    return;
  }

  const expiry = Math.floor(new Date(dom.expiry.value).getTime() / 1000);
  const physical = PHYSICAL_BOXES.includes(boxId) ? '1' : '0';
  const monitoring = state.selectedPreset?.monitoring || { profile: ProfileID.TIME_ONLY };

  await addInventoryItem({
    box_id: boxId,
    item,
    expiry,
    physical,
    quantity: dom.qty.value,
    profile: String(monitoring.profile ?? ProfileID.TIME_ONLY),
  });

  dom.item.value = '';
  resetPresetSelection();
  renderPresetTree();
  updateSelectedItemDisplay();
  await loadInventory();
  await showTab('list');
}

export function revealPresetPicker() {
  if (!dom.presetPicker.classList.contains('revealed')) {
    dom.presetPicker.classList.add('revealed');
    renderPresetTree();
  }
}

export function handleItemInput() {
  if (dom.item.value.trim()) {
    state.selectedPreset = null;
    dom.itemHint.textContent = 'Custom item mode. Type the exact name you want saved.';
  }
}


export function bindPresetTreeInteractions() {
  if (!dom.presetTree || dom.presetTree.dataset.bound === 'true') return;

  dom.presetTree.addEventListener('click', (event) => {
    const button = event.target.closest('.preset-node');
    if (!button || !dom.presetTree.contains(button)) return;
    handlePresetClick(button);
  });

  dom.presetTrail.addEventListener('click', (event) => {
    const crumb = event.target.closest('[data-preset-path-index]');
    if (!crumb) return;

    const index = Number.parseInt(crumb.dataset.presetPathIndex, 10);
    if (Number.isNaN(index)) return;

    if (index < 0) {
      handlePresetClick({ dataset: { nodeId: '__root__' } });
      return;
    }

    const nodeId = state.presetPath[index];
    if (!nodeId) return;
    handlePresetClick({ dataset: { nodeId } });
  });

  dom.presetTree.addEventListener('keydown', (event) => {
    const stage = dom.presetTree.querySelector('.preset-stage');
    if (!stage) return;

    const buttons = Array.from(stage.querySelectorAll('.preset-node'));
    const active = document.activeElement;
    const currentIndex = buttons.indexOf(active);

    if (event.key === 'ArrowRight' || event.key === 'ArrowDown') {
      const nextIndex = currentIndex === -1 ? 0 : Math.min(buttons.length - 1, currentIndex + 1);
      buttons[nextIndex]?.focus();
      event.preventDefault();
    } else if (event.key === 'ArrowLeft' || event.key === 'ArrowUp') {
      const prevIndex = currentIndex === -1 ? buttons.length - 1 : Math.max(0, currentIndex - 1);
      buttons[prevIndex]?.focus();
      event.preventDefault();
    } else if ((event.key === 'Enter' || event.key === ' ') && active?.classList.contains('preset-node')) {
      active.click();
      event.preventDefault();
    } else if (event.key === 'Escape') {
      const previousNodeId = state.presetPath[state.presetPath.length - 2];
      if (previousNodeId) {
        handlePresetClick({ dataset: { nodeId: previousNodeId } });
      } else {
        handlePresetClick({ dataset: { nodeId: '__root__' } });
      }
      event.preventDefault();
    }
  });

  dom.presetTree.dataset.bound = 'true';
}

export function initPresetTree() {
  bindPresetTreeInteractions();
  renderPresetTree();
  updateSelectedItemDisplay();
  let resizeRaf = null;
  window.addEventListener('resize', () => {
    if (resizeRaf) cancelAnimationFrame(resizeRaf);
    resizeRaf = requestAnimationFrame(() => {
      adjustPresetHeights();
      centerPresetStage();
      requestAnimationFrame(() => {
        adjustPresetHeights();
        centerPresetStage();
        drawPresetLines();
        updateLegend();
      });
    });
  });
}