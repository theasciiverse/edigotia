# Edigotia: what Whales meant it to be, and how we carry it on

Whales (the author of Cataclysm) built Edigotia in 2014 and 2015, nearly nine
hundred commits, and set it aside in June 2015 with the city and its economy
playing and everything beyond the walls still to come. Its own start screen
calls it "a game of building, trade, diplomacy and war". This file sets down
what he planned, in his words where we can, what existed when he stopped, and
what this branch built where he left no design. Sources: his development
blog (https://whalesdev.tumblr.com/, 298 posts, read 2026-11-15 through
Tumblr's read API), the notes in `notes/` and `notes/old_notes/`, the help
articles he wrote in `help_data.cpp`, and his mockups in `screenshots/`.

## Whales' vision

From "Oh hey" (https://whalesdev.tumblr.com/post/86073034776, May 2014), his
first description of the game:

- **Civilization, but one city.** "Think of it like Civilization, but you
  only control one city… kind of." Your city has tiles around it to improve
  (farms, mines, quarries, pastures), and buildings inside.
- **Fealty.** "You will (most likely) start the game in fealty to a Duke, to
  whom you will owe taxes (and occasional military support) in exchange for
  their promise of protection. Your Duke, in turn, owes taxes to a King.
  Through military action or political maneuvering, you may eventually
  become a Duke or even the King yourself."
- **Trade is crucial.** "If you're in a region that doesn't have good
  farmland... you'll need to find a neighboring city to buy food from...
  your Duke may demand you stop trading with their enemies, bandits exist
  along the way."
- **Races with their own ways.** Dwarves mine and forge, orcs fight, goblins
  owe their duke almost nothing.
- **Nobles and a dynasty.** You are a noble; marry, have heirs, or your line
  ends and the game with it.

From "More Edigotia progress" (https://whalesdev.tumblr.com/post/108429993786,
January 2015): a kingdom for each race, a capital, duchy seats and minor
cities joined by roads; caravans pay an **overhead** set by distance, roads
and the races' trading skill.

From "What Edigotia is like" (https://whalesdev.tumblr.com/post/108631710716,
January 2015), a story of a gnome named Pilute: a loan from his duke, a
caravan to a dwarven city for axes and armor, bandits on the road, selling
the weapons on to the orcs, orc guards hired for the caravan, a wedding to
the duke's daughter, and then an orcish warlord who **demands** free weapons
or war. "You could contact the Orcs you've been supplying and seek a
military alliance... You could also seek help from the Dwarves... You could
instead petition your Duke, and friendly Gnomish neighbors, for help."

From his answers to readers:

- **Explicit prices** (https://whalesdev.tumblr.com/post/109241429666): "the
  price of diplomatic agreements is explicit. AI cities will tell you 'we
  want 200 gold... to allow you to send caravans freely through our gates
  for the next year.'... there's no 'barter' button."
- **Opinion** (https://whalesdev.tumblr.com/post/109600141666): "the opinion
  rating is really your citizens' opinion of other cities", shaped by race,
  religion, laws and the nobles' doings.
- **Combat** (https://whalesdev.tumblr.com/post/117500670856): "we'll
  probably start off with auto-combat." A city is a 9x9 grid; "the center
  tile is your keep - if you lose that, your nobles are captured and you're
  in big trouble."
- **What was missing** (https://whalesdev.tumblr.com/post/118758183471, May
  2015): "Edigotia is missing some major features too, most notably actual
  interaction with other cities, but the city-building side of it is pretty
  complete!"

From "Reactive vs Proactive Gameplay"
(https://whalesdev.tumblr.com/post/111976635481): "bandits and aggressive
neighbors must be defended against... military problems have diplomatic
solutions, as well as those resting on the backs of armies or spies."

From "Here's a few mockups" (https://whalesdev.tumblr.com/post/110643995566):
"the military and diplomacy screens a while later". The mockups are in
`screenshots/`: a Military Unit screen (roster, combat strength, orders) and
a Diplomacy screen (contact, relationship, dialogue options, a liege's
demand with three answers).

His notes add the rest: `notes/fealty.txt` (what each race's lords ask and
give: taxes, troops, demands, aid, how they treat war among their own
cities), `notes/old_notes/war.txt` (how a city weighs war), and the help
articles on kingdoms, dukes and monarchs (`help_data.cpp`), including the
succession: "if the duke dies and leaves no heir, they may select a very
close friend to inherit their dynasty."

## What existed when work stopped (2015-06-23)

A generated world of kingdoms, duchies, cities and roads; your city with its
land, farms, mines, hunting camps, pastures, workshops and recipes, three
classes of citizen with morale and luxuries, taxes, wages and maintenance,
and the ministers' screens. The other cities were laid out once, at world
generation, and never took a turn. The Minister of Trade listed prices but
could not make a deal (`get_import()` and `get_export()` returned 0).
`combat.cpp` was used only for hunting; a military unit was a count; the
barracks housed nobody. The release v20150623-av.1 made it build and save.

## How this branch carries it on

The world outside the walls, in the order Whales planned it: trade first,
then diplomacy, then war, and an end to the game. `ROADMAP.md` lists the
releases.

## Choices made where Whales left no design

- **A living world per game** (2026-11-15): the world file (`world.sav`,
  `kingdoms.txt`) stays as generated; everything that changes in it (the
  other cities, kingdoms, armies, treaties, your place in the realm, the
  date) is saved beside your city as `cities/<name>.realm`. Two cities
  founded in one world each get their own history. Cities saved before this
  start their realm on the day they are loaded.
- **The other cities' turns** (2026-11-15): each day they eat, grow (a
  little slower than yours, up to half again their race's largest city),
  go hungry and lose people, collect taxes (a fifth of what each class
  earns) and pay a garrison of 1 to 7 percent of their people by race,
  plus a guard of 10, 60 or 200 for cities, duchy seats and capitals. Every
  season a city that has changed much lays out its production again. Whales'
  numbers where he had them; ours for the rest.
- **Trade deals** (2026-11-15): a deal is a standing order, so many units a
  day at the seller's price plus the route's overhead, until cancelled.
  Sellers offer what they produce beyond their needs; buyers want what they
  lack.
- **Diplomacy** (2026-11-15): three treaties, each a year long and bought
  with gold (a trade pact halves overhead; a peace pact rules out demands
  and war; an alliance brings help when you are besieged). Gifts, demands
  for tribute, war and peace, all at prices the city names. Opinion runs
  from -100 to 100 and drifts each month toward what the city has reason to
  feel. The relationship words are ours, after the mockup's "Old Friend".
  Opinion does not yet move your own citizens' morale, as Whales described.
- **Your liege** (2026-11-15): your duke is the nearest duchy seat of your
  race's kingdom. Taxes, strictness, demands, aid and the treatment of war
  among a kingdom's own cities come from `notes/fealty.txt`, cut to a small
  table per race. Demands are answered with the three replies of the mockup.
  Very low favor with the crown casts you out of the kingdom.
- **Soldiers** (2026-11-15): five kinds, after the help articles on the
  classes and the Military Unit mockup: militia, spearmen and swordsmen from
  peasants, archers from merchants, commanders from burghers. Wood and iron
  for gear, gold to recruit, wages daily (the old military expense was shown
  but never paid). The keep holds 20, each barracks 50, as the barracks data
  already said.
- **Battles** (2026-11-15): auto-combat, as Whales planned to start: a volley
  from the archers, then rounds until one side has lost half (defenders
  behind walls, two thirds) and breaks. Race combat strength and hit points
  from the race data. Walls are 130 percent for cities, 160 for duchy seats,
  200 for capitals. The 9x9 tactical siege map is not built.
- **Wars** (2026-11-15): war with a city of another kingdom is war with that
  kingdom; war with a city of your own kingdom is between the two of you, and
  your lords react by race. Your kingdom's wars are yours. Kingdoms quarrel
  over race, borders and a warlike nature, and declare war when their
  opinion of each other falls low enough; wars end after half a year or so.
  Armies march on the world map (100 travel points a day, the existing
  travel costs), besiege, and take cities. A war your kingdom drags you
  into does not bring the enemy to your own gates (your vassals are fair
  game): your lord's protection is what your taxes buy. A war you start
  yourself, or being cast out, does.
- **Sieges and conquest** (2026-11-15): a beaten city holds for 10, 20 or 30
  days (half without food); yours for 12. A city you take becomes your
  vassal, pays a fifth of its taxes as tribute, and may rise up if it hates
  you and is weakly held. Losing your keep ends the game, after Whales'
  "your nobles are captured".
- **Raiders** (2026-11-15): Whales names bandits more than once. A band now
  and then comes out of the wilds for your stores, and grows with your city.
- **Winning** (2026-11-15): the crown. By the sword (take your kingdom's
  capital), by favor (be named a duke at crown's favor 70; when the old
  monarch dies, a duke with favor 60 is chosen), or, for a race with no
  kingdom in the world, by ruling four cities besides your own. You may rule
  on after winning. Losing: the keep falls, or the city empties.
- **New towns** (2026-11-15): now and then a kingdom with a city near its
  full size sends 60 to 120 settlers to found a town within seven leagues,
  on its own or unclaimed land. They are saved with the realm.
- **Ore for sale** (2026-11-15): cities sell the ore they mine beyond their
  workshops' needs, and buy what their workshops lack, so a city with no
  hills can still smelt iron for swordsmen.
- **Feuding orcs** (2026-11-15): in a kingdom whose race expects war among
  its own (`notes/fealty.txt`: orcs), a strong city now and then marches on
  a weaker neighbor of the same kingdom. Capitals are left alone.
- **Offers** (2026-11-15): a nearby city that likes you (opinion 40) may
  offer a trade pact at half its price.
- **Allies at war** (2026-11-15): Whales' Pilute could "seek a military
  alliance". An ally with a garrison of 60 or more may send half of it
  against a city you are at war with, within 25 leagues and never of its
  own kingdom; a city it takes becomes your vassal. Allies go home when you
  make peace.
- **A fallen reign stays fallen** (2026-11-15, conservative, David may
  prefer otherwise): a lost game's save files are kept, not deleted; loading
  one shows how it ended and goes no further.

## Not built (yet)

Nobles, marriage and a dynasty (Whales' "last core feature before a v0.1"),
spies, building roads, cartels and smuggling, religion, laws, the tactical
battle map, and the shared online worlds of "Edigotia Multiplayer"
(https://whalesdev.tumblr.com/post/109529218976).
