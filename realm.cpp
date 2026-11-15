#include "realm.h"
#include "ai_city.h"
#include "player_city.h"
#include "kingdom.h"
#include "world.h"
#include "game.h"
#include "globals.h"
#include "window.h"
#include "stringfunc.h"
#include "rng.h"
#include "log.h"
#include "files.h"
#include <fstream>
#include <sstream>

#define REALM_SAVE_VERSION 1

std::string treaty_name(Treaty_type type)
{
  switch (type) {
    case TREATY_NULL:     return "nothing";
    case TREATY_TRADE:    return "trade pact";
    case TREATY_PEACE:    return "peace pact";
    case TREATY_ALLIANCE: return "alliance";
    case TREATY_MAX:      return "BUG - TREATY_MAX";
  }
  return "BUG - Unnamed Treaty_type";
}

std::string rank_name(Noble_rank rank, bool female)
{
  switch (rank) {
    case RANK_BARON:   return (female ? "baroness" : "baron");
    case RANK_DUKE:    return (female ? "duchess"  : "duke");
    case RANK_MONARCH: return (female ? "queen"    : "king");
    case RANK_MAX:     return "BUG - RANK_MAX";
  }
  return "BUG - Unnamed Noble_rank";
}

/* From notes/fealty.txt, Whales' notes on what each race's lords expect of
 * their cities.  Taxes are gold per month per 100 citizens; the rest run from
 * 0 to 4, "none" to "very high".
 */
static Fealty_datum fealty(int taxes, int strictness, int infighting, int aid,
                           int military_aid, int demands)
{
  Fealty_datum ret;
  ret.taxes = taxes;
  ret.strictness = strictness;
  ret.infighting = infighting;
  ret.aid = aid;
  ret.military_aid = military_aid;
  ret.demands = demands;
  return ret;
}

Fealty_datum get_fealty(Race race)
{
  Fealty_datum ret;
  //                      taxes strict infight aid mil.aid demands
  switch (race) {
    case RACE_HUMAN:    ret = fealty(30, 2, 1, 3, 2, 1); break;
    case RACE_ELF:      ret = fealty(20, 4, 3, 3, 3, 2); break;
    case RACE_DWARF:    ret = fealty(25, 2, 1, 2, 3, 1); break;
    case RACE_ORC:      ret = fealty(8, 1, 0, 0, 1, 2); break;
    case RACE_GOBLIN:   ret = fealty(15, 3, 0, 1, 2, 2); break;
    case RACE_OGRE:     ret = fealty(20, 1, 2, 3, 3, 2); break;
    case RACE_TROLL:    ret = fealty(3, 0, 0, 1, 2, 3); break;
    case RACE_HALFLING: ret = fealty(30, 2, 2, 3, 1, 1); break;
    case RACE_GNOME:    ret = fealty(40, 3, 3, 3, 1, 1); break;
    case RACE_MINOTAUR: ret = fealty(20, 3, 2, 3, 3, 3); break;
    case RACE_RAKSHASA: ret = fealty(55, 3, 2, 1, 4, 3); break;
    case RACE_NAGA:     ret = fealty(35, 2, 1, 3, 4, 2); break;
    default:            ret = fealty(25, 2, 1, 2, 2, 1); break;
  }
  return ret;
}

// *** City_relation ***

City_relation::City_relation()
{
  opinion = 0;
  for (int i = 0; i < TREATY_MAX; i++) {
    treaty_days[i] = 0;
  }
  at_war = false;
  truce_days = 0;
  gift_days = 999;
  demand_days = 0;
  vassal = false;
}

std::string City_relation::save_data()
{
  std::stringstream ret;
  ret << opinion << " " << TREATY_MAX << " ";
  for (int i = 0; i < TREATY_MAX; i++) {
    ret << treaty_days[i] << " ";
  }
  ret << (at_war ? 1 : 0) << " " << truce_days << " " << gift_days << " " <<
         demand_days << " " << (vassal ? 1 : 0);
  return ret.str();
}

bool City_relation::load_data(std::istream& data)
{
  int num, tmpwar, tmpvassal;
  data >> opinion >> num;
  for (int i = 0; i < num; i++) {
    int tmp;
    data >> tmp;
    if (i < TREATY_MAX) {
      treaty_days[i] = tmp;
    }
  }
  data >> tmpwar >> truce_days >> gift_days >> demand_days >> tmpvassal;
  at_war = (tmpwar != 0);
  vassal = (tmpvassal != 0);
  return !data.fail();
}

bool City_relation::has_treaty(Treaty_type type)
{
  return (type > TREATY_NULL && type < TREATY_MAX && treaty_days[type] > 0);
}

// *** Trade_deal ***

Trade_deal::Trade_deal()
{
  city_uid = -1;
  resource = RES_NULL;
  amount = 0;
  price = 0;
  importing = true;
  suspended = false;
}

std::string Trade_deal::save_data()
{
  std::stringstream ret;
  ret << city_uid << " " << int(resource) << " " << amount << " " << price <<
         " " << (importing ? 1 : 0) << " " << (suspended ? 1 : 0);
  return ret.str();
}

bool Trade_deal::load_data(std::istream& data)
{
  int tmpres, tmpimp, tmpsus;
  data >> city_uid >> tmpres >> amount >> price >> tmpimp >> tmpsus;
  if (tmpres <= RES_NULL || tmpres >= RES_MAX) {
    return false;
  }
  resource = Resource(tmpres);
  importing = (tmpimp != 0);
  suspended = (tmpsus != 0);
  return !data.fail();
}

// *** Realm ***

Realm::Realm()
{
  player = NULL;
  reset();
}

Realm::~Realm()
{
}

void Realm::reset()
{
  cities.clear();
  relations.clear();
  deals.clear();
  armies.clear();
  k_opinion.clear();
  k_war.clear();
  player_war.clear();
  rank = RANK_BARON;
  duke_uid = -1;
  favor_duke = 20;
  favor_king = 20;
  months_unpaid = 0;
  loan_owed = 0;
  months_of_reign = 0;
  excommunicated = false;
  demand_type = LIEGE_DEMAND_NULL;
  demand_amount = 0;
  demand_target = -1;
  demand_days = 0;
  tribute_today = 0;
  demand_seen = false;
  end_seen = false;
  game_end = GAME_END_NONE;
  end_text = "";
  start_year = 1400;
  next_army_uid = 0;
}

void Realm::link_cities()
{
  player = GAME->city;
  cities.clear();
  for (int k = 0; k < GAME->kingdoms.size(); k++) {
    Kingdom* kingdom = GAME->kingdoms[k];
    std::vector<City*> all;
    if (kingdom->capital) {
      all.push_back(kingdom->capital);
    }
    for (int i = 0; i < kingdom->dukes.size(); i++) {
      all.push_back(kingdom->dukes[i]);
    }
    for (int i = 0; i < kingdom->cities.size(); i++) {
      all.push_back(kingdom->cities[i]);
    }
    for (int i = 0; i < all.size(); i++) {
      AI_city* city = dynamic_cast<AI_city*>(all[i]);
      if (city) {
        city->owner_kingdom = kingdom->uid;
        cities.push_back(city);
      }
    }
  }
// Kingdoms start out liking each other as much as their races do.
  for (int a = 0; a < GAME->kingdoms.size(); a++) {
    for (int b = 0; b < GAME->kingdoms.size(); b++) {
      Kingdom* ka = GAME->kingdoms[a];
      Kingdom* kb = GAME->kingdoms[b];
      if (a == b) {
        continue;
      }
      int opinion = Race_data[ka->race]->relations[kb->race] * 10 +
                    Race_data[kb->race]->relations[ka->race] * 5;
      k_opinion[ka->uid][kb->uid] = opinion;
    }
  }
}

// Set up the cities' living state the first time: garrisons, stores.
static void first_day_for(AI_city* city)
{
  if (city->garrison.empty()) {
    city->recruit(city->get_garrison_target());
  }
  if (city->resources[RES_GOLD] <= 0) {
    city->resources[RES_GOLD] = city->get_daily_income() * 30;
  }
  if (city->resources[RES_FOOD] <= 0) {
    city->resources[RES_FOOD] = city->get_food_consumption() * 10;
  }
  city->plan_population = city->get_total_population();
  city->days_since_plan = rng(0, 89);
}

void Realm::start_new_game(Player_city* city)
{
  player = city;
  start_year = GAME->get_date().get_year();
  for (int i = 0; i < cities.size(); i++) {
    first_day_for(cities[i]);
    cities[i]->refresh_trade();
  }

// Your liege: the nearest duchy seat of your race's kingdom.
  Kingdom* kingdom = player_kingdom();
  duke_uid = -1;
  if (kingdom) {
    int best = -1;
    for (int i = 0; i < kingdom->dukes.size(); i++) {
      int dist = rl_dist(city->location, kingdom->dukes[i]->location);
      if (best == -1 || dist < best) {
        best = dist;
        duke_uid = kingdom->dukes[i]->uid;
      }
    }
  }

// What each city's people think of you, to begin with.
  Race our_race = city->get_race();
  for (int i = 0; i < cities.size(); i++) {
    AI_city* other = cities[i];
    City_relation rel;
    rel.opinion = Race_data[other->get_race()]->relations[our_race] * 8;
    if (kingdom && other->owner_kingdom == kingdom->uid) {
      rel.opinion += 15;
    }
    rel.opinion += rng(-5, 5);
    relations[other->uid] = rel;
  }
  log_info("Realm begun: %d cities, %d kingdoms, liege %d", cities.size(),
           GAME->kingdoms.size(), duke_uid);
}

bool Realm::save_to_file(std::string filename)
{
  std::ofstream fout;
  fout.open(filename.c_str());
  if (!fout.is_open()) {
    return false;
  }
  Date date = GAME->get_date();
  fout << "REALM " << REALM_SAVE_VERSION << std::endl;
  fout << date.save_data() << std::endl;
  fout << GAME->peek_city_uid() << std::endl;
  fout << int(rank) << " " << duke_uid << " " << favor_duke << " " <<
          favor_king << " " << months_unpaid << " " << loan_owed << " " <<
          months_of_reign << " " << (excommunicated ? 1 : 0) << std::endl;
  fout << int(demand_type) << " " << demand_amount << " " << demand_target <<
          " " << demand_days << std::endl;
  fout << int(game_end) << " " << start_year << " " << next_army_uid << " " <<
          (end_seen ? 1 : 0) << std::endl;
  fout << end_text << std::endl;

  fout << cities.size() << std::endl;
  for (int i = 0; i < cities.size(); i++) {
    fout << cities[i]->uid << std::endl << cities[i]->save_state();
  }

  fout << relations.size() << std::endl;
  for (std::map<int,City_relation>::iterator it = relations.begin();
       it != relations.end(); it++) {
    fout << it->first << " " << it->second.save_data() << std::endl;
  }

  fout << deals.size() << std::endl;
  for (int i = 0; i < deals.size(); i++) {
    fout << deals[i].save_data() << std::endl;
  }

  fout << armies.size() << std::endl;
  for (int i = 0; i < armies.size(); i++) {
    fout << armies[i].save_data() << std::endl;
  }

  int pairs = 0;
  std::stringstream ss_opinion, ss_war;
  for (std::map<int,std::map<int,int> >::iterator it = k_opinion.begin();
       it != k_opinion.end(); it++) {
    for (std::map<int,int>::iterator it2 = it->second.begin();
         it2 != it->second.end(); it2++) {
      ss_opinion << it->first << " " << it2->first << " " << it2->second << " ";
      pairs++;
    }
  }
  fout << pairs << " " << ss_opinion.str() << std::endl;
  pairs = 0;
  for (std::map<int,std::map<int,int> >::iterator it = k_war.begin();
       it != k_war.end(); it++) {
    for (std::map<int,int>::iterator it2 = it->second.begin();
         it2 != it->second.end(); it2++) {
      if (it2->second > 0) {
        ss_war << it->first << " " << it2->first << " " << it2->second << " ";
        pairs++;
      }
    }
  }
  fout << pairs << " " << ss_war.str() << std::endl;
  fout << player_war.size() << " ";
  for (std::map<int,int>::iterator it = player_war.begin();
       it != player_war.end(); it++) {
    fout << it->first << " " << it->second << " ";
  }
  fout << std::endl;

  fout.close();
  return true;
}

