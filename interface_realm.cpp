/* Asciiverse: the screens for the world outside the walls, modelled on
 * Whales' mockups in screenshots/ (dialogue.png, unit.png): the Minister of
 * Trade's deals, diplomacy with the other cities, your soldiers and armies,
 * and your place in the realm.
 */

#include "interface.h"
#include "player_city.h"
#include "ai_city.h"
#include "realm.h"
#include "kingdom.h"
#include "world.h"
#include "game.h"
#include "globals.h"
#include "window.h"
#include "stringfunc.h"
#include "rng.h"
#include <sstream>
#include <algorithm>

// Pads or cuts a tagged string to a visible width.
static std::string fit(std::string text, int width)
{
  int len = tagless_length(text);
  if (len > width) {
// Cut tags-free strings only; tagged ones are kept short by their callers.
    if (len == text.length()) {
      return text.substr(0, width);
    }
    return text;
  }
  while (len < width) {
    text += " ";
    len++;
  }
  return text;
}

static std::string num(int n)
{
  return itos(n);
}

// Soldiers, rounded the way a scout would report them.
static std::string about(int n)
{
  if (n < 10) {
    return "a handful of";
  }
  if (n < 100) {
    return "about " + itos(((n + 5) / 10) * 10);
  }
  return "about " + itos(((n + 25) / 50) * 50);
}

static std::string kingdom_name(Kingdom* kingdom)
{
  if (!kingdom) {
    return "no kingdom";
  }
  return capitalize(Race_data[kingdom->race]->plural_name);
}

int Interface::list_picker(std::string title, std::string header,
                           std::vector<std::string> rows, std::string help,
                           std::string keys, long& key, int start)
{
  Window w(0, 0, 80, 24);
  int sel = start;
  int top = 0;
  const int visible = 18;
  while (true) {
    if (sel >= (int)rows.size()) {
      sel = rows.size() - 1;
    }
    if (sel < 0) {
      sel = 0;
    }
    if (sel < top) {
      top = sel;
    }
    if (sel >= top + visible) {
      top = sel - visible + 1;
    }
    w.clear();
    w.outline();
    w.putstr(2, 0, c_ltblue, c_black, title);
    w.putstr(1, 1, c_yellow, c_black, header);
    if (rows.empty()) {
      w.putstr(2, 3, c_dkgray, c_black, "(nothing here)");
    }
    for (int i = top; i < rows.size() && i < top + visible; i++) {
      int y = 2 + i - top;
      if (i == sel) {
        w.putch(1, y, c_ltgreen, c_black, '>');
      }
      w.putstr(2, y, (i == sel ? c_white : c_ltgray), c_black, rows[i]);
    }
    if (top > 0) {
      w.putch(78, 2, c_ltgray, c_black, '^');
    }
    if (top + visible < rows.size()) {
      w.putch(78, 2 + visible - 1, c_ltgray, c_black, 'v');
    }
    w.putstr(1, 21, c_magenta, c_black, help);
    w.refresh();
    long ch = input();
    if (ch == KEY_UP || ch == '8') {
      sel--;
    } else if (ch == KEY_DOWN || ch == '2') {
      sel++;
    } else if (ch == KEY_PPAGE) {
      sel -= visible;
    } else if (ch == KEY_NPAGE) {
      sel += visible;
    } else if (ch == '\n') {
      key = '\n';
      return (rows.empty() ? -1 : sel);
    } else if (ch == KEY_ESC || ch == 'q' || ch == 'Q') {
      key = KEY_ESC;
      return -1;
    } else if (ch < 256 && keys.find(char(ch)) != std::string::npos) {
      key = ch;
      return (rows.empty() ? -1 : sel);
    }
  }
}

// *** The Minister of Trade ***

/* Whales' minister listed what other cities sold but could not yet make a
 * deal.  Now a deal is a standing order: so much a day, at the seller's price
 * plus the caravan's overhead, until you cancel it.
 */
void Interface::minister_trade()
{
  Realm* realm = GAME->realm;
  int start = 0;
  while (true) {
    std::vector<std::string> rows;
    for (int i = 0; i < realm->deals.size(); i++) {
      Trade_deal& deal = realm->deals[i];
      City* city = GAME->world->lookup_city_uid(deal.city_uid);
      std::stringstream ss;
      ss << (deal.importing ? "<c=ltgreen>Buy <c=/>" : "<c=yellow>Sell<c=/>") <<
            " " << fit(num(deal.amount), 5) << " " <<
            fit(capitalize(realm->deal_goods(deal)), 16) << " " <<
            (deal.importing ? "from " : "to   ") <<
            fit(city ? city->get_name() : std::string("?"), 16) << " " <<
            fit(num(deal.price) + "/100", 8) << " " <<
            fit(num((deal.amount * deal.price + 50) / 100) + " gold/day", 14);
      if (deal.suspended) {
        ss << "<c=red>paused<c=/>";
      }
      rows.push_back(ss.str());
    }
    std::stringstream title;
    title << "Minister of Trade - income " << realm->get_trade_income() <<
             " gold/day, purchases " << realm->get_trade_expense() <<
             " gold/day";
    long key;
    int index = list_picker(title.str(),
                            "     Amount Resource         With                Price    Daily",
                            rows,
"B: Buy  S: Sell  C: Cancel a deal  R: Resume a paused one  Esc/Q: Leave",
                            "bBsScCrR", key, start);
    start = (index >= 0 ? index : 0);
    if (key == KEY_ESC) {
      return;
    } else if (key == 'b' || key == 'B') {
      trade_new(true);
    } else if (key == 's' || key == 'S') {
      trade_new(false);
    } else if ((key == 'c' || key == 'C') && index >= 0) {
      if (query_yn("Cancel this deal?")) {
        realm->cancel_deal(index);
      }
    } else if ((key == 'r' || key == 'R') && index >= 0) {
      Trade_deal& deal = realm->deals[index];
      AI_city* city = realm->lookup_city(deal.city_uid);
      if (city && realm->at_war_with_player(city)) {
        popup("You can't trade with a city you are at war with.");
      } else {
        deal.suspended = false;
      }
    }
  }
}

