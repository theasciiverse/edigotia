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

## Diplomacy and war (v20150623-av.2 on)

Not patches but a continuation: the parts of the game Whales planned and
never built. `VISION.md` says what he meant and what was chosen without
him; `ROADMAP.md` lists the releases. New files: `realm.h/.cpp` (the living
world: other cities' turns, relations, treaties, trade deals, the liege,
kingdom wars, armies and sieges, the end of the game), `army.h/.cpp`
(soldiers and battles), `interface_realm.cpp` (the Trade, Diplomacy,
Military and Realm screens), `help_realm.cpp` (their help articles),
`log.h/.cpp` (the session log). Changes to Whales' files:

- `ai_city.*`: the other cities take turns (`do_turn()`), keep a garrison,
  replan their production, and save their state.
- `game.*`: time passes a day at a time for the whole world; each city's
  realm is saved beside it, and the world file is no longer rewritten on
  save; a loaded city is put back on the world map.
- `player_city.*`: imports and exports come from the realm's deals;
  soldiers by type, recruiting and disbanding; soldiers' wages are paid.
- `military.*`: a unit has a type.
- `building_data.cpp`: the keep has room for a city guard of 20.
- `world.*`: armies are drawn on the world map; routes for armies; a
  faster city lookup.
- `interface.*`: the World menu's new screens; the old trade screen is
  replaced; the finance screen counts trade.
- `help_data.cpp`, `init.cpp`, `main.cpp`: hooks for the above.
- Session logging: `session.log` in the save folder.

Bugs in Whales' code found along the way (v20150623-av.4):

- `world.cpp`: `World_map::set_size()` checked the unset old size instead
  of the new one, and could refuse to size the map before loading it; a
  small world was read back as a medium one.
- `player_city.cpp`: `get_daily_birth_points()` wrote one past the end of
  an array on the stack every day.

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