bool Realm::load_from_file(std::string filename)
{
  std::ifstream fin;
  fin.open(filename.c_str());
  if (!fin.is_open()) {
    return false;
  }
  std::string tag;
  int version;
  fin >> tag >> version;
  if (tag != "REALM" || version > REALM_SAVE_VERSION) {
    return false;
  }
  Date date;
  if (!date.load_data(fin)) {
    return false;
  }
  GAME->set_date(date);
  int next_uid;
  fin >> next_uid;
  if (next_uid > GAME->peek_city_uid()) {
    GAME->set_next_city_uid(next_uid);
  }
  int tmprank, tmpexcom;
  fin >> tmprank >> duke_uid >> favor_duke >> favor_king >> months_unpaid >>
         loan_owed >> months_of_reign >> tmpexcom;
  rank = Noble_rank(tmprank);
  excommunicated = (tmpexcom != 0);
  int tmpdemand;
  fin >> tmpdemand >> demand_amount >> demand_target >> demand_days;
  demand_type = Liege_demand_type(tmpdemand);
  int tmpend, tmpseen;
  fin >> tmpend >> start_year >> next_army_uid >> tmpseen;
  end_seen = (tmpseen != 0);
  game_end = Game_end(tmpend);
  chomp(fin);
  std::getline(fin, end_text);

  int num;
  fin >> num;
  for (int i = 0; i < num; i++) {
    int uid;
    fin >> uid;
    AI_city* city = lookup_city(uid);
    if (!city) {
      debugmsg("The realm has a city (uid %d) this world doesn't.", uid);
      return false;
    }
    if (!city->load_state(fin)) {
      debugmsg("Couldn't read the state of %s.", city->get_name().c_str());
      return false;
    }
  }

  fin >> num;
  for (int i = 0; i < num; i++) {
    int uid;
    fin >> uid;
    City_relation rel;
    if (!rel.load_data(fin)) {
      return false;
    }
    relations[uid] = rel;
  }

  fin >> num;
  for (int i = 0; i < num; i++) {
    Trade_deal deal;
    if (!deal.load_data(fin)) {
      return false;
    }
    deals.push_back(deal);
  }

  fin >> num;
  for (int i = 0; i < num; i++) {
    Army army;
    if (!army.load_data(fin)) {
      return false;
    }
    armies.push_back(army);
  }

  fin >> num;
  for (int i = 0; i < num; i++) {
    int a, b, v;
    fin >> a >> b >> v;
    k_opinion[a][b] = v;
  }
  fin >> num;
  for (int i = 0; i < num; i++) {
    int a, b, v;
    fin >> a >> b >> v;
    k_war[a][b] = v;
  }
  fin >> num;
  for (int i = 0; i < num; i++) {
    int k, v;
    fin >> k >> v;
    player_war[k] = v;
  }
  if (fin.fail()) {
    return false;
  }

  for (int i = 0; i < cities.size(); i++) {
    cities[i]->refresh_trade();
  }
  return true;
}

// *** Looking things up ***

AI_city* Realm::lookup_city(int uid)
{
  City* city = GAME->world->lookup_city_uid(uid);
  if (!city || city == player) {
    return NULL;
  }
  return dynamic_cast<AI_city*>(city);
}

Kingdom* Realm::lookup_kingdom(int uid)
{
  for (int i = 0; i < GAME->kingdoms.size(); i++) {
    if (GAME->kingdoms[i]->uid == uid) {
      return GAME->kingdoms[i];
    }
  }
  return NULL;
}

Kingdom* Realm::native_kingdom(City* city)
{
  if (!city) {
    return NULL;
  }
  for (int k = 0; k < GAME->kingdoms.size(); k++) {
    Kingdom* kingdom = GAME->kingdoms[k];
    if (kingdom->capital == city) {
      return kingdom;
    }
    for (int i = 0; i < kingdom->dukes.size(); i++) {
      if (kingdom->dukes[i] == city) {
        return kingdom;
      }
    }
    for (int i = 0; i < kingdom->cities.size(); i++) {
      if (kingdom->cities[i] == city) {
        return kingdom;
      }
    }
  }
  return NULL;
}

Kingdom* Realm::owner_kingdom(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai || ai->owner_kingdom < 0) {
    return NULL;
  }
  return lookup_kingdom(ai->owner_kingdom);
}

Kingdom* Realm::player_kingdom()
{
  if (!player) {
    return NULL;
  }
  return GAME->get_kingdom_for_race(player->get_race());
}

std::string Realm::city_title(City* city)
{
  if (!city) {
    return "nowhere";
  }
  if (city == player) {
    return "your city";
  }
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (ai && relation(ai).vassal) {
    return "your vassal";
  }
  Kingdom* native = native_kingdom(city);
  Kingdom* owner = owner_kingdom(city);
  std::string race_name = Race_data[city->get_race()]->plural_name;
  std::stringstream ss;
  switch (city->get_city_type()) {
    case CITY_TYPE_CAPITAL: ss << "capital of the " << race_name; break;
    case CITY_TYPE_DUCHY:   ss << "duchy seat of the " << race_name; break;
    default:                ss << "a city of the " << race_name; break;
  }
  if (owner && owner != native) {
    ss << ", held by the " << Race_data[owner->race]->plural_name;
  }
  return ss.str();
}

City_relation& Realm::relation(City* city)
{
  return relation(city ? city->uid : -1);
}

City_relation& Realm::relation(int uid)
{
  return relations[uid];
}

/* Whales' mockup of the diplomacy screen (screenshots/dialogue.png) calls a
 * well-liked lord an "Old Friend"; the rest of the scale is ours.
 */
std::string Realm::relationship_name(int opinion)
{
  if (opinion >= 75) {
    return "Old Friend";
  } else if (opinion >= 45) {
    return "Friend";
  } else if (opinion >= 15) {
    return "Cordial";
  } else if (opinion > -15) {
    return "Neutral";
  } else if (opinion > -45) {
    return "Wary";
  } else if (opinion > -75) {
    return "Hostile";
  }
  return "Sworn Enemy";
}

nc_color Realm::relationship_color(int opinion)
{
  if (opinion >= 45) {
    return c_ltgreen;
  } else if (opinion >= 15) {
    return c_green;
  } else if (opinion > -15) {
    return c_ltgray;
  } else if (opinion > -45) {
    return c_yellow;
  } else if (opinion > -75) {
    return c_ltred;
  }
  return c_red;
}

bool Realm::kingdom_at_war_with_player(int kingdom_uid)
{
  if (player_war.count(kingdom_uid) && player_war[kingdom_uid] > 0) {
    return true;
  }
  Kingdom* ours = player_kingdom();
  if (!ours) {
    return false;
  }
  if (kingdom_uid == ours->uid) {
    return excommunicated;
  }
// Your kingdom's wars are yours, unless you have been cast out of it.
  return (!excommunicated && kingdoms_at_war(ours->uid, kingdom_uid));
}

bool Realm::at_war_with_player(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai) {
    return false;
  }
  City_relation& rel = relation(ai);
  if (rel.vassal || ai->owner_kingdom < 0) {
    return false;
  }
  if (rel.at_war) {
    return true;
  }
  return kingdom_at_war_with_player(ai->owner_kingdom);
}

bool Realm::kingdoms_at_war(int a, int b)
{
  if (a == b || a < 0 || b < 0) {
    return false;
  }
  return ((k_war.count(a) && k_war[a].count(b) && k_war[a][b] > 0) ||
          (k_war.count(b) && k_war[b].count(a) && k_war[b][a] > 0));
}

int Realm::kingdom_opinion(int a, int b)
{
  if (k_opinion.count(a) && k_opinion[a].count(b)) {
    return k_opinion[a][b];
  }
  return 0;
}

int Realm::trade_days(City* city)
{
  if (!city || !player || !player->trade_routes.count(city->uid)) {
    return -1;
  }
  return (player->trade_routes[city->uid].distance + 99) / 100;
}

int Realm::route_overhead(City* city)
{
  if (!city || !player || !player->trade_routes.count(city->uid)) {
    return -1;
  }
  int overhead = player->trade_routes[city->uid].overhead;
  if (relation(city).has_treaty(TREATY_TRADE)) {
    overhead /= 2;
  }
  return overhead;
}

void Realm::player_message(int type, std::string text)
{
  if (player) {
    player->add_message(Message_type(type), "%s", text.c_str());
  }
  log_info("%s", text.c_str());
}

// *** Trade ***

void Realm::add_deal(Trade_deal deal)
{
  deals.push_back(deal);
  AI_city* city = lookup_city(deal.city_uid);
  if (city) {
    log_info("Deal: %s %d %s/day %s %s at %d per 100",
             deal.importing ? "buy" : "sell", deal.amount,
             Resource_data[deal.resource]->name.c_str(),
             deal.importing ? "from" : "to", city->get_name().c_str(),
             deal.price);
  }
}

void Realm::cancel_deal(int index)
{
  if (index >= 0 && index < deals.size()) {
    deals.erase(deals.begin() + index);
  }
}

void Realm::cancel_deals_with(City* city, std::string reason)
{
  int cancelled = 0;
  for (int i = 0; i < deals.size(); i++) {
    if (deals[i].city_uid == city->uid) {
      deals.erase(deals.begin() + i);
      i--;
      cancelled++;
    }
  }
  if (cancelled > 0 && !reason.empty()) {
    std::stringstream ss;
    ss << "Your trade with " << city->get_name() << " has ended: " << reason;
    player_message(MESSAGE_MAJOR, ss.str());
  }
}

void Realm::do_trade(Player_city* city)
{
  player = city;
  tribute_today = 0;
  bool blockaded = player_besieged();
  for (int i = 0; i < deals.size(); i++) {
    Trade_deal& deal = deals[i];
    AI_city* other = lookup_city(deal.city_uid);
    if (!other) {
      continue;
    }
    if (at_war_with_player(other)) {
      deal.suspended = true;
      continue;
    }
    if (deal.suspended || blockaded || other->besieged) {
      continue;
    }
    std::string res_name = Resource_data[deal.resource]->name;
    int gold = (deal.amount * deal.price + 50) / 100;
    if (deal.importing) {
      if (!city->expend_resource(RES_GOLD, gold)) {
        deal.suspended = true;
        std::stringstream ss;
        ss << "You could not pay for the " << res_name << " from " <<
              other->get_name() << "; that trade is suspended.";
        player_message(MESSAGE_MAJOR, ss.str());
        continue;
      }
      city->gain_resource(deal.resource, deal.amount);
      other->resources[RES_GOLD] += gold;
    } else {
      if (!city->expend_resource(deal.resource, deal.amount)) {
        deal.suspended = true;
        std::stringstream ss;
        ss << "You had no " << res_name << " to send to " <<
              other->get_name() << "; that trade is suspended.";
        player_message(MESSAGE_MAJOR, ss.str());
        continue;
      }
      if (other->resources[RES_GOLD] < gold) {
        deal.suspended = true;
        city->gain_resource(deal.resource, deal.amount);
        std::stringstream ss;
        ss << other->get_name() << " can no longer pay for your " <<
              res_name << "; that trade is suspended.";
        player_message(MESSAGE_MAJOR, ss.str());
        continue;
      }
      other->resources[RES_GOLD] -= gold;
      other->resources[RES_FOOD] += (deal.resource == RES_FOOD ? deal.amount :0);
      city->gain_resource(RES_GOLD, gold);
    }
  }

// Tribute from the cities you have conquered: a fifth of their taxes.
  for (int i = 0; i < cities.size(); i++) {
    AI_city* other = cities[i];
    if (relation(other).vassal && !other->besieged) {
      int tribute = other->get_daily_income() / 5;
      if (tribute > other->resources[RES_GOLD]) {
        tribute = other->resources[RES_GOLD];
      }
      other->resources[RES_GOLD] -= tribute;
      tribute_today += tribute;
    }
  }
  city->gain_resource(RES_GOLD, tribute_today);
}