// A resource or an ore, for the trade screens.
struct Trade_good
{
  Resource res;
  Mineral min;
  std::string name()
  {
    return (min != MINERAL_NULL ? Mineral_data[min]->name :
                                  Resource_data[res]->name);
  }
};

// How much a city offers (buying) or wants (selling) of a good each day.
static int good_amount(AI_city* city, Trade_good good, bool buying)
{
  if (good.min != MINERAL_NULL) {
    return (buying ? city->get_mineral_surplus(good.min) :
                     city->get_daily_demand(good.min));
  }
  return (buying ? city->get_net_resource_production(good.res) :
                   city->get_daily_demand(good.res));
}

// The price per 100 we pay (buying) or are paid (selling), overhead counted.
static int good_price(AI_city* city, Trade_good good, bool buying)
{
  int base = (good.min != MINERAL_NULL ? city->get_price(good.min) :
                                         city->get_price(good.res));
  int overhead = GAME->realm->route_overhead(city);
  int price = (buying ? base + overhead : base - overhead);
  return (price < 1 ? 1 : price);
}

static int player_has(Player_city* city, Trade_good good)
{
  return (good.min != MINERAL_NULL ? city->get_mineral_amount(good.min) :
                                     city->get_resource_amount(good.res));
}

void Interface::trade_new(bool buying)
{
  Realm* realm = GAME->realm;
// Every good: resources first, then ores.
  std::vector<Trade_good> all;
  for (int i = 1; i < RES_MAX; i++) {
    Resource res = Resource(i);
    if (!Resource_data[res]->meta && res != RES_GOLD) {
      Trade_good good = { res, MINERAL_NULL };
      all.push_back(good);
    }
  }
  for (int i = 1; i < MINERAL_MAX; i++) {
    Trade_good good = { RES_NULL, Mineral(i) };
    all.push_back(good);
  }
// The cities we may trade with.
  std::vector<AI_city*> open;
  for (std::map<int,Trade_route>::iterator it = pl_city->trade_routes.begin();
       it != pl_city->trade_routes.end(); it++) {
    AI_city* city = realm->lookup_city(it->first);
    if (city && !realm->at_war_with_player(city) &&
        !realm->relation(city).vassal) {
      open.push_back(city);
    }
  }

  std::vector<Trade_good> goods;
  std::vector<std::string> rows;
  for (int g = 0; g < all.size(); g++) {
    Trade_good good = all[g];
    if (!buying && player_has(pl_city, good) <= 0) {
      continue;
    }
    int partners = 0, best = -1;
    for (int i = 0; i < open.size(); i++) {
      if (good_amount(open[i], good, buying) <= 0) {
        continue;
      }
      int price = good_price(open[i], good, buying);
      if (best == -1 || (buying ? price < best : price > best)) {
        best = price;
      }
      partners++;
    }
    if (partners == 0) {
      continue;
    }
    std::stringstream ss;
    ss << fit(capitalize(good.name()), 20) << " " << fit(num(partners), 8) <<
          " " << fit(num(best) + "/100", 10);
    if (!buying) {
      ss << " you have " << player_has(pl_city, good);
    }
    rows.push_back(ss.str());
    goods.push_back(good);
  }
  long key;
  int index = list_picker(buying ? "Buy: what do you need?" :
                                   "Sell: what will you offer?",
                          buying ?
 " Resource             Sellers  Best price (gold per 100, overhead included)" :
 " Resource             Buyers   Best offer (gold per 100, after overhead)",
                          rows, "Enter: Choose  Esc/Q: Back", "", key);
  if (index < 0) {
    return;
  }
  Trade_good good = goods[index];

// Then, with whom.
  std::vector<AI_city*> partners;
  std::vector<int> prices, amounts;
  for (int i = 0; i < open.size(); i++) {
    AI_city* city = open[i];
    int amount = good_amount(city, good, buying);
// Less what we already trade with them.
    for (int n = 0; n < realm->deals.size(); n++) {
      Trade_deal& deal = realm->deals[n];
      if (deal.city_uid == city->uid && deal.importing == buying &&
          deal.mineral == good.min &&
          (good.min != MINERAL_NULL || deal.resource == good.res)) {
        amount -= deal.amount;
      }
    }
    if (amount <= 0) {
      continue;
    }
    partners.push_back(city);
    prices.push_back(good_price(city, good, buying));
    amounts.push_back(amount);
  }
// Cheapest (or best paying) first.
  for (int i = 0; i < partners.size(); i++) {
    for (int j = i + 1; j < partners.size(); j++) {
      if ((buying && prices[j] < prices[i]) ||
          (!buying && prices[j] > prices[i])) {
        std::swap(partners[i], partners[j]);
        std::swap(prices[i], prices[j]);
        std::swap(amounts[i], amounts[j]);
      }
    }
  }
  rows.clear();
  for (int i = 0; i < partners.size(); i++) {
    AI_city* city = partners[i];
    City_relation& rel = realm->relation(city);
    int days = realm->trade_days(city);
    std::stringstream ss;
    ss << fit(city->get_name(), 18) << " " <<
          fit(capitalize(Race_data[city->get_race()]->name), 9) << " " <<
          fit(num(days) + (days == 1 ? " day" : " days"), 8) << " " <<
          fit(num(amounts[i]) + "/day", 9) << " " <<
          fit(num(prices[i]) + "/100", 9) << " <c=" <<
          color_tag(realm->relationship_color(rel.opinion)) << ">" <<
          realm->relationship_name(rel.opinion) << "<c=/>";
    if (rel.has_treaty(TREATY_TRADE)) {
      ss << " <c=ltblue>pact<c=/>";
    }
    rows.push_back(ss.str());
  }
  std::string what = good.name();
  index = list_picker((buying ? "Buy " : "Sell ") + what + (buying ?
                      ": who will sell it?" : ": who will buy it?"),
                      buying ?
 " City               Race      Caravan  For sale  Price     Relationship" :
 " City               Race      Caravan  Wanted    Offer     Relationship",
                      rows, "Enter: Make a deal  Esc/Q: Back", "", key);
  if (index < 0) {
    return;
  }
  AI_city* city = partners[index];
  int most = amounts[index];
  if (!buying && player_has(pl_city, good) < most) {
    most = player_has(pl_city, good);
  }
  int amount = int_input_popup("%s %s a day (up to %d, at %d gold per 100):",
                               buying ? "Buy how much" : "Sell how much",
                               what.c_str(), most, prices[index]);
  if (amount <= 0) {
    return;
  }
  if (amount > most) {
    amount = most;
  }
  int daily = (amount * prices[index] + 50) / 100;
  if (!query_yn("%s %d %s a day %s %s for %d gold a day?",
                buying ? "Buy" : "Sell", amount, what.c_str(),
                buying ? "from" : "to", city->get_name().c_str(), daily)) {
    return;
  }
  Trade_deal deal;
  deal.city_uid = city->uid;
  deal.resource = good.res;
  deal.mineral = good.min;
  deal.amount = amount;
  deal.price = prices[index];
  deal.importing = buying;
  realm->add_deal(deal);
}

