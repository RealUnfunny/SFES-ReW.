export async function fetchInventoryText() {
  const response = await fetch('/inventory');
  return response.text();
}

export async function addInventoryItem(payload) {
  return fetch('/add', {
    method: 'POST',
    body: new URLSearchParams(payload),
  });
}

export async function deleteInventoryItem(boxId) {
  return fetch('/delete', {
    method: 'POST',
    body: new URLSearchParams({ box_id: boxId }),
  });
}
