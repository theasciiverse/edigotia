# Changes from upstream

This is the Asciiverse packaging of [Edigotia](https://github.com/Whales/edigotia),
Whales' unfinished game of building, trade, diplomacy and war (2014 to
2015). The repository has no licence file; the game's own start screen
(`motd.txt`) says "License: http://opensource.org/licenses/GPL-3.0 (Free
to copy & modify, but your version must be too)", so `LICENSE` here is the
GNU General Public License, version 3, as that grant names it.

Upstream is the `upstream` remote. The `asciiverse` branch starts from
upstream's `master` at `9beda6f` (2015-06-23), with its full history of
898 commits.

## Making it build and keep cities

`f36859b`, one commit:

- `world.cpp`: `pow(2, crop)` and its mineral and animal twins are
  ambiguous for today's compilers; the bit masks use `1 << int(...)`.
- `game.cpp`: `Game::save_game()` never opened its file, and the
  `cities` folder did not exist, so no city was ever saved. Both are done
  now.
- `world.cpp`: `Map_seen::load_data()` filled a grid it had never sized
  (an out-of-range write); it is sized from the saved size first.
- `interface.cpp`: the load screen skipped the first city in its list
  (`index > 0`), and a loaded city was not made `GAME->city`, so the game
  played and saved an empty city instead.

## Ours

- **Folders** (`globals.cpp`, `main.cpp`; not on Windows): when `cuss/`
  is beside the program, the game reads `cuss/`, `motd.txt` and `art.txt`
  from there, wherever it is started, and keeps its world and cities in
  the player's own folder: `$XDG_STATE_HOME/edigotia` (usually
  `~/.local/state/edigotia`), or `~/Library/Application Support/edigotia`
  on macOS. Started from the source folder, it uses `save/` there as
  before. The folders are set before the game data is initialised.
- `LICENSE`: the GPL-3.0 text, as named on the start screen.

Not added: session logging (left for a later release).

## Packaging (new files)

- `asciiverse/build.sh`: builds only the game with upstream's Makefile,
  compiler, flags and curses from the environment.
- `asciiverse/package.sh`: the av archive: `edigotia`, `cuss/*.cuss`,
  `motd.txt`, `art.txt`, `LICENSE`, `README.txt`, `README.upstream.md`.
- `asciiverse/README.txt`: how to play it.
- `asciiverse/record.sh`: records `asciiverse/edigotia.cast`, the 100x30
  catalog preview, with hub's castrec.
- `.github/workflows/release.yml`: static musl Linux (x86-64, ARM64) in
  Alpine and macOS (x86-64, ARM64), on a pushed tag.