// *** Diplomacy ***

void Interface::diplomacy_screen()
{
  Realm* realm = GAME->realm;
  int start = 0;
  while (true) {
// Every city you know of, nearest first.
    std::vector<AI_city*> known;
    for (int i = 0; i < realm->cities.size(); i++) {
      AI_city* city = realm->cities[i];
      if (pl_city->world_seen.is_seen(city->location)) {
        known.push_back(city);
      }
    }
    for (int i = 0; i < known.size(); i++) {
      for (int j = i + 1; j < known.size(); j++) {
        if (rl_dist(pl_city->location, known[j]->location) <
            rl_dist(pl_city->location, known[i]->location)) {
          std::swap(known[i], known[j]);
        }
      }
    }
    std::vector<std::string> rows;
    for (int i = 0; i < known.size(); i++) {
      AI_city* city = known[i];
      City_relation& rel = realm->relation(city);
      std::stringstream ss;
      std::string kind = (city->get_city_type() == CITY_TYPE_CAPITAL ? "capital" :
                          city->get_city_type() == CITY_TYPE_DUCHY ? "duchy" :
                          "city");
      ss << fit(city->get_name(), 18) << " " << fit(kind, 7) << " " <<
            fit(rel.vassal ? std::string("Yours") :
                kingdom_name(realm->owner_kingdom(city)), 10) << " " <<
            fit(num(rl_dist(pl_city->location, city->location)), 4) << " <c=" <<
            color_tag(realm->relationship_color(rel.opinion)) << ">" <<
            fit(realm->relationship_name(rel.opinion), 12) << "<c=/>";
      if (realm->at_war_with_player(city)) {
        ss << "<c=ltred>WAR<c=/> ";
      }
      if (city->besieged) {
        ss << "<c=red>besieged<c=/> ";
      }
      if (rel.vassal) {
        ss << "<c=ltgreen>vassal<c=/> ";
      }
      for (int t = 1; t < TREATY_MAX; t++) {
        if (rel.has_treaty(Treaty_type(t))) {
          ss << "<c=ltblue>" << (t == TREATY_TRADE ? "T" :
                                 t == TREATY_PEACE ? "P" : "A") << "<c=/>";
        }
      }
      rows.push_back(ss.str());
    }
    long key;
    int index = list_picker("Diplomacy - the cities you know",
" City               Kind    Kingdom    Dist Relationship (Treaties: T P A)",
                            rows, "Enter: Speak with them  Esc/Q: Leave", "",
                            key, start);
    if (index < 0) {
      return;
    }
    start = index;
    city_dialogue(known[index]);
  }
}

// What a city's spokesman says when you arrive.
static std::string greeting(Realm* realm, AI_city* city)
{
  City_relation& rel = realm->relation(city);
  if (rel.vassal) {
    if (rel.opinion < -30) {
      return "We pay what you ask.  Do not ask us to like it.";
    }
    return "Your servants in " + city->get_name() + " greet you, my lord.";
  }
  if (realm->at_war_with_player(city)) {
    return "We have nothing to say to you, unless you have come to make peace.";
  }
  if (rel.opinion >= 75) {
    return "Welcome, old friend!  What can we do for you?";
  } else if (rel.opinion >= 45) {
    return "Welcome, friend.  It is always good to see you.";
  } else if (rel.opinion >= 15) {
    return "Greetings, neighbor.  What brings you to " + city->get_name() + "?";
  } else if (rel.opinion > -15) {
    return "Greetings.  Speak your business.";
  } else if (rel.opinion > -45) {
    return "You may speak.  Be brief.";
  }
  return "What do you want?";
}

static std::string contact_name(AI_city* city)
{
  std::string race = Race_data[city->get_race()]->plural_name;
  switch (city->get_city_type()) {
    case CITY_TYPE_CAPITAL:
      return (city->uid % 2 ? "The Queen of the " : "The King of the ") +
             race;
    case CITY_TYPE_DUCHY:
      return (city->uid % 2 ? "The Duchess of " : "The Duke of ") +
             city->get_name();
    default:
      return "The Mayor of " + city->get_name();
  }
}

