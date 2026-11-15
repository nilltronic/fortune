# Fortune Printer: the ESP32-C6 sketch

Press a button, get a printed fortune. An ESP32-C6 picks a random line from a fortune file stored on the board and prints it on a Bluetooth ("cat") thermal printer, formatted like a fortune cookie slip.

- Fortunes are stored on the board, so printing works without internet.
- A small web page on the board lets you upload, edit and choose fortune files, set WiFi, and change a few print options.
- Fortune packs and a pack editor with an exact preview live in this repository: https://nilltronic.github.io/fortune/

Tested with two printers: the **SC05-3E00** and the **MXW01**. Other cat printers of the same family may work (see [Other printers](#other-printers)).

## What you need

- An **ESP32-C6** development board. The sketch uses the onboard BOOT button (GPIO 9) as the print button, so nothing needs wiring.
- A **Bluetooth cat printer** and paper.
- A USB cable, and the Arduino IDE with the ESP32 board package.

### A separate button (optional)

Wire a normal push button between **GPIO 9 and GND** (the sketch enables the internal pull-up). GPIO 9 is a strapping pin, so don't hold the button down while powering on or resetting the board, or it will start in download mode. To use a different pin, change `BUTTON_PIN` at the top of `fortune_printer.ino` to any free GPIO.

## Install

### 1. Libraries (Arduino Library Manager)

| Library | Notes |
| --- | --- |
| **NimBLE-Arduino** by h2zero | Version 2.x |
| **Adafruit GFX Library** | Also installs Adafruit BusIO |

WiFi, the web server, mDNS and LittleFS all come with the ESP32 board package. The sketch was developed with ESP32 core 3.3.3.

### 2. Board settings

<img width="462" height="361" alt="Screenshot 2026-10-18 060207" src="https://github.com/user-attachments/assets/0b7d62ec-7f69-4546-97ee-a79b94d93e59" />

| Setting | Value |
| --- | --- |
| Board | **ESP32C6 Dev Module** |
| Partition Scheme | **Huge APP (3MB No OTA/1MB SPIFFS)** |
| USB CDC On Boot | **Enabled** (so the Serial Monitor works over USB) |
| Core Debug Level | None |

The partition scheme matters twice. The sketch is too big for the default one (it stops with "text section exceeds available space"), and the 1 MB file area in this scheme is where your fortune files live. Changing the partition scheme later erases stored files and settings. The screenshot in the main README shows the full set of settings used.

### 3. Upload

Put `fortune_printer.ino` and `cat_printer.h` in one folder named `fortune_printer`, open the `.ino`, and upload. Open the Serial Monitor at 115200 baud to watch what it does.

## First-time setup

1. The board starts its own WiFi network called **FortunePrinter** (open, no password). Join it with your phone.
2. Open **http://192.168.4.1**.
3. Under **WiFi**, pick your home network, enter the password and press **Save and reconnect**. The board restarts and joins it.
4. From then on the page is at **http://fortune.local**. The board's own WiFi stays on as a fallback.
5. Switch the printer on and **tap the button**.

The first run creates `/fortunes/general.txt` with a set of starter fortunes.

## Using it

**Button:** tap to print. Hold for **8 seconds** to forget the saved WiFi and the admin password. (It prints when you let go, and only for a tap under 2 seconds.)

**Web page**

| Section | What it does |
| --- | --- |
| Print | Prints a fortune now. |
| Fortunes | Lists your files with their fortune counts. **Edit**, **Download** or **Delete** each. **Print from** chooses one file or "All files mixed". **Upload a .txt file** adds one. **Write a new file** opens the editor. Links go to the pack library and pack editor. |
| Fortune slip | Print the banner (the heading at the top) yes/no, and add lucky numbers yes/no. |
| WiFi | Home network settings. |
| Admin password | Optional. User name is `admin`. It protects the page, but it is sent unencrypted over plain HTTP, so treat it as a way to stop casual changes. |

### Fortune file format

A plain `.txt` file, one fortune per line:

```
# Dad Jokes
# lucky: off
Why did the chicken cross the road?\nTo get to the other side!
```

- `# Heading` on the first line sets the banner. Without it the banner says FORTUNE.
- `# lucky: off` (or `on`) overrides the lucky numbers setting for that file.
- `\n` inside a line is a line break; `\n\n` is a gap.
- Other `#` lines are comments and never print. Blank lines are ignored.
- Curly quotes and dashes become plain ones; accents and emoji are dropped (the printer font is plain ASCII).
- Uploads can be up to 100 KB. The on-device editor opens files up to 30 KB; use Download, edit, and Upload for larger ones.

The [pack library](https://nilltronic.github.io/fortune/) has ready-made packs, and the [pack editor](https://nilltronic.github.io/fortune/editor.html) previews exactly how a slip will print.

## Settings

Everything you might want to tweak is a `#define` at the top of the two files.

### `cat_printer.h` (printing)

| Setting | Default | What it does |
| --- | --- | --- |
| `SC05_MAC`, `MXW01_MAC` | the author's printers | Bluetooth addresses of your printers. Replace with your own (see below). |
| `PREFER_MXW01` | 0 | If both printers are on: 1 uses the MXW01. |
| `PRINT_ENERGY` | 0xFFFF | Darkness for the SC05 and other classic printers (max 0xFFFF). Lower it on fresh paper if prints look smudged. |
| `MXW_INTENSITY` | 0xC0 | Darkness for the MXW01 (0x00 to 0xFF). `MXW_SET_INTENSITY 0` skips sending it. |
| `MXW_ROTATE_180` | 0 | Set to 1 if the MXW01 prints upside down. |
| `TOP_BLANK_LINES` | 16 | Blank lead-in (the SC05 can lose its first line). |
| `END_FEED_LINES` / `MXW_END_FEED_LINES` | 100 / 20 | Paper feed after the slip. |
| `LSB_FIRST` | 1 | Set to 0 if prints come out mirrored. |
| `CHUNK_DELAY_MS` | 8 | Pause between Bluetooth writes. Raise it if prints are garbled. |
| `KEEP_CONNECTED` | 1 | Stay connected to the printer between presses (faster). 0 disconnects after every print. |
| `SCAN_MS` | 5000 | Longest the printer scan runs. It stops as soon as the printer is heard. |
| `TEXT_BIGGER` | 1 | 12pt text (1) or 9pt (0). |
| `EDGE_MARGIN`, `BLOCK_W`, `BLOCK_H` | 10, 34, 7 | Margin and size of the four corner blocks. Raise `EDGE_MARGIN` if the edges clip. |
| `LUCKY_COUNT`, `LUCKY_MAX` | 6, 59 | How many lucky numbers, and the highest. |
| `EXTRA_NAME_PREFIX` | "" | Bluetooth name of another model to treat as a classic printer. |

### `fortune_printer.ino` (board)

| Setting | Default | What it does |
| --- | --- | --- |
| `BUTTON_PIN` | 9 | Print button. |
| `AP_SSID` | FortunePrinter | Name of the board's own WiFi. |
| `HOST_NAME` | fortune | The `.local` name. |
| `PACKS_URL` | the pack library | Link shown on the page. Change it if you host your own. |
| `MAX_FILE_BYTES`, `EDIT_MAX_BYTES` | 100000, 30000 | Upload and editor size limits. |

## Other printers

Find your printer's address in the Serial Monitor: when you press the button the sketch lists every Bluetooth device it sees. Put the address in `SC05_MAC` (printers that behave like the SC05) or `MXW01_MAC` (printers that behave like the MXW01).

If the printer is not found by address, the sketch also recognises the names GB01, GB02, GB03, GT01 and MXW01, and anything starting with `EXTRA_NAME_PREFIX`. Those models share the protocol the SC05 uses, but I have only tested the two printers named at the top, so expect to tune darkness and margins.

## Troubleshooting

| Symptom | Try |
| --- | --- |
| Nothing in the Serial Monitor | Set **USB CDC On Boot: Enabled** and baud 115200; press the board's reset button with the monitor open. |
| "Sketch too big" | Choose the **Huge APP** partition scheme. |
| "Printer not found" | Switch the printer on, make sure no phone is connected to it, and check its address appears in the scan list. |
| Prints are faint | Raise `PRINT_ENERGY` (or `MXW_INTENSITY`). Old paper needs more. |
| Prints are smudged | Lower them. |
| Mirrored or upside down | `LSB_FIRST` / `MXW_ROTATE_180`. |
| Garbled or missing lines | Raise `CHUNK_DELAY_MS`. |
| First press after switching the printer off and on prints nothing | Press again, or set `KEEP_CONNECTED 0`. |
| `fortune.local` does not open | Use the IP address shown in the Serial Monitor, or join the FortunePrinter WiFi and use 192.168.4.1. |
| Forgot the admin password or WiFi | Hold the button for 8 seconds. |

## Notes and limits

- The board's own WiFi is open, and the page has no encryption (plain HTTP). Use the admin password to stop casual changes; don't rely on it for security.
- Starter fortunes are written to storage only when no fortune files exist.
- The page pauses for several seconds while a slip is printing.

## Credits and licences

- Starter fortunes are adapted from Josh Madison's [Fortune Cookie Fortunes](https://joshmadison.com/2008/04/20/fortune-cookie-fortunes/), licensed [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/). The list has been edited.
- The Bluetooth printing protocol builds on the community's reverse-engineering of these printers, including [rbaron/catprinter](https://github.com/rbaron/catprinter), [JJJollyjim/catprinter](https://github.com/JJJollyjim/catprinter) and the `catprinter` Rust crate. [kitty-printer](https://github.com/NaitLee/kitty-printer) was used for testing.
- Text is drawn with the Adafruit GFX Library and its bundled serif fonts, which are converted from GNU FreeFont.
- See [LICENSES.md](../LICENSES.md) for the licences of the code and packs.
