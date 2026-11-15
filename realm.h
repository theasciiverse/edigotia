#ifndef _REALM_H_
#define _REALM_H_

/* The Realm is everything outside the player's walls that changes as the game
 * goes on: the other cities growing and keeping their garrisons, their
 * people's opinion of you, the trade deals and treaties you have made, your
 * place in your kingdom (your liege, your favor with them, your rank), the
 * kingdoms' quarrels and wars, and the armies on the march.
 *
 * The world file (world.sav, kingdoms.txt) stays as it was generated; each
 * player city keeps its own Realm beside its save, as cities/<name>.realm, so
 * every game started in a world has a living world of its own.
 *
 * Whales planned all of this and wrote much of it down: in the help articles
 * on kingdoms, dukes and monarchs (help_data.cpp), in notes/fealty.txt (what a
 * lord asks of each race and gives back), in notes/old_notes/war.txt (how a
 * city weighs going to war), and on his development blog.  VISION.md lists
 * the sources, and what was chosen here where he left no design.
 */

#include "army.h"
#include "color.h"
#include "race.h"
#include "resource.h"
#include "date.h"
#include <vector>
#include <map>
#include <string>
#include <istream>

class City;
class AI_city;
class Kingdom;
class Player_city;

// Treaties the player can sign with another city.  Each lasts a year.
enum Treaty_type
{
  TREATY_NULL = 0,
  TREATY_TRADE,     // A trade pact: half the overhead on deals with them
  TREATY_PEACE,     // A peace pact: no demands, no war, for a year
  TREATY_ALLIANCE,  // An alliance: they march to help when you are besieged
  TREATY_MAX
};

std::string treaty_name(Treaty_type type);

#define TREATY_LENGTH 360

// How one city and its people see the player's city.
struct City_relation
{
  City_relation();

  std::string save_data();
  bool load_data(std::istream& data);

  bool has_treaty(Treaty_type type);

  int opinion;            // -100 to 100; their citizens' opinion of you
  int treaty_days[TREATY_MAX];  // Days left on each treaty; 0 is none
  bool at_war;            // A war between you and this city alone
  int truce_days;         // After peace, days before war may be declared
  int gift_days;          // Days since your last gift
  int demand_days;        // Days until they will hear another demand
  bool vassal;            // You conquered it; it pays you tribute
};

// A standing order to buy or sell a resource every day.
struct Trade_deal
{
  Trade_deal();

  std::string save_data();
  bool load_data(std::istream& data);

  int city_uid;
  Resource resource;
  int amount;       // Units per day
  int price;        // Gold per 100 units, overhead included
  bool importing;   // True if they sell to us, false if we sell to them
  bool suspended;   // Paused (unpaid, undelivered, or war)
};

enum Noble_rank
{
  RANK_BARON = 0, // A city of your own, under a duke
  RANK_DUKE,      // Lord of a duchy, under the monarch
  RANK_MONARCH,   // King or queen
  RANK_MAX
};

std::string rank_name(Noble_rank rank, bool female = false);

// Something your liege asks of you; see notes/fealty.txt
enum Liege_demand_type
{
  LIEGE_DEMAND_NULL = 0,
  LIEGE_DEMAND_GOLD,      // A payment
  LIEGE_DEMAND_FOOD,      // A shipment of food
  LIEGE_DEMAND_SOLDIERS,  // Troops for the royal army, in wartime
  LIEGE_DEMAND_EMBARGO,   // Stop trading with a city they dislike
  LIEGE_DEMAND_MAX
};

enum Game_end
{
  GAME_END_NONE = 0,
  GAME_END_CROWNED,     // Victory: you wear the crown
  GAME_END_FALLEN,      // Defeat: your keep was taken
  GAME_END_EMPTIED,     // Defeat: your people are all gone
  GAME_END_MAX
};

/* What each race's lords expect and give, summarized from Whales'
 * notes/fealty.txt.  The levels run from 0 (none) to 4 (very high).
 */
struct Fealty_datum
{
  int taxes;          // Monthly taxes owed, per 100 citizens (gold)
  int strictness;     // How much favor late taxes cost
  int infighting;     // 0 ignored, 1 frowned on, 2 stopped, 3 punished
  int aid;            // How freely the lord helps in need
  int military_aid;   // How readily the lord sends troops to defend you
  int demands;        // How often the lord asks for things
};

Fealty_datum get_fealty(Race race);

class Realm
{
public:
  Realm();
  ~Realm();

  void reset();
// A new city has been founded; pick its liege, set opinions.
  void start_new_game(Player_city* city);
// Called once the world and kingdoms are loaded, with or without a save.
  void link_cities();

