# Cat Printer Fortune Teller

Press a button an the esp32 to receive a printed fortune on the cat printer.

ToDo:

Upload sketch

Add site link in the web ui of the esp sketch

Add the editing tool

My arduino ide settings for esp32 c6

<img width="462" height="361" alt="Screenshot 2026-10-18 060207" src="https://github.com/user-attachments/assets/0b7d62ec-7f69-4546-97ee-a79b94d93e59" />

Added fortunes from
https://joshmadison.com/2008/04/20/fortune-cookie-fortunes/
they have been edited slightly, but I have tried to keep the feel of the originals.

# Fortune Printer Packs

Fortunes, jokes and kind words for the **Fortune Printer**: an ESP32 button that prints a random fortune on a Bluetooth cat printer.

- **Pack library:** https://nilltronic.github.io/fortune/
- **Pack editor** (with an exact preview of the printed slip): https://nilltronic.github.io/fortune/editor.html

All packs are shared under **CC0** (public domain). Use them, change them, share them.

## Using a pack

1. Open the pack library and press **Download** on a pack (or **Open in editor** to tweak it first).
2. Open your Fortune Printer's page (`http://fortune.local`) and, under **Upload a .txt file**, choose the downloaded file.
3. Choose it under **Print from**, or leave that on "All files mixed".

If your phone is joined to the printer's own "FortunePrinter" WiFi it may not have internet. Download on home WiFi or mobile data, then reconnect to upload.

## The pack format

A pack is a plain `.txt` file, one fortune per line:

```
# Dad Jokes
# lucky: off
Why did the chicken cross the road?\nTo get to the other side!
I used to hate facial hair...\nbut then it grew on me.
```

| Rule | Meaning |
| --- | --- |
| `# Heading` on the first line | The banner printed at the top of each slip (up to 32 characters). Leave it out to print `FORTUNE`. |
| `# lucky: off` (or `on`) | Never (or always) add lucky numbers for this file. Leave it out to follow the device's own setting. |
| `\n` inside a line | A line break. `\n\n` gives a gap. |
| Any other line starting with `#` | A comment. Never printed. |
| Everything else | One fortune per line. Blank lines are ignored. |

Plain ASCII prints best. The device turns curly quotes and dashes into plain ones and drops accents and emoji.
File names should be lowercase with hyphens, up to 24 characters, for example `dad-jokes.txt`.

## Adding a pack

See [CONTRIBUTING.md](CONTRIBUTING.md). The short version: write it in the [editor](https://nilltronic.github.io/fortune/editor.html), download the file, and add it to the `packs/` folder with a pull request.

## How this repo is laid out

```
index.html        the pack library page (lists everything in packs/)
editor.html       the pack editor (single file, works offline too)
packs/*.txt       the packs
CONTRIBUTING.md   how to add a pack
```

The library page finds packs by itself, so adding a `.txt` file to `packs/` is all it takes for a new pack to appear.

## Publishing the site (GitHub Pages)

In the repository: **Settings → Pages → Build and deployment → Deploy from a branch**, pick `main` and `/ (root)`. After a minute the site is live at `https://<your-user>.github.io/<repo>/`. The pages work out the repository name from their own address, so nothing needs editing.

## Older printer firmware

Earlier versions of the Fortune Printer sketch fetched a single `fortunes.txt` from the top of this repo. If you still have one of those, keep that file in the repo root.
