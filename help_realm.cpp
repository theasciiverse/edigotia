/* Asciiverse: help articles for diplomacy, trade and war.  Whales listed most
 * of these as still to be written (the TODO at the top of help_data.cpp:
 * Army, City Guard, Diplomacy, Relations, Trade, War); they describe the
 * game as it now plays.
 */

#include "help.h"
#include "globals.h"

#define _article(n) \
  cur_article = new Help_article( (n) ); \
  HELP->add_article(cur_article)

#define _type(n) \
  cur_article->type = (n)

#define _text(n) \
  cur_article->set_text( (n) )

#define _redirect(n) \
  cur_article->redirect = (n)

void init_realm_help()
{
  Help_article* cur_article;

  _article("Trade");
  _type("Game Concept");
  _text("\
Few <link=city>cities</link> can make everything they need.  The \
<c=ltgreen>Minister of Trade<c=/> (the Ministers menu) arranges standing \
deals with other cities: so many units of a <link=resources>resource</link> \
a day, bought or sold, until you cancel the deal.\n\
\n\
Press <c=magenta>B<c=/> to buy.  You see every resource that some city you \
can reach is selling, and the best price on offer.  Pick one, and you see \
each seller: how far away it is, how much it can spare a day, and its price \
in gold per 100 units.  The price already includes the <c=ltgreen>overhead\
<c=/>, the cost of getting a caravan there; nearer cities, cities on your \
roads and races that like yours cost less.  Press <c=magenta>S<c=/> to sell \
what you have to the cities that want it.\n\
\n\
Each day the caravans run: you pay for what you buy, and are paid for what \
you sell.  If you cannot pay, or have nothing to send, the deal is paused \
until you resume it.  Deals end when you go to <link=war>war</link> with \
the other city, and caravans cannot leave a city under <link=siege>siege\
</link>.  A <link=treaty>trade pact</link> halves the overhead, and steady \
trade makes a city think better of you (see <link=relations>relations\
</link>).\
");

  _article("Overhead");
  _redirect("Trade");

  _article("Diplomacy");
  _type("Game Concept");
  _text("\
The Diplomacy screen (World menu) lists every city you know of, nearest \
first, with its kingdom and what its people think of you.  Choose one to \
speak with its mayor, duke or monarch.\n\
\n\
You may propose a <link=treaty>treaty</link>, give a gift of gold, demand \
tribute, or declare <link=war>war</link>; at war, you may ask for peace.  \
Prices are always spoken plainly: a city tells you what it wants for a \
treaty or for peace, and there is no haggling.  A gift raises their \
opinion of you, less so if you gave one recently.  A demand for tribute \
works when they fear your soldiers more than they trust their own; a \
warlike city may answer a demand with war.\n\
\n\
Other cities act on their own, too.  A warlike neighbor that dislikes you \
may send envoys demanding gold, and your <link=liege>liege</link> will \
send demands of their own.\
");

  _article("Relations");
  _type("Game Concept");
  _text("\
Every city has an opinion of you, from -100 to 100.  It is really the \
opinion of its people, as Whales put it: citizens who love you will not \
march against you gladly, and citizens who hate you will cheer a war.\n\
\n\
Opinions start from how the two <link=race>races</link> feel about each \
other, warmer within your own <link=kingdom>kingdom</link>.  Each month \
they drift toward what the city has reason to feel: <link=treaty>treaties\
</link> and steady <link=trade>trade</link> warm them, war chills them, and \
a conquered <link=vassal>vassal</link> resents you.  Gifts give a quick \
boost that fades.  Breaking a pact to go to war makes every city think \
less of your word.\n\
\n\
The scale runs: Sworn Enemy, Hostile, Wary, Neutral, Cordial, Friend, Old \
Friend.\
");

  _article("Opinion");
  _redirect("Relations");

  _article("Treaty");
  _type("Game Concept");
  _text("\
Treaties last a year, and their price is gold, paid once.\n\
\n\
A <c=ltgreen>trade pact<c=/> halves the overhead on your \
<link=trade>trade</link> with that city.\n\
\n\
A <c=ltgreen>peace pact<c=/> promises a year without demands or war.  You \
can break it, but the whole world will hear of it.\n\
\n\
An <c=ltgreen>alliance<c=/> means that city will send soldiers to help if \
your city is besieged.  Only duchy seats, capitals and cities with a real \
garrison can offer one, and only to a friend.\n\
\n\
A city that dislikes you will not sign at any price.\
");

  _article("Treaties");
  _redirect("Treaty");

  _article("Favor");
  _redirect("Liege");

  _article("Liege");
  _type("Game Concept");
  _text("\
You begin as a baron, sworn to the nearest <link=duchy>duke</link> of your \
race's <link=kingdom>kingdom</link>; the duke answers to the \
<link=monarch>monarch</link>.  The Realm screen (World menu) shows your \
liege, how far you stand in their favor (-100 to 100), and what you owe.\n\
\n\
Each month you pay taxes; how much, and how strictly, depends on your race \
(Whales' notes on fealty: gnomes and rakshasa tax heavily, orcs and trolls \
hardly at all).  Paying keeps your favor; failing to costs it.  Now and then \
your liege demands gold, food, soldiers for a war, or that you stop trading \
with a city they dislike.  You may comply, do half and ask for patience, or \
refuse.  You may also ask your liege for a gift or a loan; each costs \
favor, and some races' lords give more freely than others.\n\
\n\
How your lords take a war you start among your own cities depends on your \
race: orcs expect it, elves and gnomes will not stand for it.  Fall low \
enough in the crown's favor and you will be cast out of the kingdom.\n\
\n\
Rise high enough and the monarch will name you a duke, with cities of your \
own to pay you tribute.  See <link=victory>victory</link>.\
");

  _article("Taxes to your Liege");
  _redirect("Liege");

  _article("Army");
  _type("Game Concept");
  _text("\
Soldiers are drafted from your <link=citizens>citizens</link> on the \
Military screen (World menu), and they leave their work while they serve.  \
<link=peasants>Peasants</link> become militia, spearmen (each needs 2 wood) \
or swordsmen (3 iron); <link=merchants>merchants</link> become archers (2 \
wood), who loose a volley before the lines meet; <link=burghers>burghers\
</link> become commanders, who do not fight but make every hundred \
soldiers under them fight a tenth better, up to half again as well.  Every \
soldier costs gold to recruit and wages every day.\n\
\n\
Soldiers need room: your keep holds a city guard of 20, and each \
<link=barracks>barracks</link> 50 more.  Your race matters too: orcs and \
trolls hit far harder than halflings and gnomes.\n\
\n\
To make <link=war>war</link>, send an army against a city you are at war \
with.  It carries food from your stores for the march, and marches across \
the world map (shown as <c=white>&<c=/>), faster on roads.  When it \
arrives it fights the garrison; if it wins, it lays <link=siege>siege\
</link>.  You can call an army home at any time.\
");

  _article("Military");
  _redirect("Army");

  _article("City Guard");
  _redirect("Army");

  _article("Soldiers");
  _redirect("Army");

  _article("War");
  _type("Game Concept");
  _text("\
You can declare war from the Diplomacy screen.  War on a city of another \
<link=kingdom>kingdom</link> is war on that whole kingdom; war on a city of \
your own kingdom is between you and that city alone, and your \
<link=liege>lords</link> will have something to say about it.  Your \
kingdom's wars are yours too: when your monarch goes to war, every city of \
the enemy is your enemy.\n\
\n\
The kingdoms quarrel among themselves.  Races that dislike each other, \
shared borders and a warlike nature sour them, and sooner or later one \
declares war; their armies march on each other's cities, and cities change \
hands.  Wars end in time, usually after half a year or more.\n\
\n\
Peace has a price, which the enemy names: more if they are winning, and \
they may even pay you if you have one of their cities under siege.  A war \
your kingdom is fighting is for your monarch to end, not you.  After a \
peace there is a truce of half a year.\
");

  _article("Siege");
  _type("Game Concept");
  _text("\
An army that beats a city's garrison camps outside its walls.  A besieged \
city cannot work the fields beyond its walls, trade or recruit, and it \
falls after a time: ten days for a city, twenty for a duchy seat, thirty \
for a capital, half that if its food runs out.  Soldiers raised inside, or \
allies who arrive, fight the besiegers every few days.\n\
\n\
A city you take becomes your <link=vassal>vassal</link>.  If your own city \
is besieged, your keep holds for twelve days.  Bring your armies home, raise \
soldiers to sally out, call on your allies, or buy peace.  If the keep \
falls, your reign is over.\
");

  _article("Vassal");
  _type("Game Concept");
  _text("\
A city your army conquers swears to you.  It pays you a fifth of its taxes \
every day as tribute, and a quarter of the conquering army stays to hold \
it.  Vassals resent their new master at first; a vassal that hates you and \
has few soldiers to keep it in line may rise up and return to its old \
kingdom.  Enemies may besiege your vassals, as they would any city.\
");

  _article("Vassals");
  _redirect("Vassal");

  _article("Raiders");
  _type("Game Concept");
  _text("\
Bands of raiders roam the wilds.  When one is seen heading for your city, \
you have a few days to raise soldiers.  If your soldiers win, the raiders \
scatter; if they lose, the raiders carry off a fifth of your gold and \
food.\
");

  _article("Bandits");
  _redirect("Raiders");

  _article("Victory");
  _type("Game Concept");
  _text("\
Edigotia is won by wearing the crown of your <link=kingdom>kingdom</link>.  \
Whales: \"Through military action or political maneuvering, you may \
eventually become a Duke or even the King yourself.\"\n\
\n\
<c=ltgreen>By the sword:<c=/> take your kingdom's capital.\n\
\n\
<c=ltgreen>By favor:<c=/> win the monarch's favor (70) and you will be \
named a duke.  The old monarch will not live forever; when they die without \
an heir, the court chooses a duke it trusts (favor 60) to wear the crown.\n\
\n\
<c=ltgreen>Without a kingdom:<c=/> if your race has no kingdom in this \
world, rule four cities besides your own and proclaim one.\n\
\n\
After a victory you may rule on as long as you like.  The game is lost if \
your keep falls to a <link=siege>siege</link>, or if your people are all \
gone.\
");

  _article("Defeat");
  _redirect("Victory");

  _article("Winning");
  _redirect("Victory");
}