int Realm::get_import(Resource res)
{
  int ret = 0;
  for (int i = 0; i < deals.size(); i++) {
    if (deals[i].suspended) {
      continue;
    }
    if (deals[i].importing && deals[i].resource == res) {
      ret += deals[i].amount;
    }
  }
  if (res == RES_GOLD) {
    ret += get_trade_income();
  }
  return ret;
}

int Realm::get_export(Resource res)
{
  int ret = 0;
  for (int i = 0; i < deals.size(); i++) {
    if (deals[i].suspended) {
      continue;
    }
    if (!deals[i].importing && deals[i].resource == res) {
      ret += deals[i].amount;
    }
  }
  if (res == RES_GOLD) {
    ret += get_trade_expense();
  }
  return ret;
}

int Realm::get_trade_income()
{
  int ret = 0;
  for (int i = 0; i < deals.size(); i++) {
    if (!deals[i].suspended && !deals[i].importing) {
      ret += (deals[i].amount * deals[i].price + 50) / 100;
    }
  }
  return ret + get_tribute();
}

int Realm::get_tribute()
{
  int ret = 0;
  for (int i = 0; i < cities.size(); i++) {
    if (relation(cities[i]).vassal) {
      ret += cities[i]->get_daily_income() / 5;
    }
  }
  return ret;
}

int Realm::get_trade_expense()
{
  int ret = 0;
  for (int i = 0; i < deals.size(); i++) {
    if (!deals[i].suspended && deals[i].importing) {
      ret += (deals[i].amount * deals[i].price + 50) / 100;
    }
  }
  return ret;
}

// *** Diplomacy ***

/* Whales, on the blog (post 109241429666): "the price of diplomatic
 * agreements is explicit... there's no 'barter' button".  So every treaty has
 * a price in gold, and the city tells you what it is.
 */
int Realm::treaty_price(City* city, Treaty_type type)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai || at_war_with_player(ai) || relation(ai).vassal) {
    return -1;
  }
  int opinion = relation(ai).opinion;
  int base = 0, need = 0;
  switch (type) {
    case TREATY_TRADE:    base = 150; need = -30; break;
    case TREATY_PEACE:    base = 300; need = -60; break;
    case TREATY_ALLIANCE: base = 900; need =  30; break;
    default: return -1;
  }
  if (opinion < need) {
    return -1;
  }
// An alliance is only worth something from a city with soldiers to send.
  if (type == TREATY_ALLIANCE && ai->get_city_type() == CITY_TYPE_CITY &&
      ai->garrison.total() < 50) {
    return -1;
  }
  int pop = ai->get_total_population();
  int price = base + (base * pop) / 3000;
  int discount = 100 - opinion;   // 0 to 200
  price = (price * discount) / 100;
  if (price < base / 10) {
    price = base / 10;
  }
  return price;
}

bool Realm::sign_treaty(City* city, Treaty_type type)
{
  int price = treaty_price(city, type);
  if (price < 0 || !player->expend_resource(RES_GOLD, price)) {
    return false;
  }
  AI_city* ai = dynamic_cast<AI_city*>(city);
  ai->resources[RES_GOLD] += price;
  City_relation& rel = relation(ai);
  rel.treaty_days[type] = TREATY_LENGTH;
  rel.opinion += 5;
  if (rel.opinion > 100) {
    rel.opinion = 100;
  }
  std::stringstream ss;
  ss << "You signed a " << treaty_name(type) << " with " << ai->get_name() <<
        " for a year.";
  player_message(MESSAGE_MINOR, ss.str());
  return true;
}

int Realm::give_gift(City* city, int gold)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai || gold <= 0 || !player->expend_resource(RES_GOLD, gold)) {
    return 0;
  }
  ai->resources[RES_GOLD] += gold;
  City_relation& rel = relation(ai);
  int pop = ai->get_total_population();
  int gain = (gold * 20) / (pop / 5 + 200);
// Gifts close together are worth less; the people grow used to them.
  if (rel.gift_days < 60) {
    gain /= 2;
  }
  if (gain > 30) {
    gain = 30;
  }
  rel.opinion += gain;
  if (rel.opinion > 100) {
    rel.opinion = 100;
  }
  rel.gift_days = 0;
// A gift to your own lords buys favor too.
  Kingdom* ours = player_kingdom();
  if (!excommunicated && ours && ai->owner_kingdom == ours->uid) {
    int favor = gold / 100;
    if (favor > 10) {
      favor = 10;
    }
    if (ai == get_liege()) {
      change_liege_favor(favor);
    } else if (ai->get_city_type() == CITY_TYPE_CAPITAL) {
      favor_king += favor;
      if (favor_king > 100) {
        favor_king = 100;
      }
    }
  }
  log_info("Gift of %d to %s: opinion +%d", gold, ai->get_name().c_str(), gain);
  return gain;
}

int Realm::demand_tribute(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai) {
    return 0;
  }
  City_relation& rel = relation(ai);
  rel.demand_days = 90;
  int theirs = garrison_strength(ai);
  int ours = player_strength();
  bool pays = false;
  if (ours > theirs * 2) {
    pays = true;
  } else if (ours > theirs) {
    pays = one_in(2);
  }
  if (rel.has_treaty(TREATY_ALLIANCE)) {
    pays = false; // Friends don't
  }
  if (!pays) {
    rel.opinion -= 10;
// A warlike city may answer with steel.
    int warlike = Race_data[ai->get_race()]->warlikeness;
    if (warlike >= 2 && rel.truce_days <= 0 && one_in(3)) {
      rel.at_war = true;
      std::stringstream ss;
      ss << ai->get_name() << " answers your demand with a declaration of war!";
      player_message(MESSAGE_URGENT, ss.str());
      cancel_deals_with(ai, "they are at war with you.");
    }
    return 0;
  }
  int amount = 50 + ai->get_total_population() / 5;
  if (amount > ai->resources[RES_GOLD] / 3) {
    amount = ai->resources[RES_GOLD] / 3;
  }
  if (amount < 0) {
    amount = 0;
  }
  ai->resources[RES_GOLD] -= amount;
  player->gain_resource(RES_GOLD, amount);
  rel.opinion -= 20;
  if (rel.opinion < -100) {
    rel.opinion = -100;
  }
  return amount;
}

void Realm::declare_war(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai || at_war_with_player(ai)) {
    return;
  }
  Kingdom* ours = player_kingdom();
  Kingdom* theirs = owner_kingdom(ai);
  bool broke_pact = relation(ai).has_treaty(TREATY_PEACE) ||
                    relation(ai).has_treaty(TREATY_ALLIANCE);
  std::stringstream ss;

  if (!theirs || (ours && theirs->uid == ours->uid)) {
// War inside your own kingdom: between you and this city alone.
    City_relation& rel = relation(ai);
    rel.at_war = true;
    for (int i = 0; i < TREATY_MAX; i++) {
      rel.treaty_days[i] = 0;
    }
    rel.opinion -= 50;
    cancel_deals_with(ai, "you are at war.");
    ss << "You have declared war on " << ai->get_name() << ".";
    player_message(MESSAGE_URGENT, ss.str());
// How your lords take a war among their own (notes/fealty.txt).
    if (ours) {
      Fealty_datum fealty = get_fealty(ours->race);
      switch (fealty.infighting) {
        case 0:
// Orcs think the less of a city that never fights its neighbors.
          change_liege_favor(5);
          favor_king += 2;
          break;
        case 1:
          change_liege_favor(-10);
          favor_king -= 5;
          break;
        case 2:
          change_liege_favor(-20);
          favor_king -= 15;
          player_message(MESSAGE_MAJOR, "Your lords frown on war between \
their own cities.  They may come to the aid of the city you attack.");
          break;
        default:
          change_liege_favor(-30);
          favor_king -= 25;
          player_message(MESSAGE_URGENT, "Your lords will not stand for war \
between their own cities, and will march to defend the city you attack.");
          break;
      }
    }
  } else {
// War with a city of another kingdom is war with all of it.
    player_war[theirs->uid] = 1;
    for (int i = 0; i < cities.size(); i++) {
      AI_city* other = cities[i];
      if (other->owner_kingdom == theirs->uid) {
        City_relation& rel = relation(other);
        if (rel.has_treaty(TREATY_PEACE) || rel.has_treaty(TREATY_ALLIANCE)) {
          broke_pact = true;
        }
        for (int t = 0; t < TREATY_MAX; t++) {
          rel.treaty_days[t] = 0;
        }
        rel.opinion -= (other == ai ? 50 : 30);
        if (rel.opinion < -100) {
          rel.opinion = -100;
        }
        cancel_deals_with(other, "you are at war with the " +
                          Race_data[theirs->race]->plural_name + ".");
      }
    }
    ss << "You have declared war on " << ai->get_name() << ", and so on all " <<
          "the " << Race_data[theirs->race]->plural_name << ".";
    player_message(MESSAGE_URGENT, ss.str());
    if (ours) {
      if (kingdoms_at_war(ours->uid, theirs->uid)) {
        change_liege_favor(5);
      } else if (kingdom_opinion(ours->uid, theirs->uid) > -20) {
        change_liege_favor(-10);
        favor_king -= 10;
        player_message(MESSAGE_MAJOR, "Your lords are displeased that you \
have dragged the kingdom toward a war it did not want.");
      }
    }
  }
// Breaking your word is noticed everywhere.
  if (broke_pact) {
    for (std::map<int,City_relation>::iterator it = relations.begin();
         it != relations.end(); it++) {
      it->second.opinion -= 10;
    }
    player_message(MESSAGE_MAJOR, "You broke a pact to make this war.  Every \
city in the world thinks less of your word.");
  }
  log_info("War declared on %s", ai->get_name().c_str());
}

int Realm::peace_price(City* city, bool& refuse)
{
  refuse = false;
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai || !at_war_with_player(ai)) {
    refuse = true;
    return 0;
  }
  City_relation& rel = relation(ai);
  Kingdom* theirs = owner_kingdom(ai);
// A war your kingdom is fighting is for your king to end.
  if (!rel.at_war && theirs &&
      !(player_war.count(theirs->uid) && player_war[theirs->uid] > 0)) {
    refuse = true;
    return 0;
  }
  int days = (rel.at_war ? 999 :
              (theirs && player_war.count(theirs->uid) ?
               player_war[theirs->uid] : 999));
  if (days < 20) {
    refuse = true;  // The war has barely begun
    return 0;
  }
  int theirs_str = garrison_strength(ai);
  if (theirs) {
    theirs_str = 0;
    for (int i = 0; i < cities.size(); i++) {
      if (cities[i]->owner_kingdom == theirs->uid) {
        theirs_str += garrison_strength(cities[i]);
      }
    }
    theirs_str /= 4;  // Only some of it could ever reach you
  }
  int ours = player_strength();
  int base = 100 + ai->get_total_population() / 10;
  if (player_besieged()) {
    return base * 4;  // A ransom for your city
  }
