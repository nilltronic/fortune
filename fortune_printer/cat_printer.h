// cat_printer.h - Stage 2: print a fortune on a BLE "cat printer" (service 0xAE30)
//
// Libraries needed (Library Manager):
//   - NimBLE-Arduino by h2zero (version 2.x)
//   - Adafruit GFX Library (also installs Adafruit BusIO)
//
// Protocol details come from the community reverse-engineering write-ups
// (0x51 0x78 <cmd> <dir> <lenL> <lenH> <data> <crc8> 0xFF).
// All the tunable bits are in the settings block below.

#pragma once
#include <NimBLEDevice.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <vector>
#include <strings.h>

// ---- Printer settings ----
#define SC05_MAC         "00:3e:90:49:97:01"  // SC05-3E00 (classic protocol)
#define MXW01_MAC        "48:0f:57:14:f9:d3"  // MXW01 (newer protocol)
#define PREFER_MXW01     0        // if both printers are on: 1 = use the MXW01
#define PRINT_ENERGY     0xFFFF   // SC05 darkness: raise if faint (max 0xFFFF)
#define MXW_SET_INTENSITY 1       // 1 = send a darkness command to the MXW01 first
#define MXW_INTENSITY    0xC0     // MXW01 darkness 0x00-0xFF (its own default is 0x5D)
#define MXW_ROTATE_180   0        // set to 1 if the MXW01 prints upside down
#define TOP_BLANK_LINES  16       // blank lead-in so the SC05 doesn't eat the first text
#define END_FEED_LINES   100      // paper feed after the text so it clears the tear bar (SC05 and other classic models)
#define MXW_END_FEED_LINES 20     // same, for the MXW01 (it already feeds a bit itself)
#define EXTRA_NAME_PREFIX ""      // other model's Bluetooth name (e.g. "GB05") to treat as a classic printer
#define CHUNK_DELAY_MS   8        // pause between BLE writes (raise if print is garbled)
#define LSB_FIRST        1        // set to 0 if the print comes out mirrored
#define TEXT_BIGGER      1        // 1 = 12pt serif text, 0 = smaller 9pt (more words per line)
#define EDGE_MARGIN      10       // gap between the paper edge and the corner blocks (raise if edges clip)
#define BLOCK_W          34       // the four corner blocks that frame the slip (pixels)
#define BLOCK_H          7
#define LUCKY_COUNT      6        // how many lucky numbers
#define LUCKY_MAX        59       // numbers run from 1 to this
#define SCAN_MS          5000     // longest the scan will run; it now stops as soon as the printer is heard
#define KEEP_CONNECTED   1        // 1 = stay connected between presses (faster); 0 = disconnect after each print

static const int PRINT_WIDTH = 384;                 // pixels
static const int ROW_BYTES   = PRINT_WIDTH / 8;     // 48

static NimBLEClient*               g_client = nullptr;
static NimBLERemoteCharacteristic* g_tx     = nullptr;   // AE01 (control)
static NimBLERemoteCharacteristic* g_data   = nullptr;   // AE03 (MXW01 bitmap data)
static bool          g_isMxw  = false;
static volatile bool g_gotA9  = false;   // MXW01 answered the print request
static volatile bool g_gotAA  = false;   // MXW01 says the print is complete
static volatile uint8_t g_a9Status = 0;  // 1 = rejected

// ---------- low level ----------

static uint8_t crc8(const uint8_t* d, size_t n) {
  uint8_t crc = 0;
  for (size_t i = 0; i < n; i++) {
    crc ^= d[i];
    for (int b = 0; b < 8; b++) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x07) : (uint8_t)(crc << 1);
  }
  return crc;
}