// Draws the dialogue frame of screenshots/dialogue.png.
static void draw_dialogue(Window& w, AI_city* city, std::string speech,
                          std::vector<std::string> options,
                          std::vector<std::string> responses)
{
  Realm* realm = GAME->realm;
  City_relation& rel = realm->relation(city);
  w.clear();
  w.outline();
  for (int y = 1; y < 23; y++) {
    w.putch(26, y, c_white, c_black, LINE_XOXO);
  }
  w.putch(26, 0, c_white, c_black, LINE_OXXX);
  w.putch(26, 23, c_white, c_black, LINE_XXOX);
  for (int x = 1; x < 26; x++) {
    w.putch(x, 8, c_white, c_black, LINE_OXOX);
  }
  w.putch(0, 8, c_white, c_black, LINE_XXXO);
  w.putch(26, 8, c_white, c_black, LINE_XOXX);
  for (int x = 27; x < 79; x++) {
    w.putch(x, 12, c_white, c_black, LINE_OXOX);
  }
  w.putch(26, 12, c_white, c_black, LINE_XXXO);
  w.putch(79, 12, c_white, c_black, LINE_XOXX);
  w.putstr(2, 0, c_ltblue, c_black, "Diplomacy");

  w.putstr(1, 1, c_yellow, c_black, "Contact:");
  w.putstr_n(1, 2, c_white, c_black, 25, contact_name(city));
  std::string kingdom = (rel.vassal ? std::string("Your vassal") :
                         kingdom_name(realm->owner_kingdom(city)));
  w.putstr_n(1, 3, c_ltblue, c_black, 25, kingdom);
  w.putstr(1, 4, c_yellow, c_black, "Relationship:");
  w.putstr(1, 5, realm->relationship_color(rel.opinion), c_black,
           "%s (%d)", realm->relationship_name(rel.opinion).c_str(),
           rel.opinion);
  int line = 6;
  if (realm->at_war_with_player(city)) {
    w.putstr(1, line++, c_ltred, c_black, "At war with you");
  }
  for (int t = 1; t < TREATY_MAX && line < 8; t++) {
    if (rel.has_treaty(Treaty_type(t))) {
      w.putstr(1, line++, c_ltblue, c_black, "%s (%d days)",
               capitalize(treaty_name(Treaty_type(t))).c_str(),
               rel.treaty_days[t]);
    }
  }

  w.putstr(1, 9, c_white, c_black, "Dialogue Options:");
  for (int i = 0; i < options.size() && i < 12; i++) {
    w.putstr(1, 11 + i, c_ltgray, c_black, options[i]);
  }

  std::string who = city->get_name();
  w.putstr(28, 1, c_yellow, c_black, "%s:", who.c_str());
  std::vector<std::string> lines = break_into_lines(speech, 48);
  for (int i = 0; i < lines.size() && i < 10; i++) {
    w.putstr(30, 2 + i, c_white, c_black, lines[i]);
  }
  w.putstr(28, 13, c_yellow, c_black, "You:");
  for (int i = 0; i < responses.size() && i < 9; i++) {
    w.putstr(29, 14 + i, c_ltgray, c_black, responses[i]);
  }
}