// If you have one of their cities under siege, they will pay to end it.
  for (int i = 0; i < armies.size(); i++) {
    if (armies[i].player_owned && armies[i].order == ARMY_ORDER_SIEGE) {
      AI_city* besieged = lookup_city(armies[i].target_uid);
      if (besieged && (besieged == ai || (theirs &&
                       besieged->owner_kingdom == theirs->uid))) {
        return 0 - base;
      }
    }
  }
  if (ours > theirs_str * 3 / 2) {
    return 0 - base / 2;
  } else if (theirs_str > ours * 3 / 2) {
    return base * 2;
  }
  return base;
}

bool Realm::make_peace(City* city)
{
  bool refuse;
  int price = peace_price(city, refuse);
  if (refuse) {
    return false;
  }
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (price > 0) {
    if (!player->expend_resource(RES_GOLD, price)) {
      return false;
    }
    ai->resources[RES_GOLD] += price;
  } else if (price < 0) {
    int paid = 0 - price;
    if (paid > ai->resources[RES_GOLD]) {
      paid = ai->resources[RES_GOLD];
    }
    ai->resources[RES_GOLD] -= paid;
    player->gain_resource(RES_GOLD, paid);
  }
  Kingdom* theirs = owner_kingdom(ai);
  std::vector<AI_city*> at_peace;
  if (relation(ai).at_war) {
    relation(ai).at_war = false;
    at_peace.push_back(ai);
  } else if (theirs) {
    player_war.erase(theirs->uid);
    for (int i = 0; i < cities.size(); i++) {
      if (cities[i]->owner_kingdom == theirs->uid) {
        at_peace.push_back(cities[i]);
      }
    }
  }
  for (int i = 0; i < at_peace.size(); i++) {
    relation(at_peace[i]).truce_days = 180;
    relation(at_peace[i]).opinion += 10;
  }
// Armies on both sides go home.
  for (int i = 0; i < armies.size(); i++) {
    Army& army = armies[i];
    AI_city* target = lookup_city(army.target_uid);
    bool ends = false;
    if (army.player_owned && target) {
      for (int n = 0; n < at_peace.size(); n++) {
        if (at_peace[n] == target) {
          ends = true;
        }
      }
    } else if (!army.player_owned && army.target_uid == player->uid) {
      AI_city* home = lookup_city(army.home_uid);
      for (int n = 0; n < at_peace.size(); n++) {
        if (at_peace[n] == home ||
            (theirs && army.kingdom_uid == theirs->uid)) {
          ends = true;
        }
      }
    }
    if (ends && army.order != ARMY_ORDER_RETURN) {
      if (target) {
        target->besieged = false;
      }
      recall_army(i);
    }
  }
  std::stringstream ss;
  ss << "Peace with " << ai->get_name();
  if (price > 0) {
    ss << ", for " << price << " gold.";
  } else if (price < 0) {
    ss << "; they pay you " << (0 - price) << " gold to end the war.";
  } else {
    ss << ".";
  }
  player_message(MESSAGE_MAJOR, ss.str());
  return true;
}

// *** War ***

int Realm::player_strength()
{
  Troops troops = player->get_troops();
  for (int i = 0; i < armies.size(); i++) {
    if (armies[i].player_owned) {
      troops.add(armies[i].troops);
    }
  }
  return troops_strength(troops, player->get_race());
}

int Realm::garrison_strength(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai) {
    return 0;
  }
  return troops_strength(ai->garrison, ai->get_race());
}

// Walls and household guards: how much harder a city is to take.
static int defense_bonus_for(City* city)
{
  if (!city) {
    return 100;
  }
  switch (city->get_city_type()) {
    case CITY_TYPE_CAPITAL: return 200;
    case CITY_TYPE_DUCHY:   return 160;
    default:                return 130;
  }
}

// Days a city holds out under siege once its garrison is beaten.
static int siege_length(City* city)
{
  if (!city) {
    return 10;
  }
  if (city->is_player_city()) {
    return 12;
  }
  switch (city->get_city_type()) {
    case CITY_TYPE_CAPITAL: return 30;
    case CITY_TYPE_DUCHY:   return 20;
    default:                return 10;
  }
}

int Realm::siege_days_left(City* city)
{
  for (int i = 0; i < armies.size(); i++) {
    if (armies[i].order == ARMY_ORDER_SIEGE && city &&
        armies[i].target_uid == city->uid) {
      int left = siege_length(city) - armies[i].siege_days;
      return (left > 0 ? left : 0);
    }
  }
  return -1;
}

bool Realm::player_besieged()
{
  if (!player) {
    return false;
  }
  for (int i = 0; i < armies.size(); i++) {
    if (!armies[i].player_owned && armies[i].order == ARMY_ORDER_SIEGE &&
        armies[i].target_uid == player->uid) {
      return true;
    }
  }
  return false;
}

bool Realm::plan_path(Army& army, Point dest)
{
  army.path = GAME->world->travel_route(army.race, army.pos, dest);
  if (army.path.empty() && army.pos != dest) {
    return false;
  }
  return true;
}

bool Realm::send_army(Troops troops, City* target)
{
  if (!target || troops.total() <= 0) {
    return false;
  }
  Army army;
  army.uid = next_army_uid++;
  army.player_owned = true;
  army.home_uid = player->uid;
  army.target_uid = target->uid;
  army.kingdom_uid = -1;
  army.race = player->get_race();
  army.order = ARMY_ORDER_ATTACK;
  army.troops = troops;
  army.pos = player->location;
  if (!plan_path(army, target->location)) {
    return false;
  }
// Each soldier carries a day's food for every day out, and ten more.
  int cost = 0;
  for (int i = 0; i < army.path.size(); i++) {
    cost += GAME->world->travel_step_cost(army.race, army.path[i]);
  }
  int days = cost / 100 + 1;
  army.supplies = days * 2 + 10;
  int food = troops.total() * army.supplies;
  if (!player->expend_resource(RES_FOOD, food)) {
    return false;
  }
  Troops home = player->get_troops();
  for (int i = 0; i < UNIT_MAX; i++) {
    home.count[i] -= troops.count[i];
    if (home.count[i] < 0) {
      home.count[i] = 0;
    }
  }
  player->set_troops(home);
  armies.push_back(army);
  log_info("Army of %d sent to %s (%d days)", troops.total(),
           target->get_name().c_str(), days);
  return true;
}

void Realm::recall_army(int index)
{
  if (index < 0 || index >= armies.size()) {
    return;
  }
  Army& army = armies[index];
  City* home = GAME->world->lookup_city_uid(army.home_uid);
  if (army.order == ARMY_ORDER_SIEGE) {
    AI_city* target = lookup_city(army.target_uid);
    if (target) {
      target->besieged = false;
    }
  }
  army.order = ARMY_ORDER_RETURN;
  army.siege_days = 0;
  if (home) {
    if (!plan_path(army, home->location)) {
      army.path.clear();
    }
  } else {
    army.path.clear();
  }
}

void Realm::move_armies()
{
  for (int i = 0; i < armies.size(); i++) {
    Army& army = armies[i];
    army.days_out++;
    if (army.player_owned) {
      if (army.supplies > 0) {
        army.supplies--;
      } else if (army.troops.total() > 0) {
// Out of food, soldiers slip away home.
        int leaving = army.troops.total() / 30 + 1;
        Troops gone;
        for (int u = 1; u < UNIT_MAX && leaving > 0; u++) {
          int n = (leaving < army.troops.count[u] ? leaving :
                                                    army.troops.count[u]);
          army.troops.count[u] -= n;
          gone.count[u] += n;
          leaving -= n;
        }
        for (int u = 1; u < UNIT_MAX; u++) {
          player->population[ Unit_data[u].citizen ].add_citizens(gone.count[u]);
        }
        if (army.days_out % 5 == 0) {
          player_message(MESSAGE_MAJOR, "Your army has run out of food; \
soldiers are leaving it to walk home.");
        }
      }
    }
    if (army.order == ARMY_ORDER_SIEGE) {
      siege_turn(army);
      continue;
    }
    army.travel_points += 100;
    while (!army.path.empty()) {
      int cost = GAME->world->travel_step_cost(army.race, army.path[0]);
      if (army.travel_points < cost) {
        break;
      }
      army.travel_points -= cost;
      army.pos = army.path[0];
      army.path.erase(army.path.begin());
    }
    if (army.path.empty()) {
      army.travel_points = 0;
      army_arrives(army);
    }
  }
// Clear away the armies that are gone.
  for (int i = 0; i < armies.size(); i++) {
    if (armies[i].troops.total() <= 0 || armies[i].order == ARMY_ORDER_NULL) {
      armies.erase(armies.begin() + i);
      i--;
    }
  }
}