static uint8_t rev8(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

static void onPrinterNotify(NimBLERemoteCharacteristic* c, uint8_t* data, size_t len, bool isNotify) {
  Serial.print("Printer says:");
  for (size_t i = 0; i < len; i++) Serial.printf(" %02X", data[i]);
  Serial.println();
  // MXW01 replies: 22 21 <cmd> 00 <lenL> <lenH> <payload...>
  if (len >= 7 && data[0] == 0x22 && data[1] == 0x21) {
    if (data[2] == 0xA9) { g_a9Status = data[6]; g_gotA9 = true; }
    else if (data[2] == 0xAA) { g_gotAA = true; }
  }
}

static void bleWriteTo(NimBLERemoteCharacteristic* chr, const uint8_t* data, size_t len) {
  size_t chunk = (size_t)g_client->getMTU() - 3;
  if (chunk < 20) chunk = 20;
  if (chunk > 100) chunk = 100;
  for (size_t i = 0; i < len; i += chunk) {
    size_t n = len - i;
    if (n > chunk) n = chunk;
    chr->writeValue(data + i, n, false);  // write without response
    delay(CHUNK_DELAY_MS);
  }
}

static void bleWrite(const uint8_t* data, size_t len) {
  bleWriteTo(g_tx, data, len);
}

static void sendCmd(uint8_t cmd, const uint8_t* data, uint8_t len) {
  uint8_t pkt[64];
  if (len > 48) return;
  pkt[0] = 0x51;
  pkt[1] = 0x78;
  pkt[2] = cmd;
  pkt[3] = 0x00;
  pkt[4] = len;
  pkt[5] = 0x00;
  for (uint8_t i = 0; i < len; i++) pkt[6 + i] = data[i];
  pkt[6 + len] = crc8(data, len);
  pkt[7 + len] = 0xFF;
  bleWrite(pkt, 8 + len);
}

// ---------- connect / disconnect ----------

static void dumpServices() {
  Serial.println("Services on this device:");
  const std::vector<NimBLERemoteService*>& svcs = g_client->getServices(true);
  for (size_t i = 0; i < svcs.size(); i++) {
    Serial.printf("  %s\n", svcs[i]->getUUID().toString().c_str());
    const std::vector<NimBLERemoteCharacteristic*>& chars = svcs[i]->getCharacteristics(true);
    for (size_t j = 0; j < chars.size(); j++) {
      Serial.printf("    char %s\n", chars[j]->getUUID().toString().c_str());
    }
  }
}

static void printerDisconnect() {
  if (g_client) {
    g_client->disconnect();
    NimBLEDevice::deleteClient(g_client);
    g_client = nullptr;
  }
  g_tx = nullptr;
  g_data = nullptr;
}

// Other models, recognised by advertised name. GB01/GB02/GB03/GT01 are the
// names listed in the rbaron and JJJollyjim cat printer repos (classic protocol).
static bool nameIsKnownPrinter(const char* n, bool* isMxw) {
  *isMxw = false;
  if (!n || !n[0]) return false;
  if (strncasecmp(n, "MXW01", 5) == 0) { *isMxw = true; return true; }
  static const char* classic[] = {"GB01", "GB02", "GB03", "GT01"};
  for (size_t i = 0; i < sizeof(classic) / sizeof(classic[0]); i++) {
    if (strncasecmp(n, classic[i], 4) == 0) return true;
  }
  if (EXTRA_NAME_PREFIX[0] && strncasecmp(n, EXTRA_NAME_PREFIX, strlen(EXTRA_NAME_PREFIX)) == 0) return true;
  return false;
}

// Stops the scan the moment the preferred printer is heard. If only the other
// one (or a name-matched model) is heard, the main loop waits a moment longer
// in case the preferred one shows up too.
static volatile bool     g_hitPreferred = false;
static volatile uint32_t g_hitOtherAt   = 0;

class PrinterScanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* d) override {
    std::string addr = d->getAddress().toString();
    std::string nm = d->getName();
    bool m = false;
    bool preferred = (strcasecmp(addr.c_str(), PREFER_MXW01 ? MXW01_MAC : SC05_MAC) == 0);
    bool other = (strcasecmp(addr.c_str(), PREFER_MXW01 ? SC05_MAC : MXW01_MAC) == 0) ||
                 nameIsKnownPrinter(nm.c_str(), &m);
    if (preferred) {
      g_hitPreferred = true;
      NimBLEDevice::getScan()->stop();
    } else if (other && g_hitOtherAt == 0) {
      g_hitOtherAt = millis();
    }
  }
};

