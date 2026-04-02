#include "WebserverHost.h"
#include "NtfyNotifier.h"

#include ".common/comm.h"

#include "CsvOperations.h"
#include "Esp.h"
#include <CSV_Parser.h>
#include <ESP8266WebServer.h>

CSV_Parser parser2("ucssuLuLcc", true, ',');
// The .csv will have data stored in the following format:
// ->   serial_no, box_id, item_name, reg_date, expiry_date, physical, condition;
// The first is an unsigned 8 bit int(s), next two are strings(s), next
// two are unsigned long(s), while physical is a boolean, so it can be
// interpret as a character
CSV_Parser physical_parser2("-s---uc-", true, ',');
// '-' means to ignore that column, thus saving memory.

void WebSetup(ESP8266WebServer *server)
{
  server->on("/", HTTP_GET, [server]() { handleRoot(server); });
  server->on("/inventory", HTTP_GET, [server]() { handleGetInventory(server); });
  server->on("/add", HTTP_POST, [server]() { handleAdd(server); });
  server->on("/delete", HTTP_POST, [server]() { handleDelete(server); });
  server->begin();
  d_SerialPrintln("Webserver started!");
}

PROGMEM const char INDEX_HTML[] = R"=====(
<!DOCTYPE html>
<html>
<head>
  <title>Inventory</title>
  <style>
    body { font-family: sans-serif; max-width: 800px; margin: 40px auto; padding: 0 20px; }
    table { width: 100%; border-collapse: collapse; margin-bottom: 40px; }
    th, td { border: 1px solid #ccc; padding: 8px; text-align: left; }
    th { background: #f4f4f4; }
    input, button, select { padding: 8px; margin: 4px 0; width: 100%; box-sizing: border-box; }
    button { background: #333; color: white; border: none; cursor: pointer; }
    button:hover { background: #555; }
    .delete { background: #c0392b; width: auto; padding: 4px 8px; }
    .delete:hover { background: #e74c3c; }
    h2 { margin-top: 40px; }
  </style>
</head>
<body>
  <h1>Inventory</h1>
  <table id="inventory-table">
    <thead>
      <tr>
        <th>Serial</th>
        <th>Box ID</th>
        <th>Item</th>
        <th>Registered</th>
        <th>Expiry</th>
        <th>Physical</th>
        <th>Condition</th>
        <th>Delete</th>
      </tr>
    </thead>
    <tbody id="inventory-body">
      <tr><td colspan="8">Loading...</td></tr>
    </tbody>
  </table>

  <h2>Add Item</h2>
  <form onsubmit="addItem(event)">
    <input type="text"           id="box_id"   placeholder="Box ID (e.g. A1)" required />
    <input type="text"           id="item"     placeholder="Item Name"         required />
    <input type="datetime-local" id="expiry"                                   required />
    <label>
      <input type="checkbox" id="physical" />
      Has Physical Box
    </label>
    <button type="submit">Add Item</button>
  </form>

  <script>
    const CONDITIONS = { 1: 'Fresh', 2: 'Expiring', 3: 'Rotting', 4: 'Error', 5: 'Unknown' };

    async function loadInventory() {
      const res  = await fetch('/inventory');
      const data = await res.json();
      const body = document.getElementById('inventory-body');
      if (!data.length) { body.innerHTML = '<tr><td colspan="8">No items</td></tr>'; return; }
      body.innerHTML = data.map(r => `
        <tr>
          <td>${r.serial}</td>
          <td>${r.box_id}</td>
          <td>${r.item}</td>
          <td>${new Date(r.reg_date * 1000).toLocaleDateString()}</td>
          <td>${new Date(r.expiry_date * 1000).toLocaleDateString()}</td>
          <td>${r.physical ? 'Yes' : 'No'}</td>
          <td>${CONDITIONS[r.condition] || 'Unknown'}</td>
          <td><button class="delete" onclick="deleteItem(${r.serial})">Delete</button></td>
        </tr>`).join('');
    }

    async function addItem(e) {
      e.preventDefault();
      const expiry = Math.floor(new Date(document.getElementById('expiry').value).getTime() / 1000);
      const body   = new URLSearchParams({
        box_id:   document.getElementById('box_id').value,
        item:     document.getElementById('item').value,
        expiry:   expiry,
        physical: document.getElementById('physical').checked ? '1' : '0'
      });
      const res = await fetch('/add', { method: 'POST', body });
      if (res.ok) { loadInventory(); e.target.reset(); }
      else alert('Failed to add item');
    }

    async function deleteItem(serial) {
      if (!confirm('Delete this item?')) return;
      const body = new URLSearchParams({ serial });
      const res  = await fetch('/delete', { method: 'POST', body });
      if (res.ok) loadInventory();
      else alert('Failed to delete item');
    }

    loadInventory();
  </script>
</body>
</html>
)=====";

void handleRoot(ESP8266WebServer *server)
{
  server->send(200, "text/html", INDEX_HTML);
}

void handleGetInventory(ESP8266WebServer *server)
{
  CSV_Parser cp(/*format*/ "ucssuLuLcc");
  if (!cp.readSDfile(INVENTORY_FILE))
  {
    server->send(500, "application/json", "[]");
    return;
  }

  uint8_t *serials = (uint8_t *)cp["serial_no"];
  char **ids = (char **)cp["box_id"];
  char **items = (char **)cp["item_name"];
  uint32_t *dateRegs = (uint32_t *)cp["reg_date"];
  uint32_t *expiries = (uint32_t *)cp["expiry_date"];
  char *physicals = (char *)cp["physical"];
  char *conditions = (char *)cp["condition"];
  int count = cp.getRowsCount();

  String json = "[";
  for (int i = 0; i < count; i++)
  {
    if (i > 0)
      json += ",";
    json += "{";
    json += "\"serial\":" + String(serials[i]) + ",";
    json += "\"box_id\":\"" + String(ids[i]) + "\",";
    json += "\"item\":\"" + String(items[i]) + "\",";
    json += "\"reg_date\":" + String(dateRegs[i]) + ",";
    json += "\"expiry_date\":" + String(expiries[i]) + ",";
    json += "\"physical\":" + String(physicals[i]) + ",";
    json += "\"condition\":" + String(conditions[i]);
    json += "}";
  }
  json += "]";
  server->send(200, "application/json", json);
}

void handleAdd(ESP8266WebServer *server)
{
  if (!server->hasArg("box_id") || !server->hasArg("item") || !server->hasArg("expiry") || !server->hasArg("physical"))
  {
    server->send(400, "text/plain", "Missing fields");
    return;
  }

  InventoryRecordLine record;
  strncpy(record.box_id, server->arg("box_id").c_str(), sizeof(record.box_id));
  strncpy(record.item_name, server->arg("item").c_str(), sizeof(record.item_name));

  record.reg_date = time(nullptr);
  record.expiry_date = server->arg("expiry").toInt();
  record.physical = server->arg("physical") == "1";
  record.condition = FoodCondition::UNKNOWN; // Unknown by default
  record.Serial_no = 0;                      // assigned in AddRecord

  AddRecord(&record);

  // reload parser so next ActiveBoxes request is fresh
  physical_parser2.readSDfile(INVENTORY_FILE);

  server->send(200, "text/plain", "Ok.");
  NtfyNotify("", const char *title, const char *message, const char *priority)
}

void handleDelete(ESP8266WebServer *server)
{
  if (!server->hasArg("serial"))
  {
    server->send(400, "text/plain", "Missing serial");
    return;
  }

  uint8_t serial = (uint8_t)server->arg("serial").toInt();

  DeleteRecord(serial);

  // reload parser so next ActiveBoxes request is fresh
  physical_parser2 = CSV_Parser(/*format*/ "-s---c", true, ',');
  physical_parser2.readSDfile(INVENTORY_FILE);

  server->send(200, "text/plain", "Ok.");
}