void Interface::city_dialogue(AI_city* city)
{
  Realm* realm = GAME->realm;
  Window w(0, 0, 80, 24);
  std::string speech = greeting(realm, city);
  std::vector<std::string> responses;
  while (true) {
    City_relation& rel = realm->relation(city);
    bool war = realm->at_war_with_player(city);
    std::vector<std::string> options;
    if (!war && !rel.vassal) {
      options.push_back("<c=magenta>T<c=/>: Propose a treaty");
      options.push_back("<c=magenta>G<c=/>: Give a gift");
      if (!rel.has_treaty(TREATY_PEACE) && !rel.has_treaty(TREATY_ALLIANCE)) {
        options.push_back("<c=magenta>D<c=/>: Demand tribute");
      }
      options.push_back("<c=magenta>W<c=/>: Declare <c=ltred>WAR<c=/>");
    } else if (war) {
      options.push_back("<c=magenta>P<c=/>: Ask for peace");
    } else {
      options.push_back("<c=magenta>G<c=/>: Give a gift");
    }
    options.push_back("<c=magenta>I<c=/>: About " + city->get_name());
    options.push_back("<c=magenta>V<c=/>: View world map");
    options.push_back("<c=magenta>Esc/Q<c=/>: End dialogue");
    draw_dialogue(w, city, speech, options, responses);
    w.refresh();
    responses.clear();
    long ch = input();

    if (ch == KEY_ESC || ch == 'q' || ch == 'Q') {
      return;

    } else if (ch == 'v' || ch == 'V') {
      GAME->world->draw(city->location, &(pl_city->world_seen));

    } else if (ch == 'i' || ch == 'I') {
      std::stringstream ss;
      ss << city->get_name() << ", " << realm->city_title(city) << ".  " <<
            city->get_total_population() << " people (" <<
            city->population[CIT_PEASANT].count << " peasants, " <<
            city->population[CIT_MERCHANT].count << " merchants, " <<
            city->population[CIT_BURGHER].count << " burghers).  Our scouts " <<
            "count " << about(city->garrison.total()) << " soldiers.  ";
      int days = realm->trade_days(city);
      if (days >= 0) {
        ss << "A caravan takes " << days << (days == 1 ? " day" : " days") <<
              " to reach it.  ";
      }
      std::vector<std::string> sells;
      for (int i = 1; i < RES_MAX && sells.size() < 4; i++) {
        Resource res = Resource(i);
        if (!Resource_data[res]->meta && res != RES_GOLD &&
            city->get_net_resource_production(res) > 0) {
          sells.push_back(Resource_data[res]->name);
        }
      }
      if (!sells.empty()) {
        ss << "It sells ";
        for (int i = 0; i < sells.size(); i++) {
          ss << sells[i] << (i + 2 == sells.size() ? " and " :
                             i + 1 == sells.size() ? ".  " : ", ");
        }
      }
      if (city->besieged) {
        ss << "It is under siege.";
      }
      speech = ss.str();

    } else if ((ch == 't' || ch == 'T') && !war && !rel.vassal) {
      std::vector<std::string> resp;
      int prices[TREATY_MAX];
      for (int t = 1; t < TREATY_MAX; t++) {
        prices[t] = realm->treaty_price(city, Treaty_type(t));
        std::stringstream ss;
        ss << "<c=magenta>" << t << "<c=/>: " <<
              (t == TREATY_ALLIANCE ? "An " : "A ") << treaty_name(Treaty_type(t));
        if (prices[t] < 0) {
          ss << " <c=dkgray>(they won't)<c=/>";
        } else {
          ss << " for <c=yellow>" << prices[t] << " gold<c=/>";
          if (rel.has_treaty(Treaty_type(t))) {
            ss << " (renew)";
          }
        }
        resp.push_back(ss.str());
      }
      resp.push_back("<c=magenta>Esc<c=/>: Never mind");
      draw_dialogue(w, city, "These are our terms.  A trade pact halves the \
overhead on your caravans to us.  A peace pact means a year without demands \
or war.  An alliance means our soldiers will march to help you if you are \
besieged.", options, resp);
      w.refresh();
      long ch2 = input();
      int t = ch2 - '0';
      if (t > 0 && t < TREATY_MAX) {
        if (prices[t] < 0) {
          speech = "We will not sign that with you.  Not as things stand.";
        } else if (pl_city->get_resource_amount(RES_GOLD) < prices[t]) {
          speech = "You don't have the gold.  Come back when you do.";
        } else if (realm->sign_treaty(city, Treaty_type(t))) {
          speech = "It is agreed.  May it serve us both.";
        }
      }

    } else if (ch == 'g' || ch == 'G') {
      int gold = int_input_popup("Give how much gold? (you have %d)",
                                 pl_city->get_resource_amount(RES_GOLD));
      if (gold > 0) {
        if (gold > pl_city->get_resource_amount(RES_GOLD)) {
          speech = "You are generous with gold you don't have.";
        } else {
          int gain = realm->give_gift(city, gold);
          if (gain >= 10) {
            speech = "Such generosity!  We will not forget it.";
          } else if (gain > 0) {
            speech = "Our thanks.";
          } else {
            speech = "Is that all?  Well.  Thank you, we suppose.";
          }
        }
      }

    } else if ((ch == 'd' || ch == 'D') && !war && !rel.vassal &&
               !rel.has_treaty(TREATY_PEACE) &&
               !rel.has_treaty(TREATY_ALLIANCE)) {
      if (rel.demand_days > 0) {
        speech = "Again?  We heard your last demand not long ago.  No.";
      } else if (query_yn("Demand that %s pay you tribute?",
                          city->get_name().c_str())) {
        int paid = realm->demand_tribute(city);
        if (paid > 0) {
          std::stringstream ss;
          ss << "Very well.  Take your " << paid << " gold, and leave us in " <<
                "peace.";
          speech = ss.str();
        } else if (realm->at_war_with_player(city)) {
          speech = "Come and take it, then!";
        } else {
          speech = "No.  You may leave now.";
        }
      }

    } else if ((ch == 'w' || ch == 'W') && !war && !rel.vassal) {
      if (rel.truce_days > 0) {
        speech = "The ink on our peace is barely dry.  Even you would not " \
                 "break it so soon.";
      } else {
        Kingdom* theirs = realm->owner_kingdom(city);
        Kingdom* ours = realm->player_kingdom();
        std::string warning;
        if (theirs && theirs != ours) {
          warning = "This means war with every city of the " +
                    Race_data[theirs->race]->plural_name + ".\n";
        } else {
          warning = "This city is of your own kingdom; your lords will " \
                    "take notice.\n";
        }
        if (query_yn("%sDeclare war on %s?", warning.c_str(),
                     city->get_name().c_str())) {
          realm->declare_war(city);
          speech = "So be it.  We will meet you in the field.";
        }
      }

    } else if ((ch == 'p' || ch == 'P') && war) {
      bool refuse;
      int price = realm->peace_price(city, refuse);
      if (refuse) {
        Kingdom* ours = realm->player_kingdom();
        if (ours && !rel.at_war && realm->kingdoms_at_war(ours->uid,
            city->owner_kingdom)) {
          speech = "This war is between our kingdoms.  It is for your " \
                   "monarch and ours to end, not you.";
        } else {
          speech = "Peace?  This war has barely begun.";
        }
      } else {
        std::stringstream ss;
        if (price > 0) {
          ss << "Peace will cost you " << price << " gold.";
        } else if (price < 0) {
          ss << "We will pay you " << (0 - price) << " gold to end this war.";
        } else {
          ss << "We will make peace, and ask nothing.";
        }
        draw_dialogue(w, city, ss.str(), options,
                      std::vector<std::string>(1,
                      "<c=magenta>Y<c=/>: Agree   <c=magenta>N<c=/>: Refuse"));
        w.refresh();
        long ch2 = input();
        if (ch2 == 'y' || ch2 == 'Y') {
          if (price > pl_city->get_resource_amount(RES_GOLD)) {
            speech = "You don't have the gold.  The war goes on.";
          } else if (realm->make_peace(city)) {
            speech = "Then there is peace between us.  Let it last.";
          }
        }
      }
    }
  }
}

// *** Military ***