static bool printerConnect() {
  // Already connected from the last press? Skip scan, connect and discovery.
  if (KEEP_CONNECTED && g_client && g_client->isConnected() && g_tx) {
    Serial.println("Still connected to the printer");
    return true;
  }
  if (g_client) printerDisconnect();  // stale link: start clean

  static PrinterScanCallbacks scanCb;
  NimBLEDevice::init("");
  NimBLEScan* scan = NimBLEDevice::getScan();
  scan->setActiveScan(true);
  scan->setScanCallbacks(&scanCb, false);
  scan->clearResults();
  g_hitPreferred = false;
  g_hitOtherAt = 0;

  Serial.println("Scanning for printer...");
  uint32_t tScan = millis();
  scan->start(SCAN_MS, false);
  while (scan->isScanning()) {
    if (g_hitOtherAt && millis() - g_hitOtherAt > 600) scan->stop();
    delay(10);
  }
  Serial.printf("Scan took %lu ms\n", (unsigned long)(millis() - tScan));
  NimBLEScanResults res = scan->getResults();

  const NimBLEAdvertisedDevice* foundSc = nullptr;
  const NimBLEAdvertisedDevice* foundMx = nullptr;
  const NimBLEAdvertisedDevice* foundOther = nullptr;  // matched by name only
  bool otherIsMxw = false;
  for (int i = 0; i < res.getCount(); i++) {
    const NimBLEAdvertisedDevice* d = res.getDevice(i);
    std::string nm = d->getName();
    Serial.printf("  saw %s  %s\n", d->getAddress().toString().c_str(), nm.c_str());
    if (strcasecmp(d->getAddress().toString().c_str(), SC05_MAC) == 0)  foundSc = d;
    if (strcasecmp(d->getAddress().toString().c_str(), MXW01_MAC) == 0) foundMx = d;
    bool m = false;
    if (!foundOther && nameIsKnownPrinter(nm.c_str(), &m)) { foundOther = d; otherIsMxw = m; }
  }
  const NimBLEAdvertisedDevice* found = nullptr;
  if (PREFER_MXW01) found = foundMx ? foundMx : foundSc;
  else              found = foundSc ? foundSc : foundMx;
  g_isMxw = (found != nullptr && found == foundMx);
  if (!found && foundOther) {  // neither of your own printers is on: try any known model
    found = foundOther;
    g_isMxw = otherIsMxw;
    Serial.println("Using a printer matched by name");
  }
  if (!found) {
    Serial.println("Printer not found (is it on, and not connected to a phone?)");
    return false;
  }

  uint32_t tConn = millis();
  g_client = NimBLEDevice::createClient();
  if (!g_client->connect(found)) {
    Serial.println("Connect failed");
    printerDisconnect();
    return false;
  }
  Serial.printf("Connected in %lu ms (MTU %u)\n", (unsigned long)(millis() - tConn), (unsigned)g_client->getMTU());

  NimBLERemoteService* svc = g_client->getService("AE30");
  if (!svc) {
    Serial.println("Service AE30 not found");
    dumpServices();
    printerDisconnect();
    return false;
  }
  g_tx = svc->getCharacteristic("AE01");
  NimBLERemoteCharacteristic* rx = svc->getCharacteristic("AE02");
  if (!g_tx) {
    Serial.println("Characteristic AE01 not found");
    dumpServices();
    printerDisconnect();
    return false;
  }
  if (g_isMxw) {
    g_data = svc->getCharacteristic("AE03");
    if (!g_data) {
      Serial.println("Characteristic AE03 not found (needed for MXW01)");
      dumpServices();
      printerDisconnect();
      return false;
    }
  }
  if (rx && rx->canNotify()) rx->subscribe(true, onPrinterNotify);
  Serial.println(g_isMxw ? "Using MXW01 protocol" : "Using SC05 protocol");
  return true;
}