void Realm::army_arrives(Army& army)
{
  City* target = GAME->world->lookup_city_uid(army.target_uid);
  City* home = GAME->world->lookup_city_uid(army.home_uid);

  if (army.order == ARMY_ORDER_RETURN) {
    if (army.player_owned) {
      player->soldiers_home(army.troops);
      std::stringstream ss;
      ss << "Your army is home: " << army.troops.total() << " soldiers.";
      player_message(MESSAGE_MINOR, ss.str());
// Coming home to a city under siege, they fight for it.
      for (int i = 0; i < armies.size(); i++) {
        if (!armies[i].player_owned && armies[i].order == ARMY_ORDER_SIEGE &&
            armies[i].target_uid == player->uid) {
          armies[i].siege_days -= 2;
        }
      }
    } else {
      AI_city* ai_home = dynamic_cast<AI_city*>(home);
      if (ai_home && ai_home->owner_kingdom == army.kingdom_uid) {
        ai_home->garrison.add(army.troops);
      }
    }
    army.troops.clear();
    army.order = ARMY_ORDER_NULL;
    return;
  }

  if (!target) {
    recall_army(&army - &armies[0]);
    return;
  }

  if (army.order == ARMY_ORDER_RELIEVE) {
// Allies who come to help join the defenders.
    AI_city* ai_target = dynamic_cast<AI_city*>(target);
    if (target == player) {
      std::stringstream ss;
      ss << army.get_name() << " has come to your aid with " <<
            army.troops.total() << " soldiers!";
      player_message(MESSAGE_MAJOR, ss.str());
// Fight any besiegers.
      for (int i = 0; i < armies.size(); i++) {
        Army& enemy = armies[i];
        if (!enemy.player_owned && enemy.order == ARMY_ORDER_SIEGE &&
            enemy.target_uid == player->uid && enemy.troops.total() > 0) {
          Battle_report rep = fight_battle(army.troops, army.race,
                                           enemy.troops, enemy.race, 100);
          std::stringstream ss2;
          if (rep.attacker_won) {
            ss2 << "Your allies drove off " << enemy.get_name() << "! " <<
                   rep.summary;
            enemy.order = ARMY_ORDER_ATTACK;
            recall_army(i);
          } else {
            ss2 << "Your allies could not break the siege. " << rep.summary;
          }
          player_message(MESSAGE_MAJOR, ss2.str());
          break;
        }
      }
      AI_city* ai_home = dynamic_cast<AI_city*>(home);
      if (ai_home) {
        army.order = ARMY_ORDER_ATTACK;
        recall_army(&army - &armies[0]);
      } else {
        army.troops.clear();
        army.order = ARMY_ORDER_NULL;
      }
    } else if (ai_target) {
      ai_target->garrison.add(army.troops);
      army.troops.clear();
      army.order = ARMY_ORDER_NULL;
    }
    return;
  }

// ARMY_ORDER_ATTACK
// Raiders: they fight for your stores, and take what they can if they win.
  if (army.home_uid < 0 && target == player) {
    Troops defenders = player->get_troops();
    Battle_report rep = fight_battle(army.troops, army.race, defenders,
                                     player->get_race(), 130);
    player->set_troops(defenders);
    std::stringstream ss;
    if (rep.attacker_won) {
      int gold = player->get_resource_amount(RES_GOLD) / 5;
      int food = player->get_resource_amount(RES_FOOD) / 5;
      player->expend_resource(RES_GOLD, gold);
      player->expend_resource(RES_FOOD, food);
      ss << "Raiders broke into " << player->get_name() << " and carried " <<
            "off " << gold << " gold and " << food << " food. " << rep.summary;
      player_message(MESSAGE_URGENT, ss.str());
    } else {
      ss << "Your soldiers drove off the raiders! " << rep.summary;
      player_message(MESSAGE_MAJOR, ss.str());
    }
    army.troops.clear();
    army.order = ARMY_ORDER_NULL;
    return;
  }

  bool still_enemy;
  if (army.player_owned) {
    still_enemy = at_war_with_player(target);
  } else if (target == player) {
    AI_city* ai_home = dynamic_cast<AI_city*>(home);
    still_enemy = (ai_home ? at_war_with_player(ai_home) :
                   kingdom_at_war_with_player(army.kingdom_uid));
  } else {
    AI_city* ai_target = dynamic_cast<AI_city*>(target);
    still_enemy = (ai_target && !relation(ai_target).vassal &&
                   kingdoms_at_war(army.kingdom_uid, ai_target->owner_kingdom));
    if (ai_target && relation(ai_target).vassal) {
// Your vassals are yours to defend.
      still_enemy = kingdom_at_war_with_player(army.kingdom_uid);
    }
  }
  if (!still_enemy) {
    recall_army(&army - &armies[0]);
    return;
  }

  std::stringstream ss;
  if (target == player) {
    Troops defenders = player->get_troops();
// The player's walls: the keep, and each barracks makes the city harder.
    int bonus = 130;
    Battle_report rep = fight_battle(army.troops, army.race, defenders,
                                     player->get_race(), bonus);
    player->set_troops(defenders);
    if (rep.attacker_won) {
      ss << army.get_name() << " has beaten your defenders and laid siege " <<
            "to your city! " << rep.summary << "  Your keep will hold for " <<
            siege_length(player) << " days.";
      player_message(MESSAGE_URGENT, ss.str());
      army.order = ARMY_ORDER_SIEGE;
      army.siege_days = 0;
      cancel_deals_with(target, "");
    } else {
      ss << "Your soldiers drove " << army.get_name() << " from your walls! " <<
            rep.summary;
      player_message(MESSAGE_MAJOR, ss.str());
      recall_army(&army - &armies[0]);
    }
    return;
  }

  AI_city* ai_target = dynamic_cast<AI_city*>(target);
  if (!ai_target) {
    recall_army(&army - &armies[0]);
    return;
  }
  Battle_report rep = fight_battle(army.troops, army.race, ai_target->garrison,
                                   ai_target->get_race(),
                                   defense_bonus_for(ai_target));
  if (army.player_owned) {
    if (rep.attacker_won) {
      ss << "Your army defeated the garrison of " << ai_target->get_name() <<
            " and lays siege to it. " << rep.summary << "  It will fall in " <<
            siege_length(ai_target) << " days.";
      army.order = ARMY_ORDER_SIEGE;
      army.siege_days = 0;
      ai_target->besieged = true;
      player_message(MESSAGE_MAJOR, ss.str());
    } else {
      ss << "Your army was beaten at " << ai_target->get_name() <<
            " and falls back toward home. " << rep.summary;
      player_message(MESSAGE_URGENT, ss.str());
      recall_army(&army - &armies[0]);
    }
  } else {
    if (rep.attacker_won) {
      army.order = ARMY_ORDER_SIEGE;
      army.siege_days = 0;
      ai_target->besieged = true;
      if (relation(ai_target).vassal) {
        ss << army.get_name() << " has laid siege to your vassal " <<
              ai_target->get_name() << ".";
        player_message(MESSAGE_URGENT, ss.str());
      }
      send_relief(ai_target);
    } else {
      recall_army(&army - &armies[0]);
    }
    log_debug("%s attacks %s: %s", army.get_name().c_str(),
              ai_target->get_name().c_str(), rep.summary.c_str());
  }
}

void Realm::siege_turn(Army& army)
{
  City* target = GAME->world->lookup_city_uid(army.target_uid);
  if (!target) {
    army.order = ARMY_ORDER_ATTACK;
    recall_army(&army - &armies[0]);
    return;
  }
  army.siege_days++;

  if (target == player) {
// Soldiers raised inside the walls sally out to fight.
    Troops defenders = player->get_troops();
    if (defenders.fighters() > 0 && army.siege_days % 3 == 0) {
      Battle_report rep = fight_battle(army.troops, army.race, defenders,
                                       player->get_race(), 130);
      player->set_troops(defenders);
      std::stringstream ss;
      if (!rep.attacker_won) {
        ss << "Your soldiers sallied out and broke the siege! " << rep.summary;
        player_message(MESSAGE_MAJOR, ss.str());
        army.order = ARMY_ORDER_ATTACK;
        recall_army(&army - &armies[0]);
        return;
      }
      ss << "Your soldiers sallied out but could not break the siege. " <<
            rep.summary;
      player_message(MESSAGE_MAJOR, ss.str());
    }
    int left = siege_length(player) - army.siege_days;
    if (left <= 0) {
      std::stringstream ss;
      ss << army.get_name() << " has taken your keep.  " << player->get_name() <<
            " has fallen, and you are their prisoner.";
      end_game(GAME_END_FALLEN, ss.str());
    } else if (left <= 3 || army.siege_days % 4 == 0) {
      std::stringstream ss;
      ss << "Your city is besieged by " << army.get_name() << " (" <<
            army.troops.total() << " soldiers).  The keep can hold " << left <<
            " more days.";
      player_message(MESSAGE_URGENT, ss.str());
    }
    return;
  }

  AI_city* ai_target = dynamic_cast<AI_city*>(target);
  if (!ai_target) {
    army.order = ARMY_ORDER_ATTACK;
    recall_army(&army - &armies[0]);
    return;
  }
  ai_target->besieged = true;
// A rebuilt garrison (or relief that arrived) fights again.
  if (ai_target->garrison.fighters() > 0 && army.siege_days % 3 == 0) {
    Battle_report rep = fight_battle(army.troops, army.race,
                                     ai_target->garrison,
                                     ai_target->get_race(),
                                     defense_bonus_for(ai_target));
    if (!rep.attacker_won) {
      if (army.player_owned) {
        std::stringstream ss;
        ss << "The defenders of " << ai_target->get_name() << " broke your " <<
              "siege! " << rep.summary;
        player_message(MESSAGE_URGENT, ss.str());
      }
      ai_target->besieged = false;
      army.order = ARMY_ORDER_ATTACK;
      recall_army(&army - &armies[0]);
      return;
    }
  }
// A city that runs out of food gives up sooner.
  int needed = siege_length(ai_target);
  if (ai_target->resources[RES_FOOD] <= 0) {
    needed = needed / 2;
  }
  if (army.siege_days >= needed) {
    conquer(army, ai_target);
  } else if (army.player_owned && army.siege_days % 5 == 0) {
    std::stringstream ss;
    ss << "The siege of " << ai_target->get_name() << " goes on: " <<
          needed - army.siege_days << " days until it falls.";
    player_message(MESSAGE_MINOR, ss.str());
  }
}

void Realm::conquer(Army& army, City* target)
{
  AI_city* ai_target = dynamic_cast<AI_city*>(target);
  if (!ai_target) {
    return;
  }
  ai_target->besieged = false;
// Some of the people flee before the new masters.
  ai_target->lose_citizens(ai_target->get_total_population() / 10);
  City_relation& rel = relation(ai_target);
  std::stringstream ss;

  if (army.player_owned) {
    int conquered_from = ai_target->owner_kingdom;
    make_vassal(ai_target);
// A quarter of the army stays to hold it; the rest march home.
    Troops stay;
    for (int i = 1; i < UNIT_MAX; i++) {
      stay.count[i] = army.troops.count[i] / 4;
      army.troops.count[i] -= stay.count[i];
    }
    ai_target->garrison = stay;
    ss << ai_target->get_name() << " has fallen to your army!  It is your " <<
          "vassal now, and pays you tribute.";
    player_message(MESSAGE_MAJOR, ss.str());
// Taking a city from your kingdom's enemies pleases the crown.
    Kingdom* ours = player_kingdom();
    Kingdom* taken_from = (army.target_uid >= 0 ?
                           lookup_kingdom(conquered_from) : NULL);
    if (ours && taken_from && taken_from != ours && !excommunicated &&
        kingdoms_at_war(ours->uid, taken_from->uid)) {
      change_liege_favor(10);
      favor_king += 10;
      if (favor_king > 100) {
        favor_king = 100;
      }
      player_message(MESSAGE_MINOR, "The crown is pleased with your victory.");
    }
    army.order = ARMY_ORDER_ATTACK;
    recall_army(&army - &armies[0]);
    return;
  }

  bool was_vassal = rel.vassal;
  Kingdom* old_owner = owner_kingdom(ai_target);
  Kingdom* new_owner = lookup_kingdom(army.kingdom_uid);
  rel.vassal = false;
  ai_target->owner_kingdom = army.kingdom_uid;
  ai_target->garrison = army.troops;
  army.troops.clear();
  army.order = ARMY_ORDER_NULL;
  cancel_deals_with(ai_target, (new_owner && at_war_with_player(ai_target)) ?
                    "it has fallen to your enemies." : "");
  if (was_vassal) {
    ss << "Your vassal " << ai_target->get_name() << " has fallen to the " <<
          (new_owner ? Race_data[new_owner->race]->plural_name : "rebels") <<
          ".";
    player_message(MESSAGE_URGENT, ss.str());
  } else if (player->world_seen.is_seen(ai_target->location)) {
    ss << ai_target->get_name() << " has fallen to the " <<
          (new_owner ? Race_data[new_owner->race]->plural_name : "rebels");
    if (old_owner) {
      ss << ", taken from the " << Race_data[old_owner->race]->plural_name;
    }
    ss << ".";
    player_message(MESSAGE_MINOR, ss.str());
  }
}

void Realm::make_vassal(City* city)
{
  AI_city* ai = dynamic_cast<AI_city*>(city);
  if (!ai) {
    return;
  }
  Kingdom* old_owner = owner_kingdom(ai);
  City_relation& rel = relation(ai);
  rel.vassal = true;
  rel.at_war = false;
  rel.opinion -= 20;
  if (rel.opinion < -100) {
    rel.opinion = -100;
  }
  ai->owner_kingdom = -1;
  cancel_deals_with(ai, "");
  if (old_owner) {
    log_debug("%s taken from kingdom %d", ai->get_name().c_str(),
              old_owner->uid);
  }
}