void Interface::military_screen()
{
  Realm* realm = GAME->realm;
  Window w(0, 0, 80, 24);
  while (true) {
    Troops troops = pl_city->get_troops();
    Race race = pl_city->get_race();
    w.clear();
    w.outline();
    w.putstr(2, 0, c_ltblue, c_black, "Military");
    w.putstr(1, 1, c_white, c_black, "Roster (in %s):",
             pl_city->get_name().c_str());
    w.putstr(1, 2, c_ltcyan, c_black, "Type:          Count:  Att:  Def:");
    int y = 3;
    for (int i = 1; i < UNIT_MAX; i++) {
      nc_color col = (troops.count[i] > 0 ? c_ltgray : c_dkgray);
      w.putstr(1, y, col, c_black, "%-14s %6d  %4d  %4d",
               capitalize(Unit_data[i].name).c_str(), troops.count[i],
               Unit_data[i].attack, Unit_data[i].defense);
      y++;
    }
    int in_field = 0;
    for (int i = 0; i < realm->armies.size(); i++) {
      if (realm->armies[i].player_owned) {
        in_field += realm->armies[i].troops.total();
      }
    }
    w.putstr(1, 9, c_ltgray, c_black, "Barracks room: %d of %d in use",
             troops.total() + in_field, pl_city->get_military_supported());
    w.putstr(1, 10, c_ltgray, c_black, "Wages:         %d gold a day",
             pl_city->get_military_expense());

    w.putstr(40, 1, c_white, c_black, "Combat Strength:");
    int raw = troops_strength(troops, race, 100);
    w.putstr(40, 2, c_ltgray, c_black, "Total attack:      %6d", raw);
    w.putstr(40, 3, c_ltgray, c_black, "%-18s %6d%%",
             (capitalize(Race_data[race]->plural_name) + ":").c_str(),
             Race_data[race]->base_combat * 10);
    w.putstr(40, 4, c_ltgray, c_black, "Behind your walls: %6d%%", 130);
    w.putstr(40, 5, c_yellow, c_black, "Defending:         %6d",
             troops_strength(troops, race, 130));
    if (realm->player_besieged()) {
      w.putstr(40, 7, c_ltred, c_black, "YOUR CITY IS UNDER SIEGE");
      w.putstr(40, 8, c_ltred, c_black, "The keep holds %d more days.",
               realm->siege_days_left(pl_city));
    }

    w.putstr(1, 12, c_white, c_black, "Armies in the field:");
    y = 13;
    int shown = 0;
    for (int i = 0; i < realm->armies.size() && y < 19; i++) {
      Army& army = realm->armies[i];
      if (!army.player_owned) {
        continue;
      }
      shown++;
      City* target = GAME->world->lookup_city_uid(army.target_uid);
      std::stringstream ss;
      ss << shown << ". " << army.troops.total() << " soldiers, " <<
            army_order_name(army.order);
      if (army.order != ARMY_ORDER_RETURN && target) {
        ss << " " << target->get_name();
      }
      if (army.order == ARMY_ORDER_SIEGE) {
        ss << " (" << realm->siege_days_left(target) << " days to go)";
      } else {
        ss << " (" << army.path.size() << " leagues to go)";
      }
      ss << ", food for " << army.supplies << " days";
      w.putstr_n(2, y, c_ltgray, c_black, 76, ss.str());
      y++;
    }
    if (shown == 0) {
      w.putstr(2, 13, c_dkgray, c_black, "None.");
    }
    w.putstr(1, 20, c_magenta, c_black,
             "R: Recruit  D: Disband  S: Send an army  C: Call an army home");
    w.putstr(1, 21, c_magenta, c_black,
             "V: View world map  Esc/Q: Leave this screen");
    w.refresh();

    long ch = input();
    if (ch == KEY_ESC || ch == 'q' || ch == 'Q') {
      return;
    } else if (ch == 'v' || ch == 'V') {
      GAME->world->draw(pl_city->location, &(pl_city->world_seen));
    } else if (ch == 'r' || ch == 'R') {
      std::vector<std::string> opts;
      for (int i = 1; i < UNIT_MAX; i++) {
        std::stringstream ss;
        ss << capitalize(Unit_data[i].name) << " (" <<
              citizen_type_name(Unit_data[i].citizen, true) << ", " <<
              Unit_data[i].recruit_cost << " gold";
        if (Unit_data[i].gear != RES_NULL) {
          ss << ", " << Unit_data[i].gear_amount << " " <<
                Resource_data[ Unit_data[i].gear ]->name;
        }
        ss << "): up to " << pl_city->max_recruits(Unit_type(i));
        opts.push_back(ss.str());
      }
      opts.push_back("Never mind");
      int pick = menu_vec("Recruit which soldiers?", opts) + 1;
      if (pick > 0 && pick < UNIT_MAX) {
        Unit_type type = Unit_type(pick);
        int most = pl_city->max_recruits(type);
        if (most <= 0) {
          popup("You can't raise any %s: you need idle %s, room in your \
barracks, the gold%s.", Unit_data[type].name.c_str(),
                citizen_type_name(Unit_data[type].citizen, true).c_str(),
                Unit_data[type].gear == RES_NULL ? "" : " and the gear");
        } else {
          int amount = int_input_popup("Recruit how many %s? (up to %d)",
                                       Unit_data[type].name.c_str(), most);
          if (amount > 0) {
            std::string why = pl_city->recruit(type, amount);
            if (!why.empty()) {
              popup(why.c_str());
            }
          }
        }
      }
    } else if (ch == 'd' || ch == 'D') {
      std::vector<std::string> opts;
      std::vector<Unit_type> types;
      for (int i = 1; i < UNIT_MAX; i++) {
        if (troops.count[i] > 0) {
          opts.push_back(capitalize(Unit_data[i].name) + " (" +
                         itos(troops.count[i]) + ")");
          types.push_back(Unit_type(i));
        }
      }
      if (types.empty()) {
        popup("You have no soldiers at home.");
      } else {
        opts.push_back("Never mind");
        int pick = menu_vec("Send which soldiers home to their work?", opts);
        if (pick >= 0 && pick < types.size()) {
          int amount = int_input_popup("Disband how many? (up to %d)",
                                       troops.count[ types[pick] ]);
          if (amount > 0) {
            pl_city->disband(types[pick], amount);
          }
        }
      }
    } else if (ch == 's' || ch == 'S') {
      send_army_screen();
    } else if (ch == 'c' || ch == 'C') {
      std::vector<std::string> opts;
      std::vector<int> indices;
      for (int i = 0; i < realm->armies.size(); i++) {
        Army& army = realm->armies[i];
        if (army.player_owned && army.order != ARMY_ORDER_RETURN) {
          City* target = GAME->world->lookup_city_uid(army.target_uid);
          opts.push_back(itos(army.troops.total()) + " soldiers, " +
                         army_order_name(army.order) + " " +
                         (target ? target->get_name() : std::string("")));
          indices.push_back(i);
        }
      }
      if (indices.empty()) {
        popup("You have no armies out to call home.");
      } else {
        opts.push_back("Never mind");
        int pick = menu_vec("Call which army home?", opts);
        if (pick >= 0 && pick < indices.size()) {
          realm->recall_army(indices[pick]);
        }
      }
    }
  }
}