  bool save_to_file(std::string filename);
  bool load_from_file(std::string filename);

// One day passes outside the player's city.
  void do_turn();

// Trade, called from Player_city::do_turn()
  void do_trade(Player_city* city);
  int  get_import(Resource res);  // Expected daily, for reports
  int  get_export(Resource res);
  int  get_trade_income();        // Gold per day from sales and tribute
  int  get_trade_expense();       // Gold per day on purchases
  int  get_tribute();             // Gold per day from vassals

// *** Looking things up ***
  std::vector<AI_city*> cities;   // Every city but the player's
  AI_city* lookup_city(int uid);
  Kingdom* lookup_kingdom(int uid);
  Kingdom* native_kingdom(City* city);  // The kingdom it was founded in
  Kingdom* owner_kingdom(City* city);   // The kingdom that holds it now
  Kingdom* player_kingdom();            // Your race's kingdom, or NULL
  std::string city_title(City* city);   // "the duchy seat of the dwarves"

  City_relation& relation(City* city);
  City_relation& relation(int uid);
  std::string relationship_name(int opinion);
  nc_color relationship_color(int opinion);

  bool at_war_with_player(City* city);
  bool kingdom_at_war_with_player(int kingdom_uid);
  bool kingdoms_at_war(int a, int b);
  int  kingdom_opinion(int a, int b);
  int  trade_days(City* city);     // Days for a caravan to reach it, or -1
  int  route_overhead(City* city); // Overhead per 100 units, pacts included

// *** Player actions: trade ***
  std::vector<Trade_deal> deals;
  void add_deal(Trade_deal deal);
  void cancel_deal(int index);
  void cancel_deals_with(City* city, std::string reason);

// *** Player actions: diplomacy ***
// Gold the city asks to sign a treaty; -1 if they will not sign at any price.
  int  treaty_price(City* city, Treaty_type type);
  bool sign_treaty(City* city, Treaty_type type);
  int  give_gift(City* city, int gold);   // Returns the opinion gained
// Demand tribute; returns the gold they hand over, or 0 if they refuse.
  int  demand_tribute(City* city);
  void declare_war(City* city);
// Gold to make peace; negative if they will pay you to stop.  If refuse is
// set, they will not hear of peace at all.
  int  peace_price(City* city, bool& refuse);
  bool make_peace(City* city);

// *** Player actions: war ***
  std::vector<Army> armies;
  int  player_strength();           // Stationed troops and armies in the field
  int  garrison_strength(City* city);
  bool send_army(Troops troops, City* target);
  void recall_army(int index);
  bool player_besieged();
  int  siege_days_left(City* city); // For a city under siege

// *** Your place in the kingdom ***
  Noble_rank rank;
  int duke_uid;       // Your duke's city; -1 if you answer to the monarch
  int favor_duke;     // -100 to 100
  int favor_king;
  int months_unpaid;  // Months of taxes not paid
  int loan_owed;      // Gold owed to your liege
  int months_of_reign;// Months since the monarch's reign was last counted
  bool excommunicated;// Cast out of the kingdom

  Liege_demand_type demand_type;  // A demand waiting for your answer
  int demand_amount;
  int demand_target;
  int demand_days;

  City* get_liege();
  int   get_liege_favor();
  void  change_liege_favor(int amount);
  int   get_monthly_taxes();
  int   aid_price(bool loan);       // Favor it costs to ask for aid
  int   request_aid(bool loan);     // Gold received
  void  answer_demand(int choice);  // 0 comply, 1 bargain, 2 refuse
  std::string describe_demand();

  int tribute_today;      // Gold from vassals today (for reports)
  bool demand_seen;       // The player has been shown the current demand
  bool end_seen;          // The player has been shown the end of the game

// *** The end of the game ***
  Game_end game_end;
  std::string end_text;
  int start_year;

// *** Kingdom politics ***
  std::map<int, std::map<int,int> > k_opinion;  // [a][b]
  std::map<int, std::map<int,int> > k_war;      // Days at war; 0 at peace
  std::map<int, int> player_war;                // Kingdom uid -> days at war

  std::map<int, City_relation> relations;

  int next_army_uid;

private:
  void daily_cities();
  void monthly_kingdoms();
  void monthly_player_relations();
  void monthly_liege();
  void monthly_events();
  void move_armies();
  void army_arrives(Army& army);
  void siege_turn(Army& army);
  void conquer(Army& army, City* target);
  void launch_ai_armies();
  void ai_launch_army(AI_city* from, City* target, int kingdom_uid);
  void send_relief(City* besieged);
  void check_game_end();
  void end_game(Game_end type, std::string text);
  void start_kingdom_war(int a, int b);
  void end_kingdom_war(int a, int b);
  void change_kingdom_opinion(int a, int b, int amount);
  bool plan_path(Army& army, Point dest);
  void make_vassal(City* city);
  void player_message(int type, std::string text);

  Player_city* player;
};

#endif
