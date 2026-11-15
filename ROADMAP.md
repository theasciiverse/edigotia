# Edigotia roadmap

Our continuation of Whales' Edigotia (see `VISION.md` for his plan and where
it comes from). This is our build of his version, so releases carry his last
date and our number: `v20150623-av.N`.

## v20150623-av.1: it builds, and cities keep

- [x] Builds with today's compilers
- [x] Saving and loading a city work (they never did)
- [x] Folders: game data beside the program, saves in the player's state
      folder

## v20150623-av.2: the world outside the walls

Trade, diplomacy and war, the three things in the game's own subtitle that
were never built.

- [x] The other cities take their turns: they eat, grow or go hungry, collect
      taxes, keep and pay garrisons, and lay out their land again as they
      change
- [x] Each city keeps its own living world beside its save
      (`cities/<name>.realm`); the generated world file is left alone
- [x] The Minister of Trade makes standing deals to buy and sell, at the
      seller's price plus overhead
- [x] Diplomacy (World menu): every city you know, its opinion of you,
      treaties at named prices (trade pact, peace pact, alliance), gifts,
      demands for tribute, war and peace, in the frame of Whales' mockup
- [x] Your liege: monthly taxes, favor, demands answered three ways, gifts
      and loans of aid, rules on war among a kingdom's own cities, all by
      race from `notes/fealty.txt`; cast out at very low favor
- [x] Soldiers (World menu, Military): militia, spearmen, swordsmen, archers
      and commanders, drafted from the classes, with gear, recruiting costs
      and wages; the keep and barracks give them room
- [x] Armies march on the world map, fight auto-battles, besiege and take
      cities; your conquests become vassals and pay tribute
- [x] Kingdoms quarrel, go to war, take each other's cities and make peace;
      your kingdom's wars are yours
- [x] Raiders come out of the wilds for your stores
- [x] An end: the crown (by conquest, by favor and succession, or by
      proclaiming a kingdom of your own), and defeat when your keep falls or
      your people are gone
- [x] Help articles for all of it, and a session log in the save folder

## v20150623-av.3: a world that answers back

- [ ] The kingdoms found new cities as they prosper
- [ ] Trade in ores and minerals, so a city without iron can arm swordsmen
- [ ] Other cities come to you: offers of pacts, requests for help against
      their enemies, alliances that join your wars
- [ ] Orcish cities fight each other, as Whales' notes say they must
- [ ] A first-day guide in the messages, and the new screens in the help
- [ ] Balance from playing it: prices, garrisons, war and siege lengths

## Later

- [ ] Your citizens' opinion of a neighbor moves their morale (Whales: they
      grow restless if you will not fight a city they hate)
- [ ] Nobles, marriage and a dynasty, Whales' "last core feature"
- [ ] Spies, roads you build, laws, religion
- [ ] The tactical siege map on the city's 9x9 grid