void Interface::send_army_screen()
{
  Realm* realm = GAME->realm;
  Troops troops = pl_city->get_troops();
  if (troops.fighters() <= 0) {
    popup("You have no soldiers at home to send.");
    return;
  }
// Enemy cities you know of, nearest first.
  std::vector<AI_city*> targets;
  for (int i = 0; i < realm->cities.size(); i++) {
    AI_city* city = realm->cities[i];
    if (pl_city->world_seen.is_seen(city->location) &&
        realm->at_war_with_player(city)) {
      targets.push_back(city);
    }
  }
  if (targets.empty()) {
    popup("You are at war with no city you know of.  Declare war first \
(World menu, Diplomacy).");
    return;
  }
  for (int i = 0; i < targets.size(); i++) {
    for (int j = i + 1; j < targets.size(); j++) {
      if (rl_dist(pl_city->location, targets[j]->location) <
          rl_dist(pl_city->location, targets[i]->location)) {
        std::swap(targets[i], targets[j]);
      }
    }
  }
  std::vector<std::string> rows;
  for (int i = 0; i < targets.size(); i++) {
    AI_city* city = targets[i];
    std::stringstream ss;
    ss << fit(city->get_name(), 18) << " " <<
          fit(kingdom_name(realm->owner_kingdom(city)), 10) << " " <<
          fit(num(rl_dist(pl_city->location, city->location)), 5) << " " <<
          fit(about(city->garrison.total()) + " soldiers", 24);
    if (city->besieged) {
      ss << "<c=red>besieged<c=/>";
    }
    rows.push_back(ss.str());
  }
  long key;
  int index = list_picker("Send an army: where?",
                          " City               Kingdom    Dist  Garrison",
                          rows, "Enter: March on it  Esc/Q: Back", "", key);
  if (index < 0) {
    return;
  }
  AI_city* target = targets[index];
  int total = troops.total();
  int amount = int_input_popup("Send how many of your %d soldiers to %s?",
                               total, target->get_name().c_str());
  if (amount <= 0) {
    return;
  }
  if (amount > total) {
    amount = total;
  }
// Take every kind in proportion.
  Troops sending;
  int taken = 0;
  for (int i = 1; i < UNIT_MAX; i++) {
    sending.count[i] = (troops.count[i] * amount) / total;
    taken += sending.count[i];
  }
  for (int i = 1; i < UNIT_MAX && taken < amount; i++) {
    while (taken < amount && sending.count[i] < troops.count[i]) {
      sending.count[i]++;
      taken++;
    }
  }
  if (!realm->send_army(sending, target)) {
    popup("Your army can't go: there is no road there, or not enough food to \
carry (each soldier needs a day's food for every day out, and ten more).");
    return;
  }
  popup("Your army of %d marches on %s.", sending.total(),
        target->get_name().c_str());
}

// *** The realm: your liege, your rank, the kingdoms ***

void Interface::realm_screen()
{
  Realm* realm = GAME->realm;
  Window w(0, 0, 80, 24);
  while (true) {
    w.clear();
    w.outline();
    w.putstr(2, 0, c_ltblue, c_black, "The Realm");
    Kingdom* ours = realm->player_kingdom();
    City* liege = realm->get_liege();
    w.putstr(1, 1, c_white, c_black, "You are the %s of %s.",
             rank_name(realm->rank).c_str(), pl_city->get_name().c_str());
    int y = 2;
    if (!ours) {
      w.putstr(1, y++, c_ltgray, c_black, "The %s have no kingdom in this \
world; you answer to no one.",
               Race_data[pl_city->get_race()]->plural_name.c_str());
      w.putstr(1, y++, c_ltgray, c_black, "Rule four cities besides your \
own and you may proclaim a kingdom.");
    } else if (realm->excommunicated) {
      w.putstr(1, y++, c_ltred, c_black, "You have been cast out of the \
kingdom of the %s.", Race_data[ours->race]->plural_name.c_str());
    } else {
      if (liege) {
        int favor = realm->get_liege_favor();
        w.putstr(1, y++, c_ltgray, c_black, "Your liege: %s (%s).",
                 liege->get_name().c_str(),
                 realm->city_title(liege).c_str());
        w.putstr(1, y++, realm->relationship_color(favor), c_black,
                 "Their favor: %d (%s)", favor,
                 realm->relationship_name(favor).c_str());
      }
      if (realm->rank == RANK_BARON && liege &&
          liege->get_city_type() != CITY_TYPE_CAPITAL) {
        w.putstr(1, y++, realm->relationship_color(realm->favor_king), c_black,
                 "The crown's favor: %d (%s)", realm->favor_king,
                 realm->relationship_name(realm->favor_king).c_str());
      }
      w.putstr(1, y++, c_ltgray, c_black, "Taxes: %d gold a month%s",
               realm->get_monthly_taxes(),
               realm->months_unpaid > 0 ?
               (" (" + itos(realm->months_unpaid) + " months owed)").c_str() :
               "");
      if (realm->loan_owed > 0) {
        w.putstr(1, y++, c_ltgray, c_black, "You owe your liege %d gold.",
                 realm->loan_owed);
      }
      if (realm->rank == RANK_BARON) {
        w.putstr(1, y++, c_dkgray, c_black, "With the crown's favor at 70, a \
baron may be named a duke.");
      } else if (realm->rank == RANK_DUKE) {
        w.putstr(1, y++, c_dkgray, c_black, "When the old monarch dies, the \
court may choose a duke it trusts (60).");
      }
      w.putstr(1, y++, c_dkgray, c_black, "Or take %s, the capital, by force.",
               ours->capital ? ours->capital->get_name().c_str() : "?");
    }
    y++;
    if (realm->demand_type != LIEGE_DEMAND_NULL) {
      w.putstr(1, y++, c_yellow, c_black, "Your liege demands:");
      std::vector<std::string> lines =
        break_into_lines(realm->describe_demand(), 76);
      for (int i = 0; i < lines.size() && i < 3; i++) {
        w.putstr(2, y++, c_white, c_black, lines[i]);
      }
    }
    y++;
// Vassals.
    int vassals = 0;
    std::stringstream ss_vassals;
    for (int i = 0; i < realm->cities.size(); i++) {
      if (realm->relation(realm->cities[i]).vassal) {
        ss_vassals << (vassals > 0 ? ", " : "") <<
                      realm->cities[i]->get_name();
        vassals++;
      }
    }
    if (vassals > 0) {
      w.putstr(1, y++, c_ltgreen, c_black, "Your vassals: %s",
               ss_vassals.str().c_str());
      w.putstr(1, y++, c_ltgray, c_black, "Tribute: about %d gold a day.",
               realm->get_tribute());
    }
// The kingdoms.
    w.putstr(1, y++, c_white, c_black, "The kingdoms:");
    for (int i = 0; i < GAME->kingdoms.size() && y < 20; i++) {
      Kingdom* k = GAME->kingdoms[i];
      std::stringstream ss;
      int cities_held = 0;
      for (int n = 0; n < realm->cities.size(); n++) {
        if (realm->cities[n]->owner_kingdom == k->uid) {
          cities_held++;
        }
      }
      ss << "<c=" << color_tag(k->color) << ">" <<
            fit(kingdom_name(k), 10) << "<c=/> " << fit(num(cities_held) +
            " cities", 10);
      if (k == ours) {
        ss << " <c=white>yours<c=/>";
      }
      if (realm->kingdom_at_war_with_player(k->uid)) {
        ss << " <c=ltred>at war with you<c=/>";
      }
      for (int n = 0; n < GAME->kingdoms.size(); n++) {
        Kingdom* other = GAME->kingdoms[n];
        if (other != k && realm->kingdoms_at_war(k->uid, other->uid) &&
            k->uid < other->uid) {
          ss << " <c=red>war with the " <<
                Race_data[other->race]->plural_name << "<c=/>";
        }
      }
      w.putstr(2, y++, c_ltgray, c_black, ss.str());
    }
    w.putstr(1, 21, c_magenta, c_black,
             "A: Answer a demand  L: Ask for a loan  H: Ask for help  \
Esc/Q: Leave");
    w.refresh();

    long ch = input();
    if (ch == KEY_ESC || ch == 'q' || ch == 'Q') {
      return;
    } else if ((ch == 'a' || ch == 'A') &&
               realm->demand_type != LIEGE_DEMAND_NULL) {
      liege_demand_dialogue();
    } else if ((ch == 'l' || ch == 'L' || ch == 'h' || ch == 'H') && liege &&
               !realm->excommunicated) {
      bool loan = (ch == 'l' || ch == 'L');
      if (loan && realm->loan_owed > 0) {
        popup("Pay off the loan you have first.");
      } else if (realm->get_liege_favor() < 0) {
        popup("Your liege does not favor you enough to help.");
      } else if (query_yn("Ask %s for %s?  It will cost you %d favor.",
                          liege->get_name().c_str(),
                          loan ? "a loan (paid back over ten months)" :
                                 "a gift of gold",
                          realm->aid_price(loan))) {
        int gold = realm->request_aid(loan);
        if (gold > 0) {
          popup("%s sends you %d gold.", liege->get_name().c_str(), gold);
        } else {
          popup("%s will not help you.", liege->get_name().c_str());
        }
      }
    }
  }
}

