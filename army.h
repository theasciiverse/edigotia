#ifndef _ARMY_H_
#define _ARMY_H_

#include "geometry.h"
#include "citizen.h"
#include "resource.h"
#include "race.h"
#include "glyph.h"
#include <string>
#include <vector>
#include <istream>

/* Edigotia's armies, after Whales' mockup of the Military Unit screen
 * (screenshots/unit.png) and the help text he wrote for peasants, merchants
 * and burghers: peasants fill the ranks as infantry, merchants take the
 * weapons that need training (bows), and burghers command, multiplying the
 * strength of everyone under them.
 */

enum Unit_type
{
  UNIT_NULL = 0,

  UNIT_MILITIA,     // Peasants with whatever they could find
  UNIT_SPEARMEN,    // Peasants with spears (wood)
  UNIT_SWORDSMEN,   // Peasants with swords and mail (iron)
  UNIT_ARCHERS,     // Merchants with bows (wood); they shoot first
  UNIT_COMMANDERS,  // Burghers; each makes the rest fight better

  UNIT_MAX
};

struct Unit_type_datum
{
  std::string name;         // Plural, as it appears in a roster
  std::string description;
  Citizen_type citizen;     // Who is drafted to fill it
  int attack;               // Strength in battle, before race and bonuses
  int defense;              // Reduces the damage it takes
  bool ranged;              // Gets a volley in before the lines meet
  Resource gear;            // Spent once per soldier recruited
  int gear_amount;
  int recruit_cost;         // Gold per soldier recruited
  int wages;                // Gold per 10 soldiers per day
};

extern Unit_type_datum Unit_data[UNIT_MAX];
void init_unit_data();
Unit_type lookup_unit_type(std::string name);

// A roster of soldiers by type; used for armies in the field and garrisons.
struct Troops
{
  Troops();

  std::string save_data();
  bool load_data(std::istream& data);

  int total();          // All soldiers, commanders included
  int fighters();       // All soldiers but commanders
  bool empty() { return total() <= 0; }
  void clear();
  void add(const Troops& other);

  int count[UNIT_MAX];
};

/* Battle strength of a set of troops of a race.  bonus is a percentage (100 is
 * no change), for walls, home ground and the like.
 */
int troops_strength(Troops troops, Race race, int bonus = 100);

struct Battle_report
{
  Battle_report();

  bool attacker_won;
  int attackers_before, defenders_before;
  int attackers_lost, defenders_lost;
  int rounds;
  std::string summary;  // One line for messages
};

/* Fight a battle.  Losses are taken out of both Troops.  Battles are not to
 * the last soldier: the side that loses more than half its strength (or two
 * thirds, for defenders behind walls) breaks and flees.
 * defense_bonus is a percentage for the defender (walls, keep, home ground).
 */
Battle_report fight_battle(Troops& attackers, Race att_race,
                           Troops& defenders, Race def_race,
                           int defense_bonus = 100);

enum Army_order
{
  ARMY_ORDER_NULL = 0,
  ARMY_ORDER_ATTACK,    // March on target_uid and take it
  ARMY_ORDER_SIEGE,     // Camped outside target_uid, starving it out
  ARMY_ORDER_RETURN,    // March home to home_uid and rejoin its garrison
  ARMY_ORDER_RELIEVE,   // March to target_uid and help defend it
  ARMY_ORDER_MAX
};

std::string army_order_name(Army_order order);

struct Army
{
  Army();

  std::string save_data();
  bool load_data(std::istream& data);

  std::string get_name();   // "the army of Lanussin"
  glyph get_glyph();
  bool at_destination();

  int uid;
  bool player_owned;
  int home_uid;     // City it marched from (the player's city for the player)
  int target_uid;   // City it is marching on, or besieging
  int kingdom_uid;  // Kingdom it fights for; -1 for the player and rebels
  Race race;
  Army_order order;
  Troops troops;

  Point pos;                // Where it is on the world map
  std::vector<Point> path;  // The road ahead, next step first
  int travel_points;        // Saved-up movement; 100 points is a day's march
  int days_out;             // Days since it set out
  int siege_days;           // Days spent besieging target_uid
  int supplies;             // Days of food carried
};

#endif