// Allies and lords send soldiers to a city under siege.
void Realm::send_relief(City* besieged)
{
  if (!besieged) {
    return;
  }
  AI_city* ai_besieged = dynamic_cast<AI_city*>(besieged);
  int kingdom_uid = (ai_besieged ? ai_besieged->owner_kingdom : -1);
  int sent = 0;
  for (int i = 0; i < cities.size() && sent < 2; i++) {
    AI_city* helper = cities[i];
    if (helper == besieged || helper->besieged ||
        helper->get_city_type() == CITY_TYPE_CITY) {
      continue;
    }
    bool helps = false;
    if (besieged == player) {
      City_relation& rel = relation(helper);
      if (rel.has_treaty(TREATY_ALLIANCE) && !at_war_with_player(helper)) {
        helps = true;
      }
// Your liege, if they think well enough of you (notes/fealty.txt).
      if (helper->uid == duke_uid ||
          (duke_uid < 0 && helper->get_city_type() == CITY_TYPE_CAPITAL &&
           player_kingdom() && helper->owner_kingdom == player_kingdom()->uid)) {
        Fealty_datum fealty = get_fealty(helper->get_race());
        if (!excommunicated && get_liege_favor() >= 30 - fealty.military_aid * 10
            && !at_war_with_player(helper)) {
          helps = true;
        }
      }
    } else if (kingdom_uid >= 0 && helper->owner_kingdom == kingdom_uid) {
      helps = one_in(2);
    }
    if (!helps) {
      continue;
    }
    int days = GAME->world->route_cost(helper->get_race(), helper->location,
                                       besieged->location) / 100;
    if (days < 0 || days > 25) {
      continue;
    }
    Army army;
    army.uid = next_army_uid++;
    army.player_owned = false;
    army.home_uid = helper->uid;
    army.target_uid = besieged->uid;
    army.kingdom_uid = helper->owner_kingdom;
    army.race = helper->get_race();
    army.order = ARMY_ORDER_RELIEVE;
    for (int u = 1; u < UNIT_MAX; u++) {
      army.troops.count[u] = helper->garrison.count[u] / 3;
      helper->garrison.count[u] -= army.troops.count[u];
    }
    army.pos = helper->location;
    if (army.troops.total() < 10 || !plan_path(army, besieged->location)) {
      helper->garrison.add(army.troops);
      continue;
    }
    armies.push_back(army);
    sent++;
    if (besieged == player) {
      std::stringstream ss;
      ss << helper->get_name() << " is sending " << army.troops.total() <<
            " soldiers to relieve your city, about " << days << " days away.";
      player_message(MESSAGE_MAJOR, ss.str());
    }
  }
}

// An AI city sends half its garrison against a target.
void Realm::ai_launch_army(AI_city* from, City* target, int kingdom_uid)
{
  Army army;
  army.uid = next_army_uid++;
  army.player_owned = false;
  army.home_uid = from->uid;
  army.target_uid = target->uid;
  army.kingdom_uid = kingdom_uid;
  army.race = from->get_race();
  army.order = ARMY_ORDER_ATTACK;
  for (int u = 1; u < UNIT_MAX; u++) {
    army.troops.count[u] = from->garrison.count[u] / 2;
    from->garrison.count[u] -= army.troops.count[u];
  }
  army.pos = from->location;
  if (!plan_path(army, target->location)) {
    from->garrison.add(army.troops);
    return;
  }
  armies.push_back(army);
  if (target == player) {
    std::stringstream ss;
    ss << "Scouts report " << army.troops.total() << " soldiers marching on " <<
          "your city from " << from->get_name() << "!";
    player_message(MESSAGE_URGENT, ss.str());
  } else {
    AI_city* ai_target = dynamic_cast<AI_city*>(target);
    if (ai_target && relation(ai_target).vassal) {
      std::stringstream ss;
      ss << "Scouts report an army from " << from->get_name() <<
            " marching on your vassal " << ai_target->get_name() << ".";
      player_message(MESSAGE_URGENT, ss.str());
    }
  }
  log_debug("%s marches on %s with %d", from->get_name().c_str(),
            target->get_name().c_str(), army.troops.total());
}

/* Once a month, kingdoms at war (and cities at war with the player) look for
 * a target: the nearest enemy city they think they can take.  This follows
 * the spirit of notes/old_notes/war.txt, cut down to strength and distance.
 */
void Realm::launch_ai_armies()
{
  std::map<int,int> marching;  // Armies out per kingdom
  for (int i = 0; i < armies.size(); i++) {
    if (!armies[i].player_owned && armies[i].order != ARMY_ORDER_RETURN) {
      marching[ armies[i].kingdom_uid ]++;
    }
  }
  for (int i = 0; i < cities.size(); i++) {
    AI_city* from = cities[i];
    int k = from->owner_kingdom;
    if (k < 0 || from->besieged || marching[k] >= 2) {
      continue;
    }
    if (from->garrison.fighters() < 60) {
      continue;
    }
    bool player_enemy = at_war_with_player(from);
    bool any_war = player_enemy;
    for (int n = 0; !any_war && n < GAME->kingdoms.size(); n++) {
      if (kingdoms_at_war(k, GAME->kingdoms[n]->uid)) {
        any_war = true;
      }
    }
    if (!any_war || !one_in(4)) {
      continue;
    }
    int send_str = troops_strength(from->garrison, from->get_race()) / 2;
// Find the nearest enemy city we could take.
    City* best = NULL;
    int best_dist = 9999;
    if (player_enemy) {
      int pdist = rl_dist(from->location, player->location);
      int pstr = troops_strength(player->get_troops(), player->get_race(), 130);
      if (pdist < 30 && send_str > pstr) {
        best = player;
        best_dist = pdist;
      }
    }
    for (int n = 0; n < cities.size(); n++) {
      AI_city* other = cities[n];
      if (other == from || other->besieged) {
        continue;
      }
      bool enemy = false;
      if (relation(other).vassal) {
        enemy = player_enemy;
      } else {
        enemy = kingdoms_at_war(k, other->owner_kingdom);
      }
      if (!enemy) {
        continue;
      }
      int dist = rl_dist(from->location, other->location);
      int their_str = troops_strength(other->garrison, other->get_race(),
                                      defense_bonus_for(other));
      if (dist < best_dist && dist < 30 && send_str > their_str) {
        best = other;
        best_dist = dist;
      }
    }
    if (best) {
      ai_launch_army(from, best, k);
      marching[k]++;
    }
  }
}

// *** Kingdoms ***

void Realm::change_kingdom_opinion(int a, int b, int amount)
{
  if (a == b) {
    return;
  }
  int v = kingdom_opinion(a, b) + amount;
  if (v > 100) {
    v = 100;
  } else if (v < -100) {
    v = -100;
  }
  k_opinion[a][b] = v;
}

void Realm::start_kingdom_war(int a, int b)
{
  k_war[a][b] = 1;
  Kingdom* ka = lookup_kingdom(a);
  Kingdom* kb = lookup_kingdom(b);
  if (!ka || !kb) {
    return;
  }
  std::stringstream ss;
  ss << "War!  The " << Race_data[ka->race]->plural_name << " have declared " <<
        "war on the " << Race_data[kb->race]->plural_name << ".";
  Kingdom* ours = player_kingdom();
  if (ours && !excommunicated && (ours == ka || ours == kb)) {
    ss << "  Your kingdom's war is yours: every city of the " <<
          Race_data[(ours == ka ? kb : ka)->race]->plural_name <<
          " is now your enemy.";
    Kingdom* enemy = (ours == ka ? kb : ka);
    for (int i = 0; i < cities.size(); i++) {
      if (cities[i]->owner_kingdom == enemy->uid) {
        City_relation& rel = relation(cities[i]);
        for (int t = 0; t < TREATY_MAX; t++) {
          rel.treaty_days[t] = 0;
        }
        rel.opinion -= 20;
        cancel_deals_with(cities[i], "");
      }
    }
    player_message(MESSAGE_URGENT, ss.str());
  } else {
    player_message(MESSAGE_MAJOR, ss.str());
  }
}

void Realm::end_kingdom_war(int a, int b)
{
  if (k_war.count(a)) {
    k_war[a].erase(b);
  }
  if (k_war.count(b)) {
    k_war[b].erase(a);
  }
  k_opinion[a][b] = -10;
  k_opinion[b][a] = -10;
  Kingdom* ka = lookup_kingdom(a);
  Kingdom* kb = lookup_kingdom(b);
  if (ka && kb) {
    std::stringstream ss;
    ss << "Peace: the " << Race_data[ka->race]->plural_name << " and the " <<
          Race_data[kb->race]->plural_name << " have ended their war.";
    player_message(MESSAGE_MAJOR, ss.str());
  }
// Their armies go home.
  for (int i = 0; i < armies.size(); i++) {
    if (!armies[i].player_owned &&
        (armies[i].kingdom_uid == a || armies[i].kingdom_uid == b) &&
        armies[i].order != ARMY_ORDER_RETURN &&
        armies[i].target_uid != player->uid) {
      AI_city* target = lookup_city(armies[i].target_uid);
      if (target && !relation(target).vassal) {
        target->besieged = false;
        recall_army(i);
      }
    }
  }
}

// Do two kingdoms' cities sit close to each other?  Borders breed quarrels.
static bool kingdoms_share_border(Realm* realm, int a, int b)
{
  for (int i = 0; i < realm->cities.size(); i++) {
    if (realm->cities[i]->owner_kingdom != a) {
      continue;
    }
    for (int j = 0; j < realm->cities.size(); j++) {
      if (realm->cities[j]->owner_kingdom == b &&
          rl_dist(realm->cities[i]->location, realm->cities[j]->location) <= 12) {
        return true;
      }
    }
  }
  return false;
}

void Realm::monthly_kingdoms()
{
  int nk = GAME->kingdoms.size();
  for (int i = 0; i < nk; i++) {
    for (int j = 0; j < nk; j++) {
      if (i == j) {
        continue;
      }
      Kingdom* ka = GAME->kingdoms[i];
      Kingdom* kb = GAME->kingdoms[j];
      int a = ka->uid, b = kb->uid;
// What the races feel for each other, soured by shared borders and by a
// warlike nature (Race_datum::warlikeness).
      int base = (Race_data[ka->race]->relations[kb->race] * 2 +
                  Race_data[kb->race]->relations[ka->race]) * 4;
      if (kingdoms_share_border(this, a, b)) {
        base -= 15;
      }
      int warlike = Race_data[ka->race]->warlikeness;
      if (warlike > 0) {
        base -= warlike * 10;
      }
      int now = kingdom_opinion(a, b);
      int drift = (base - now) / 6 + rng(-8, 8);
      if (kingdoms_at_war(a, b)) {
        drift -= 2;
      }
      change_kingdom_opinion(a, b, drift);
    }
  }
// Wars: count the days, end the long ones, start new ones.
  for (int i = 0; i < nk; i++) {
    for (int j = i + 1; j < nk; j++) {
      int a = GAME->kingdoms[i]->uid, b = GAME->kingdoms[j]->uid;
      if (kingdoms_at_war(a, b)) {
        int days = (k_war[a].count(b) ? k_war[a][b] : k_war[b][a]);
        if (days > 180 && rng(1, 100) <= 10 + (days - 180) / 15) {
          end_kingdom_war(a, b);
        }
        continue;
      }
      int opinion = (kingdom_opinion(a, b) + kingdom_opinion(b, a)) / 2;
      if (opinion > -25) {
        continue;
      }
// The more warlike of the two decides.
      Race ra = GAME->kingdoms[i]->race, rb = GAME->kingdoms[j]->race;
      int warlike = Race_data[ra]->warlikeness;
      int attacker = a, defender = b;
      if (Race_data[rb]->warlikeness > warlike) {
        warlike = Race_data[rb]->warlikeness;
        attacker = b;
        defender = a;
      }
      int chance = 2 + (warlike + 3) * 2 + (-25 - opinion) / 5;
// Kingdoms already at war rarely start another.
      for (int n = 0; n < nk; n++) {
        if (kingdoms_at_war(attacker, GAME->kingdoms[n]->uid) ||
            kingdoms_at_war(defender, GAME->kingdoms[n]->uid)) {
          chance /= 3;
        }
      }
      if (rng(1, 100) <= chance) {
        start_kingdom_war(attacker, defender);
      }
    }
  }
}

// *** You and the other cities ***