// ---------- text layout ----------

static String asciiOnly(const String& s) {
  String o;
  for (size_t i = 0; i < s.length(); i++) {
    unsigned char c = s[i];
    if (c >= 32 && c < 127) o += (char)c;
  }
  return o;
}

static void wrapText(const String& in, int maxChars, std::vector<String>& out) {
  String line = "";
  int i = 0, n = in.length();
  while (i < n) {
    while (i < n && in[i] == ' ') i++;
    int j = i;
    while (j < n && in[j] != ' ') j++;
    String word = in.substring(i, j);
    i = j;
    if (word.length() == 0) break;
    while ((int)word.length() > maxChars) {  // very long word: hard split
      if (line.length()) { out.push_back(line); line = ""; }
      out.push_back(word.substring(0, maxChars));
      word = word.substring(maxChars);
    }
    if (line.length() && (int)(line.length() + 1 + word.length()) > maxChars) {
      out.push_back(line);
      line = word;
    } else {
      line = line.length() ? line + " " + word : word;
    }
  }
  if (line.length()) out.push_back(line);
}

// ---------- MXW01 path ----------
// Control packets (AE01): 22 21 <cmd> 00 <lenL> <lenH> <payload> <crc8> FF
// Flow: set intensity -> A9 print request (line count) -> wait for reply ->
//       bitmap rows on AE03 -> AD flush -> wait for AA (print complete).

static void mxwCtrl(uint8_t cmd, const uint8_t* payload, uint8_t len) {
  uint8_t pkt[32];
  if (len > 20) return;
  pkt[0] = 0x22;
  pkt[1] = 0x21;
  pkt[2] = cmd;
  pkt[3] = 0x00;
  pkt[4] = len;
  pkt[5] = 0x00;
  for (uint8_t i = 0; i < len; i++) pkt[6 + i] = payload[i];
  pkt[6 + len] = crc8(payload, len);
  pkt[7 + len] = 0xFF;
  bleWriteTo(g_tx, pkt, 8 + len);
}

static bool mxwPrint(const uint8_t* canvasBuf, int height) {
  const int total = TOP_BLANK_LINES + height + MXW_END_FEED_LINES;
  size_t bytes = (size_t)total * ROW_BYTES;
  uint8_t* pk = (uint8_t*)malloc(bytes);
  if (!pk) {
    Serial.println("Out of memory for MXW01 buffer");
    return false;
  }

  // Build the packed image: LSB-first, 1 = black, with blank lead-in and tail
  for (int r = 0; r < total; r++) {
    int ur = MXW_ROTATE_180 ? (total - 1 - r) : r;
    int cr = ur - TOP_BLANK_LINES;
    for (int x = 0; x < ROW_BYTES; x++) {
      uint8_t v = 0;
      if (cr >= 0 && cr < height) {
        int ux = MXW_ROTATE_180 ? (ROW_BYTES - 1 - x) : x;
        uint8_t cb = canvasBuf[cr * ROW_BYTES + ux];  // canvas is MSB-first
        v = MXW_ROTATE_180 ? cb : rev8(cb);
      }
      pk[(size_t)r * ROW_BYTES + x] = v;
    }
  }

  g_gotA9 = false;
  g_gotAA = false;
  g_a9Status = 0;

  if (MXW_SET_INTENSITY) {
    uint8_t inten = MXW_INTENSITY;
    mxwCtrl(0xA2, &inten, 1);
    delay(100);
  }

  Serial.printf("MXW01: requesting print of %d lines\n", total);
  uint8_t a9[4] = {(uint8_t)(total & 0xFF), (uint8_t)((total >> 8) & 0xFF), 0x30, 0x00};
  mxwCtrl(0xA9, a9, 4);

  uint32_t t0 = millis();
  while (!g_gotA9 && millis() - t0 < 3000) delay(20);
  if (!g_gotA9) {
    Serial.println("MXW01: no reply to print request");
    free(pk);
    return false;
  }
  if (g_a9Status == 0x01) {
    Serial.println("MXW01: printer rejected the print request (paper? cover? battery?)");
    free(pk);
    return false;
  }

  bleWriteTo(g_data, pk, bytes);  // bitmap rows, chunked
  free(pk);

  uint8_t z = 0x00;
  mxwCtrl(0xAD, &z, 1);  // flush / start

  t0 = millis();
  while (!g_gotAA && millis() - t0 < 60000) delay(50);
  if (!g_gotAA) {
    Serial.println("MXW01: timed out waiting for print complete");
    return false;
  }
  return true;
}

