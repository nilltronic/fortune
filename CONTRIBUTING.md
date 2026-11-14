# Contributing a pack

Thanks for adding to the library! A pack is one `.txt` file in the `packs/` folder.

## 1. Write it

The easiest way is the [pack editor](https://nilltronic.github.io/fortune/editor.html). It shows exactly how each fortune will print, warns about characters the printer can't handle, and saves a correctly formatted file.

If you'd rather write it by hand, follow the format in the [README](README.md#the-pack-format).

## 2. Check it

- **One theme per file** (fortunes, dad jokes, kind words, trivia...). Aim for at least 10 entries.
- **Heading** of 32 characters or fewer, for example `# Kind Words`.
- **Keep entries short.** Up to about 120 characters is ideal. Anything over 9 lines tall prints a very long slip.
- **Plain ASCII.** Curly quotes and dashes are converted automatically; accents and emoji are dropped.
- **File name:** lowercase, hyphens, up to 24 characters, ends in `.txt` (`space-facts.txt`).
- **Jokes:** put the setup and the punchline in one line with `\n` between them. Add `# lucky: off` so lucky numbers don't print under a joke.

## 3. Content rules

These slips end up on paper in people's homes, classrooms and workshops.

- **Original or freely licensed only.** Don't copy from books, websites, other fortune lists or joke collections unless the licence clearly allows it. If you're not sure, write your own.
- **Family friendly.** No hate, harassment, sexual content, graphic violence, or anything aimed at a real person.
- **No personal information** and no advertising or links.
- **Take care with health and money.** Kind words are welcome; advice that could cause harm is not.

## 4. Licence

By submitting a pack you agree to release it under **CC0 1.0** (public domain), the same as the rest of the packs.

## 5. Submit it

1. In this repository, open the `packs/` folder.
2. Choose **Add file → Upload files**, drop in your `.txt`, and follow GitHub's prompts to propose the change (a pull request).
3. Give it a short description of the theme.

A maintainer will check the format and the content rules, then merge it. Once merged, it appears in the pack library automatically.

## Fixing or improving a pack

Open the pack in the editor (**Open in editor** on the library page), make your changes, download it, and upload it over the old file in a pull request.