void Realm::monthly_player_relations()
{
  Kingdom* ours = player_kingdom();
  Race our_race = player->get_race();
  std::map<int,int> deals_with;
  for (int i = 0; i < deals.size(); i++) {
    if (!deals[i].suspended) {
      deals_with[ deals[i].city_uid ]++;
    }
  }
  for (int i = 0; i < cities.size(); i++) {
    AI_city* city = cities[i];
    City_relation& rel = relation(city);
    int target = Race_data[city->get_race()]->relations[our_race] * 8;
    if (ours && city->owner_kingdom == ours->uid && !excommunicated) {
      target += 15;
    }
    for (int t = 1; t < TREATY_MAX; t++) {
      if (rel.has_treaty(Treaty_type(t))) {
        target += 10;
      }
    }
    int trade = deals_with[city->uid] * 5;
    target += (trade > 20 ? 20 : trade);
    if (rel.vassal) {
      target -= 25;
    }
    if (at_war_with_player(city)) {
      target -= 50;
    }
// The people's opinion moves a little toward what they have reason to feel.
    rel.opinion += (target - rel.opinion) / 6 + rng(-2, 2);
    if (rel.opinion > 100) {
      rel.opinion = 100;
    } else if (rel.opinion < -100) {
      rel.opinion = -100;
    }
// A resentful vassal with few soldiers in it may throw you off.
    if (rel.vassal && rel.opinion < -50 && city->garrison.fighters() < 20 &&
        one_in(4)) {
      Kingdom* native = native_kingdom(city);
      rel.vassal = false;
      rel.at_war = false;
      city->owner_kingdom = (native ? native->uid : -1);
      city->recruit(20);
      std::stringstream ss;
      ss << "Your vassal " << city->get_name() << " has risen against you and " <<
            "returned to the " << (native ? Race_data[native->race]->plural_name :
                                   std::string("wilds")) << ".";
      player_message(MESSAGE_URGENT, ss.str());
    }
  }
}

// *** Your liege ***

City* Realm::get_liege()
{
  if (rank == RANK_MONARCH) {
    return NULL;
  }
  if (duke_uid >= 0) {
    City* duke = GAME->world->lookup_city_uid(duke_uid);
    AI_city* ai_duke = dynamic_cast<AI_city*>(duke);
    Kingdom* ours = player_kingdom();
    if (ai_duke && ours && ai_duke->owner_kingdom == ours->uid) {
      return duke;
    }
  }
  Kingdom* ours = player_kingdom();
  if (ours && ours->capital) {
    AI_city* capital = dynamic_cast<AI_city*>(ours->capital);
    if (capital && capital->owner_kingdom == ours->uid) {
      return capital;
    }
  }
  return NULL;
}

int Realm::get_liege_favor()
{
  City* liege = get_liege();
  if (!liege) {
    return 0;
  }
  if (liege->get_city_type() == CITY_TYPE_CAPITAL) {
    return favor_king;
  }
  return favor_duke;
}

void Realm::change_liege_favor(int amount)
{
  City* liege = get_liege();
  if (liege && liege->get_city_type() == CITY_TYPE_CAPITAL) {
    favor_king += amount;
  } else {
    favor_duke += amount;
// The crown hears of it, through the duke.
    favor_king += (amount > 0 ? (amount + 1) / 2 : amount / 2);
  }
  if (favor_duke > 100)  favor_duke = 100;
  if (favor_duke < -100) favor_duke = -100;
  if (favor_king > 100)  favor_king = 100;
  if (favor_king < -100) favor_king = -100;
}

int Realm::get_monthly_taxes()
{
  City* liege = get_liege();
  if (!liege || excommunicated) {
    return 0;
  }
  Fealty_datum fealty = get_fealty(liege->get_race());
  int ret = 20 + (fealty.taxes * player->get_total_population()) / 100;
// Higher favor, lighter taxes; low favor, heavier (notes/fealty.txt).
  int favor = get_liege_favor();
  if (favor >= 50) {
    ret = (ret * 3) / 4;
  } else if (favor < -20) {
    ret = (ret * 3) / 2;
  }
  return ret;
}

int Realm::aid_price(bool loan)
{
  City* liege = get_liege();
  if (!liege) {
    return 999;
  }
  Fealty_datum fealty = get_fealty(liege->get_race());
  int ret = (loan ? 4 : 12) - fealty.aid * 2;
  return (ret < 1 ? 1 : ret);
}

int Realm::request_aid(bool loan)
{
  City* liege = get_liege();
  if (!liege || excommunicated) {
    return 0;
  }
  Fealty_datum fealty = get_fealty(liege->get_race());
  int favor = get_liege_favor();
  if (favor < 0 || (fealty.aid == 0 && !loan)) {
    return 0;
  }
  if (loan && loan_owed > 0) {
    return 0;
  }
  int amount = (loan ? 1000 + player->get_total_population() * 5 :
                       200 + player->get_total_population());
  if (!loan) {
    amount = (amount * (fealty.aid + 1)) / 4;
  }
  change_liege_favor(0 - aid_price(loan));
  player->gain_resource(RES_GOLD, amount);
  if (loan) {
// Paid back a tenth a month, with a little interest.
    loan_owed = (amount * 11) / 10;
  }
  log_info("Aid from liege: %d gold (%s)", amount, loan ? "loan" : "gift");
  return amount;
}

std::string Realm::describe_demand()
{
  City* liege = get_liege();
  std::string lord = (liege ? liege->get_name() : std::string("Your liege"));
  std::stringstream ss;
  switch (demand_type) {
    case LIEGE_DEMAND_GOLD:
      ss << "Our treasury is in need.  As our loyal subject, you will send " <<
            demand_amount << " gold to " << lord << ".";
      break;
    case LIEGE_DEMAND_FOOD:
      ss << "Our granaries run low.  You will send us " << demand_amount <<
            " food.";
      break;
    case LIEGE_DEMAND_SOLDIERS:
      ss << "The kingdom is at war, and every city must do its part.  You " <<
            "will send " << demand_amount << " soldiers to the royal army.";
      break;
    case LIEGE_DEMAND_EMBARGO: {
      City* target = GAME->world->lookup_city_uid(demand_target);
      ss << "We are told you trade with " <<
            (target ? target->get_name() : std::string("our rivals")) <<
            ".  Their friends are no friends of ours.  You will stop.";
    } break;
    default:
      ss << "We have nothing to ask of you.";
      break;
  }
  return ss.str();
}

void Realm::answer_demand(int choice)
{
  if (demand_type == LIEGE_DEMAND_NULL) {
    return;
  }
  int amount = demand_amount;
  if (choice == 1) {
    amount = amount / 2;  // We do half, and ask for patience
  }
  bool done = (choice != 2);
  if (done) {
    switch (demand_type) {
      case LIEGE_DEMAND_GOLD:
        if (!player->expend_resource(RES_GOLD, amount)) {
          done = false;
        }
        break;
      case LIEGE_DEMAND_FOOD:
        if (!player->expend_resource(RES_FOOD, amount)) {
          done = false;
        }
        break;
      case LIEGE_DEMAND_SOLDIERS: {
        Troops troops = player->get_troops();
        if (troops.fighters() < amount) {
          done = false;
        } else {
          int left = amount;
          for (int i = UNIT_MILITIA; i < UNIT_COMMANDERS && left > 0; i++) {
            int n = (left < troops.count[i] ? left : troops.count[i]);
            troops.count[i] -= n;
            left -= n;
          }
          player->set_troops(troops);
          Kingdom* ours = player_kingdom();
          if (ours && ours->capital) {
            AI_city* cap = dynamic_cast<AI_city*>(ours->capital);
            if (cap) {
              cap->recruit(amount);
            }
          }
        }
      } break;
      case LIEGE_DEMAND_EMBARGO: {
        City* target = GAME->world->lookup_city_uid(demand_target);
        if (target) {
          cancel_deals_with(target, "your liege forbade it.");
        }
      } break;
      default:
        break;
    }
  }
  if (!done) {
    choice = 2;
  }
  switch (choice) {
    case 0: change_liege_favor(10); break;
    case 1: change_liege_favor(-3); break;
    default: change_liege_favor(-15); break;
  }
  demand_type = LIEGE_DEMAND_NULL;
  demand_days = 0;
}