// ---------- fortune slip layout ----------

static bool g_luckyNumbers = false;  // the sketch sets this from the web page option
static bool g_showTitle    = true;   // the banner, also set from the web page
static String g_heading    = "FORTUNE";  // banner text (a file can set its own with a "# Heading" line)

static int pxWidth(GFXcanvas1& c, const String& s) {
  int16_t x1, y1;
  uint16_t w, h;
  c.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return (int)w;
}

// Word wrap by measured pixel width (the serif font is proportional)
static void wrapPx(GFXcanvas1& c, const String& in, int maxPx, std::vector<String>& out) {
  String line = "";
  int i = 0, n = in.length();
  while (i < n) {
    while (i < n && in[i] == ' ') i++;
    int j = i;
    while (j < n && in[j] != ' ') j++;
    String word = in.substring(i, j);
    i = j;
    if (word.length() == 0) break;
    String trial = line.length() ? line + " " + word : word;
    if (line.length() == 0 || pxWidth(c, trial) <= maxPx) {
      line = trial;  // (a single very long word just gets its own line)
    } else {
      out.push_back(line);
      line = word;
    }
  }
  if (line.length()) out.push_back(line);
}

// Splits a fortune on the two typed characters \n (backslash, n), wraps each piece
// to the width, and turns an empty piece into a gap (so \n\n = blank line).
static void layoutText(GFXcanvas1& c, const String& text, int maxPx, std::vector<String>& out) {
  int pos = 0;
  while (true) {
    int nl = text.indexOf("\\n", pos);
    String seg = (nl < 0) ? text.substring(pos) : text.substring(pos, nl);
    seg.trim();
    if (seg.length()) wrapPx(c, seg, maxPx, out);
    else out.push_back("");
    if (nl < 0) break;
    pos = nl + 2;
  }
  while (!out.empty() && out.back().length() == 0) out.pop_back();                 // no trailing gap
  while (!out.empty() && out.front().length() == 0) out.erase(out.begin());        // no leading gap
}

// e.g. "Lucky numbers: 4  11  23  29  37  52" (distinct, sorted)
static String makeLuckyNumbers() {
  int nums[LUCKY_COUNT];
  int n = 0;
  while (n < LUCKY_COUNT) {
    int v = 1 + (int)(esp_random() % LUCKY_MAX);
    bool dup = false;
    for (int i = 0; i < n; i++) if (nums[i] == v) dup = true;
    if (!dup) nums[n++] = v;
  }
  for (int i = 1; i < n; i++) {  // tiny insertion sort
    int v = nums[i];
    int j = i - 1;
    while (j >= 0 && nums[j] > v) { nums[j + 1] = nums[j]; j--; }
    nums[j + 1] = v;
  }
  String s = "Lucky numbers: ";
  for (int i = 0; i < n; i++) {
    s += String(nums[i]);
    if (i < n - 1) s += "  ";
  }
  return s;
}