void Interface::liege_demand_dialogue()
{
  Realm* realm = GAME->realm;
  City* liege = realm->get_liege();
  AI_city* ai_liege = dynamic_cast<AI_city*>(liege);
  if (!ai_liege || realm->demand_type == LIEGE_DEMAND_NULL) {
    return;
  }
  Window w(0, 0, 80, 24);
  std::vector<std::string> responses;
  std::string half;
  switch (realm->demand_type) {
    case LIEGE_DEMAND_GOLD:
    case LIEGE_DEMAND_FOOD:
    case LIEGE_DEMAND_SOLDIERS:
      half = "I can only give half of that, my liege.  I ask your patience.";
      break;
    default:
      half = "I will do as you say, my liege, though it costs me dearly.";
      break;
  }
  responses.push_back("<c=magenta>1<c=/>  You are right, my liege.  It will \
be done.");
  responses.push_back("<c=magenta>2<c=/>  " + half);
  responses.push_back("<c=magenta>3<c=/>  I'm sorry, my liege, but this \
cannot be done.");
  std::vector<std::string> options;
  options.push_back("<c=magenta>1-3<c=/>: Answer");
  options.push_back("<c=magenta>Esc<c=/>: Answer later");
  draw_dialogue(w, ai_liege, "On to other matters...\n" +
                realm->describe_demand(), options, responses);
  w.refresh();
  while (true) {
    long ch = input();
    if (ch == KEY_ESC || ch == 'q' || ch == 'Q') {
      return;
    }
    if (ch >= '1' && ch <= '3') {
      realm->answer_demand(ch - '1');
      return;
    }
  }
}

void Interface::show_game_end()
{
  Realm* realm = GAME->realm;
  bool won = (realm->game_end == GAME_END_CROWNED);
  Window w(0, 0, 80, 24);
  w.clear();
  w.outline();
  w.putstr(2, 0, won ? c_yellow : c_ltred, c_black,
           won ? "Victory" : "Defeat");
  std::vector<std::string> lines = break_into_lines(realm->end_text, 70);
  int y = 3;
  for (int i = 0; i < lines.size() && y < 12; i++) {
    w.putstr(5, y++, c_white, c_black, lines[i]);
  }
  y++;
  Date date = GAME->get_date();
  w.putstr(5, ++y, c_ltgray, c_black, "%s, %s.  You ruled for %d years.",
           pl_city->get_name().c_str(), date.get_text().c_str(),
           date.get_year() - realm->start_year);
  w.putstr(5, ++y, c_ltgray, c_black, "Population at the end: %d",
           pl_city->get_total_population());
  int vassals = 0;
  for (int i = 0; i < realm->cities.size(); i++) {
    if (realm->relation(realm->cities[i]).vassal) {
      vassals++;
    }
  }
  w.putstr(5, ++y, c_ltgray, c_black, "Cities sworn to you: %d", vassals);
  if (won) {
    w.putstr(5, 20, c_ltgreen, c_black, "You may rule on as long as you like.");
  }
  w.putstr(5, 21, c_magenta, c_black, "Press Enter.");
  w.refresh();
  long ch;
  do {
    ch = input();
  } while (ch != '\n' && ch != ' ' && ch != KEY_ESC);
}