void Realm::monthly_liege()
{
  City* liege = get_liege();
  Kingdom* ours = player_kingdom();
  if (!liege || !ours) {
    return;
  }
  Fealty_datum fealty = get_fealty(liege->get_race());

// Taxes.
  if (!excommunicated) {
    int taxes = get_monthly_taxes();
    if (player->expend_resource(RES_GOLD, taxes)) {
      AI_city* ai_liege = dynamic_cast<AI_city*>(liege);
      if (ai_liege) {
        ai_liege->resources[RES_GOLD] += taxes;
      }
      if (months_unpaid > 0) {
        months_unpaid = 0;
      }
      change_liege_favor(2);
      std::stringstream ss;
      ss << "You paid " << taxes << " gold in taxes to " << liege->get_name() <<
            ".";
      player_message(MESSAGE_MINOR, ss.str());
    } else {
      months_unpaid++;
      change_liege_favor(0 - (2 + fealty.strictness * 2) * months_unpaid);
      std::stringstream ss;
      ss << "You could not pay your taxes to " << liege->get_name() <<
            " this month (" << months_unpaid << " months owed).";
      player_message(MESSAGE_MAJOR, ss.str());
    }
  }

// The loan.
  if (loan_owed > 0) {
    int payment = loan_owed / 10 + 1;
    if (payment > loan_owed) {
      payment = loan_owed;
    }
    if (player->expend_resource(RES_GOLD, payment)) {
      loan_owed -= payment;
      if (loan_owed == 0) {
        player_message(MESSAGE_MINOR, "You have paid off your liege's loan.");
        change_liege_favor(5);
      }
    } else {
      change_liege_favor(-5);
      player_message(MESSAGE_MAJOR, "You missed a payment on your liege's \
loan.");
    }
  }

// A demand left unanswered counts as refused.
  if (demand_type != LIEGE_DEMAND_NULL) {
    demand_days++;
    if (demand_days >= 2) {
      player_message(MESSAGE_MAJOR, "You ignored your liege's demand.");
      answer_demand(2);
    }
  } else if (!excommunicated && rng(1, 100) <= 8 + fealty.demands * 4) {
// A new demand.
    std::vector<Liege_demand_type> options;
    options.push_back(LIEGE_DEMAND_GOLD);
    options.push_back(LIEGE_DEMAND_FOOD);
    bool at_war = false;
    for (int i = 0; i < GAME->kingdoms.size(); i++) {
      if (kingdoms_at_war(ours->uid, GAME->kingdoms[i]->uid)) {
        at_war = true;
      }
    }
    if (at_war && player->get_troops().fighters() >= 10) {
      options.push_back(LIEGE_DEMAND_SOLDIERS);
    }
// A trade partner your lords dislike.
    int embargo = -1;
    for (int i = 0; i < deals.size(); i++) {
      AI_city* partner = lookup_city(deals[i].city_uid);
      if (partner && partner->owner_kingdom != ours->uid &&
          kingdom_opinion(ours->uid, partner->owner_kingdom) < -25) {
        embargo = partner->uid;
      }
    }
    if (embargo >= 0) {
      options.push_back(LIEGE_DEMAND_EMBARGO);
    }
    demand_type = options[ rng(0, options.size() - 1) ];
    demand_seen = false;
    demand_target = embargo;
    demand_days = 0;
    switch (demand_type) {
      case LIEGE_DEMAND_GOLD:
        demand_amount = 100 + player->get_resource_amount(RES_GOLD) / 8;
        break;
      case LIEGE_DEMAND_FOOD:
        demand_amount = 200 + player->get_resource_amount(RES_FOOD) / 8;
        break;
      case LIEGE_DEMAND_SOLDIERS:
        demand_amount = player->get_troops().fighters() / 4 + 5;
        break;
      default:
        demand_amount = 0;
        break;
    }
    std::stringstream ss;
    ss << liege->get_name() << " has sent you a demand.  Answer it from the " <<
          "Realm screen (4 then 4) within the month.";
    player_message(MESSAGE_URGENT, ss.str());
  }

// Cast out of the kingdom?
  if (!excommunicated && favor_king <= -80) {
    excommunicated = true;
    std::stringstream ss;
    ss << "The " << rank_name(RANK_MONARCH) << " of the " <<
          Race_data[ours->race]->plural_name << " has cast you out of the " <<
          "kingdom.  Its cities are your enemies now.";
    player_message(MESSAGE_URGENT, ss.str());
    for (int i = 0; i < cities.size(); i++) {
      if (cities[i]->owner_kingdom == ours->uid) {
        cancel_deals_with(cities[i], "you have been cast out of the kingdom.");
      }
    }
  }

// Rising in the kingdom: the monarch names a well-favored baron a duke.
  if (rank == RANK_BARON && !excommunicated && favor_king >= 70 &&
      player->get_total_population() >= 400 &&
      GAME->get_date().get_year() > start_year) {
    rank = RANK_DUKE;
    duke_uid = -1;
    std::stringstream ss;
    ss << "The " << rank_name(RANK_MONARCH) << " has named you a " <<
          rank_name(RANK_DUKE) << "!  You answer to the crown alone now";
// A duke's lands: the nearest minor cities of the kingdom become yours.
    std::vector<AI_city*> fiefs;
    for (int n = 0; n < 3; n++) {
      AI_city* best = NULL;
      int best_dist = 9999;
      for (int i = 0; i < cities.size(); i++) {
        AI_city* c = cities[i];
        bool taken = false;
        for (int f = 0; f < fiefs.size(); f++) {
          if (fiefs[f] == c) {
            taken = true;
          }
        }
        if (taken || c->owner_kingdom != ours->uid ||
            c->get_city_type() != CITY_TYPE_CITY) {
          continue;
        }
        int dist = rl_dist(c->location, player->location);
        if (dist < best_dist) {
          best = c;
          best_dist = dist;
        }
      }
      if (best) {
        fiefs.push_back(best);
      }
    }
    for (int i = 0; i < fiefs.size(); i++) {
      make_vassal(fiefs[i]);
      relation(fiefs[i]).opinion += 30;  // Given, not taken
      ss << (i == 0 ? ", and " : (i + 1 == fiefs.size() ? " and " : ", ")) <<
            fiefs[i]->get_name();
    }
    if (!fiefs.empty()) {
      ss << (fiefs.size() == 1 ? " pays" : " pay") << " you tribute";
    }
    ss << ".";
    player_message(MESSAGE_UNLOCK, ss.str());
  }

// The old monarch.  Whales' help: when a monarch dies without an heir, the
// crown goes to whoever the court favors, or whoever seizes it.
  months_of_reign++;
  if (months_of_reign > 36 && rng(1, 100) <= 2 + months_of_reign / 24) {
    months_of_reign = 0;
    if (rank == RANK_DUKE && favor_king >= 60 && !excommunicated) {
      end_game(GAME_END_CROWNED, "The old " + rank_name(RANK_MONARCH) +
               " has died without an heir, and the court has chosen you, " +
               "the most trusted duke of the realm, to wear the crown.");
    } else {
      std::stringstream ss;
      ss << "The " << rank_name(RANK_MONARCH) << " of the " <<
            Race_data[ours->race]->plural_name << " has died.  A new " <<
            "monarch has been crowned";
      if (rank == RANK_DUKE) {
        ss << "; had the court trusted you more, it might have been you";
      }
      ss << ".";
      player_message(MESSAGE_MAJOR, ss.str());
      favor_king = favor_king / 2;
    }
  }
}

// Other cities act toward you: proposals and threats.
void Realm::monthly_events()
{
/* Whales: "bandits and aggressive neighbors must be defended against".  After
 * the first season, now and then a band of raiders comes for your stores.
 */
  int months = (GAME->get_date().get_year() - start_year) * 12 +
               GAME->get_date().get_month();
  if (months > 4 && rng(1, 100) <= 7) {
    bool out = false;
    for (int i = 0; i < armies.size(); i++) {
      if (armies[i].home_uid < 0 && !armies[i].player_owned) {
        out = true;
      }
    }
    if (!out) {
      Army army;
      army.uid = next_army_uid++;
      army.player_owned = false;
      army.home_uid = -1;
      army.target_uid = player->uid;
      army.kingdom_uid = -1;
      army.race = RACE_HUMAN;
// Raiders of whatever race lives in the wild nearby.
      if (!cities.empty()) {
        army.race = cities[ rng(0, cities.size() - 1) ]->get_race();
      }
      army.order = ARMY_ORDER_ATTACK;
      int size = 6 + player->get_total_population() / 12 + months / 3;
      army.troops.count[UNIT_MILITIA] = size / 2;
      army.troops.count[UNIT_SPEARMEN] = size - size / 2;
// They come out of the wilds a few days' march away.
      bool placed = false;
      for (int tries = 0; tries < 30 && !placed; tries++) {
        Point p(player->location.x + rng(-10, 10),
                player->location.y + rng(-10, 10));
        if (rl_dist(p, player->location) < 6 || GAME->world->OOB(p) ||
            GAME->world->get_city(p) ||
            GAME->world->travel_cost(p, army.race) <= 0) {
          continue;
        }
        army.pos = p;
        if (plan_path(army, player->location) && !army.path.empty()) {
          placed = true;
        }
      }
      if (placed) {
        armies.push_back(army);
        std::stringstream ss;
        ss << "A band of " << army.troops.total() << " raiders has been seen " <<
              "in the hills, heading for " << player->get_name() << "!";
        player_message(MESSAGE_URGENT, ss.str());
        return;
      }
    }
  }

  if (!one_in(3)) {
    return;
  }
// A warlike neighbor that dislikes you may demand tribute.
  for (int i = 0; i < cities.size(); i++) {
    AI_city* city = cities[i];
    City_relation& rel = relation(city);
    if (rel.vassal || at_war_with_player(city) || rel.truce_days > 0 ||
        rel.has_treaty(TREATY_PEACE) || rel.has_treaty(TREATY_ALLIANCE)) {
      continue;
    }
    if (rl_dist(city->location, player->location) > 20) {
      continue;
    }
    int warlike = Race_data[city->get_race()]->warlikeness;
    if (warlike < 1 || rel.opinion > -20 || !one_in(4)) {
      continue;
    }
    int theirs = garrison_strength(city) / 2;
    if (theirs <= player_strength()) {
      continue;
    }
    int amount = 100 + player->get_total_population() / 2;
    std::stringstream ss;
    ss << "Envoys from " << city->get_name() << " (" << city_title(city) <<
          ") say: \"Pay us " << amount << " gold, or we will come and take " <<
          "it.\"\n\nPay them?";
    if (query_yn(ss.str().c_str())) {
      if (player->expend_resource(RES_GOLD, amount)) {
        city->resources[RES_GOLD] += amount;
        rel.opinion += 10;
        rel.truce_days = 120;
        player_message(MESSAGE_MAJOR, "You paid " + city->get_name() +
                       " to leave you be.");
      } else {
        player_message(MESSAGE_URGENT, "You could not pay " +
                       city->get_name() + ", and they have declared war!");
        rel.at_war = true;
        cancel_deals_with(city, "");
      }
    } else {
      player_message(MESSAGE_URGENT, "You refused " + city->get_name() +
                     ", and they have declared war!");
      rel.at_war = true;
      cancel_deals_with(city, "");
    }
    return;
  }
}

void Realm::daily_cities()
{
  for (int i = 0; i < cities.size(); i++) {
    cities[i]->do_turn();
  }
  for (std::map<int,City_relation>::iterator it = relations.begin();
       it != relations.end(); it++) {
    City_relation& rel = it->second;
    for (int t = 1; t < TREATY_MAX; t++) {
      if (rel.treaty_days[t] > 0) {
        rel.treaty_days[t]--;
        if (rel.treaty_days[t] == 0) {
          AI_city* city = lookup_city(it->first);
          if (city) {
            std::stringstream ss;
            ss << "Your " << treaty_name(Treaty_type(t)) << " with " <<
                  city->get_name() << " has run out.";
            player_message(MESSAGE_MINOR, ss.str());
          }
        }
      }
    }
    if (rel.truce_days > 0) {
      rel.truce_days--;
    }
    if (rel.gift_days < 999) {
      rel.gift_days++;
    }
    if (rel.demand_days > 0) {
      rel.demand_days--;
    }
  }
  for (std::map<int,int>::iterator it = player_war.begin();
       it != player_war.end(); it++) {
    if (it->second > 0) {
      it->second++;
    }
  }
  for (std::map<int,std::map<int,int> >::iterator it = k_war.begin();
       it != k_war.end(); it++) {
    for (std::map<int,int>::iterator it2 = it->second.begin();
         it2 != it->second.end(); it2++) {
      if (it2->second > 0) {
        it2->second++;
      }
    }
  }
}

void Realm::do_turn()
{
// A defeat ends the world; after a victory, you may rule on.
  if (game_end == GAME_END_FALLEN || game_end == GAME_END_EMPTIED || !player) {
    return;
  }
  daily_cities();
  move_armies();
  if (game_end == GAME_END_FALLEN || game_end == GAME_END_EMPTIED) {
    return;
  }
  if (GAME->get_date().get_day() == 1) {
    monthly_kingdoms();
    monthly_player_relations();
    monthly_liege();
    launch_ai_armies();
    monthly_events();
  }
  if (game_end == GAME_END_NONE) {
    check_game_end();
  }
}

void Realm::end_game(Game_end type, std::string text)
{
  if (game_end != GAME_END_NONE) {
    return;
  }
  game_end = type;
  end_text = text;
  if (type == GAME_END_CROWNED) {
    rank = RANK_MONARCH;
  }
  log_info("Game over (%d): %s", int(type), text.c_str());
}

/* Whales: "Through military action or political maneuvering, you may
 * eventually become a Duke or even the King yourself."  The crown is the
 * victory: by succession (monthly_liege()), by taking your kingdom's capital,
 * or, for a race with no kingdom of its own, by ruling five cities.
 */
void Realm::check_game_end()
{
  if (player->get_total_population() <= 0) {
    end_game(GAME_END_EMPTIED, "The last of your people have gone.  " +
             player->get_name() + " stands empty.");
    return;
  }
  Kingdom* ours = player_kingdom();
  if (ours && ours->capital) {
    AI_city* capital = dynamic_cast<AI_city*>(ours->capital);
    if (capital && relation(capital).vassal) {
      end_game(GAME_END_CROWNED, "You hold " + capital->get_name() + ", the " +
               "capital of the " + Race_data[ours->race]->plural_name +
               ", and its throne is yours.  Long live the " +
               rank_name(RANK_MONARCH) + "!");
      return;
    }
  }
  int ruled = 0;
  for (int i = 0; i < cities.size(); i++) {
    if (relation(cities[i]).vassal) {
      ruled++;
    }
  }
  if (!ours && ruled >= 4) {
    end_game(GAME_END_CROWNED, "With " + itos(ruled) + " cities sworn to " +
             player->get_name() + ", you proclaim a kingdom of the " +
             Race_data[player->get_race()]->plural_name + ", and are " +
             "crowned its first " + rank_name(RANK_MONARCH) + ".");
  }
}