// ---------- the main entry point ----------

static bool printFortune(const String& fortuneRaw) {
  uint32_t tPress = millis();
  String fortune = asciiOnly(fortuneRaw);
  if (fortune.length() == 0) return false;

  // Layout: centred serif text framed by four corner blocks, like a fortune slip
  const GFXfont* bodyFont  = TEXT_BIGGER ? &FreeSerif12pt7b : &FreeSerif9pt7b;
  const GFXfont* smallFont = &FreeSerif9pt7b;
  const int sideMargin = EDGE_MARGIN + 20;        // keeps the text clear of the blocks
  const int maxPx      = PRINT_WIDTH - 2 * sideMargin;
  const int blockPad   = EDGE_MARGIN;

  GFXcanvas1 meas(PRINT_WIDTH, 8);                          // only used to measure text
  meas.setTextWrap(false);                        // never wrap while measuring
  meas.setFont(bodyFont);
  std::vector<String> lines;
  layoutText(meas, fortune, maxPx, lines);

  int16_t mx, my;
  uint16_t mw, mh;
  meas.getTextBounds("Ag", 0, 0, &mx, &my, &mw, &mh);
  const int ascent   = -my;                       // height of a tall letter above the baseline
  const int lineStep = bodyFont->yAdvance;

  std::vector<String> luckyLines;
  meas.setFont(smallFont);
  if (g_luckyNumbers) wrapPx(meas, makeLuckyNumbers(), maxPx, luckyLines);
  meas.getTextBounds("Ag", 0, 0, &mx, &my, &mw, &mh);
  const int smallAscent = -my;
  const int smallStep   = smallFont->yAdvance;

  // Heading: 12pt if it fits, else 9pt, else cut short
  String heading = asciiOnly(g_heading);
  heading.trim();
  if (heading.length() == 0) heading = "FORTUNE";
  const GFXfont* headFont = bodyFont;
  meas.setFont(headFont);
  if (pxWidth(meas, heading) > maxPx) {
    headFont = smallFont;
    meas.setFont(headFont);
    while (heading.length() > 1 && pxWidth(meas, heading) > maxPx) heading.remove(heading.length() - 1);
  }
  int16_t hx, hy;
  uint16_t hw, hh;
  meas.getTextBounds("Ag", 0, 0, &hx, &hy, &hw, &hh);
  const int headAscent = -hy;

  // Vertical offset of every body line (an empty line is a half-height gap)
  std::vector<int> lineOff;
  int bodyH = 0;
  for (size_t i = 0; i < lines.size(); i++) {
    lineOff.push_back(bodyH);
    bodyH += lines[i].length() ? lineStep : lineStep / 2;
  }

  int y = blockPad + BLOCK_H + 14;                // top of the content
  const int titleTop = y;
  if (g_showTitle) y += 36;
  const int textTop = y;
  y += bodyH;
  int luckyTop = 0;
  if (!luckyLines.empty()) {
    y += 8;
    luckyTop = y;
    y += (int)luckyLines.size() * smallStep;
  }
  const int height = y + 14 + BLOCK_H + blockPad;

  GFXcanvas1 canvas(PRINT_WIDTH, height);
  uint8_t* buf = canvas.getBuffer();
  if (!buf) {
    Serial.println("Out of memory for canvas");
    return false;
  }
  canvas.fillScreen(0);
  canvas.setTextWrap(false);
  canvas.setTextColor(1);

  // The four corner blocks
  const int blkL = blockPad;
  const int blkR = PRINT_WIDTH - blockPad - BLOCK_W;
  const int blkT = blockPad;
  const int blkB = height - blockPad - BLOCK_H;
  canvas.fillRect(blkL, blkT, BLOCK_W, BLOCK_H, 1);
  canvas.fillRect(blkR, blkT, BLOCK_W, BLOCK_H, 1);
  canvas.fillRect(blkL, blkB, BLOCK_W, BLOCK_H, 1);
  canvas.fillRect(blkR, blkB, BLOCK_W, BLOCK_H, 1);

  // Draw one centred line of text with its baseline at the given y
  auto drawCentred = [&](const String& str, int baseline) {
    int16_t x1, y1;
    uint16_t w, h;
    canvas.getTextBounds(str, 0, 0, &x1, &y1, &w, &h);
    canvas.setCursor((PRINT_WIDTH - (int)w) / 2 - x1, baseline);
    canvas.print(str);
  };

  if (g_showTitle) {
    canvas.setFont(headFont);
    drawCentred(heading, titleTop + headAscent);
  }
  canvas.setFont(bodyFont);
  for (size_t i = 0; i < lines.size(); i++) {
    if (lines[i].length()) drawCentred(lines[i], textTop + ascent + lineOff[i]);
  }
  if (!luckyLines.empty()) {
    canvas.setFont(smallFont);
    for (size_t i = 0; i < luckyLines.size(); i++) {
      drawCentred(luckyLines[i], luckyTop + smallAscent + (int)i * smallStep);
    }
  }

  // Connect
  if (!printerConnect()) return false;
  Serial.printf("Ready to send %lu ms after the press\n", (unsigned long)(millis() - tPress));

  if (g_isMxw) {
    bool ok = mxwPrint(buf, height);
    if (!KEEP_CONNECTED || !ok) printerDisconnect();  // keep the link only after a good print
    Serial.println(ok ? "Print done (MXW01)" : "MXW01 print failed");
    return ok;
  }

  Serial.printf("Printing %d lines...\n", height + TOP_BLANK_LINES);

  // Job setup
  uint8_t d;
  d = 0x32;  sendCmd(0xA4, &d, 1);                       // quality (200 dpi)
  const uint8_t latticeStart[11] = {0xAA, 0x55, 0x17, 0x38, 0x44, 0x5F, 0x5F, 0x5F, 0x44, 0x38, 0x2C};
  sendCmd(0xA6, latticeStart, 11);
  uint8_t energy[2] = {(uint8_t)(PRINT_ENERGY & 0xFF), (uint8_t)((PRINT_ENERGY >> 8) & 0xFF)};
  sendCmd(0xAF, energy, 2);                              // energy (darkness)
  d = 0x01;  sendCmd(0xBE, &d, 1);                       // apply energy
  d = 0x00;  sendCmd(0xA9, &d, 1);                       // update device

  // Blank lead-in, then the image rows
  uint8_t row[ROW_BYTES];
  memset(row, 0, sizeof(row));
  for (int y = 0; y < TOP_BLANK_LINES; y++) sendCmd(0xA2, row, ROW_BYTES);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < ROW_BYTES; x++) {
      uint8_t b = buf[y * ROW_BYTES + x];  // canvas is MSB-first, 1 = black
      row[x] = LSB_FIRST ? rev8(b) : b;
    }
    sendCmd(0xA2, row, ROW_BYTES);
  }

  // Feed paper, close the job
  uint8_t feed[2] = {(uint8_t)(END_FEED_LINES & 0xFF), (uint8_t)((END_FEED_LINES >> 8) & 0xFF)};
  sendCmd(0xA1, feed, 2);
  const uint8_t latticeEnd[11] = {0xAA, 0x55, 0x17, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17};
  sendCmd(0xA6, latticeEnd, 11);

  if (KEEP_CONNECTED) {
    delay(300);  // stay connected: the next press skips the scan and connect
  } else {
    delay(3000);  // let the printer finish its buffer before we drop the link
    printerDisconnect();
  }
  Serial.printf("Print done, %lu ms after the press\n", (unsigned long)(millis() - tPress));
  return true;
}
