Edigotia
========

Whales' unfinished game of building, trade, diplomacy and war (2014 to
2015). Generate a world of kingdoms (humans, elves, dwarves, orcs,
goblins and more), found a city of your race in it and grow it: farms,
hunting camps, pastures, mines and housing, peasants, merchants and
burghers, taxes and morale, ministers for finance, food, hunting,
livestock, mines and trade. Whales finished the city and its economy;
this build carries on with the rest of what its title promises: the other
cities grow and keep their garrisons, you trade with them, sign treaties,
answer to your liege, raise soldiers, march armies across the map, besiege
and take cities, and win the crown of your kingdom, or lose your keep.

This build is the Asciiverse packaging of github.com/Whales/edigotia, from
github.com/theasciiverse/edigotia; PATCHES.md there lists every change.
Edigotia is under the GNU General Public License, version 3: Whales says
so on its own start screen (motd.txt), and LICENSE is that licence's text.

Running it
----------

Start ./edigotia in a terminal of at least 80x24. It reads its screens
from cuss/ beside the program. Your world and cities are saved in
$XDG_STATE_HOME/edigotia (usually ~/.local/state/edigotia), or in
~/Library/Application Support/edigotia on macOS.

On the first screen: G generates a world, N founds a new city in it, L
loads a saved city, H opens the help. In the city, the number keys open
the menus (1: Game, with Save & Quit), A builds an area, . advances a day
and > a week (something urgent stops the week early).

The world beyond your walls:

- 2 then 7, the Minister of Trade: B buys, S sells, by standing deals.
- 4 then 2, Diplomacy: speak with any city you know; treaties, gifts,
  demands, war and peace.
- 4 then 3, Military: recruit soldiers, send armies, call them home.
- 4 then 4, the Realm: your liege, taxes, demands, aid, the kingdoms.

You win by wearing your kingdom's crown: take its capital, or rise to duke
in the crown's favor and be chosen when the old monarch dies. The help
(H, or 5) has articles on Trade, Diplomacy, Army, War, Siege and Victory.

Each city keeps its living world beside its save, in cities/<name>.realm.
A session log is written to session.log in the save folder (more detail
with EDIGOTIA_LOG=debug).
