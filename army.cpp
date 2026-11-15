#include "army.h"
#include "rng.h"
#include "stringfunc.h"
#include "globals.h"
#include "game.h"
#include "world.h"
#include "city.h"
#include <sstream>

Unit_type_datum Unit_data[UNIT_MAX];

#define _unit(n) cur = &(Unit_data[(n)])

void init_unit_data()
{
  Unit_type_datum* cur;

  _unit(UNIT_NULL);
  cur->name = "nobody";
  cur->citizen = CIT_NULL;
  cur->attack = 0; cur->defense = 0; cur->ranged = false;
  cur->gear = RES_NULL; cur->gear_amount = 0;
  cur->recruit_cost = 0; cur->wages = 0;

  _unit(UNIT_MILITIA);
  cur->name = "militia";
  cur->description = "Peasants called up with pitchforks, axes and courage.  \
Cheap and quick to raise, and no match for trained soldiers.";
  cur->citizen = CIT_PEASANT;
  cur->attack = 4; cur->defense = 0; cur->ranged = false;
  cur->gear = RES_NULL; cur->gear_amount = 0;
  cur->recruit_cost = 2; cur->wages = 5;

  _unit(UNIT_SPEARMEN);
  cur->name = "spearmen";
  cur->description = "Peasants drilled to hold a line of spears.  Each needs \
2 wood for his spear and shield.";
  cur->citizen = CIT_PEASANT;
  cur->attack = 7; cur->defense = 3; cur->ranged = false;
  cur->gear = RES_WOOD; cur->gear_amount = 2;
  cur->recruit_cost = 5; cur->wages = 8;

  _unit(UNIT_SWORDSMEN);
  cur->name = "swordsmen";
  cur->description = "Peasants with swords and mail, the heart of any real \
army.  Each needs 3 iron.";
  cur->citizen = CIT_PEASANT;
  cur->attack = 10; cur->defense = 6; cur->ranged = false;
  cur->gear = RES_IRON; cur->gear_amount = 3;
  cur->recruit_cost = 10; cur->wages = 12;

  _unit(UNIT_ARCHERS);
  cur->name = "archers";
  cur->description = "Merchants trained to the bow.  They loose a volley \
before the lines meet.  Each needs 2 wood.";
  cur->citizen = CIT_MERCHANT;
  cur->attack = 8; cur->defense = 1; cur->ranged = true;
  cur->gear = RES_WOOD; cur->gear_amount = 2;
  cur->recruit_cost = 8; cur->wages = 15;

  _unit(UNIT_COMMANDERS);
  cur->name = "commanders";
  cur->description = "Burghers who lead.  They do not fight in the line, but \
every commander makes a hundred soldiers fight a tenth better, up to half \
again as well.";
  cur->citizen = CIT_BURGHER;
  cur->attack = 0; cur->defense = 4; cur->ranged = false;
  cur->gear = RES_NULL; cur->gear_amount = 0;
  cur->recruit_cost = 50; cur->wages = 100;
}

Unit_type lookup_unit_type(std::string name)
{
  name = no_caps(name);
  for (int i = 0; i < UNIT_MAX; i++) {
    if (Unit_data[i].name == name) {
      return Unit_type(i);
    }
  }
  return UNIT_NULL;
}

Troops::Troops()
{
  clear();
}

void Troops::clear()
{
  for (int i = 0; i < UNIT_MAX; i++) {
    count[i] = 0;
  }
}

std::string Troops::save_data()
{
  std::stringstream ret;
  ret << UNIT_MAX << " ";
  for (int i = 0; i < UNIT_MAX; i++) {
    ret << count[i] << " ";
  }
  return ret.str();
}

bool Troops::load_data(std::istream& data)
{
  clear();
  int num;
  data >> num;
  for (int i = 0; i < num; i++) {
    int tmp;
    data >> tmp;
    if (i < UNIT_MAX) {
      count[i] = tmp;
    }
  }
  return !data.fail();
}

int Troops::total()
{
  int ret = 0;
  for (int i = 1; i < UNIT_MAX; i++) {
    ret += count[i];
  }
  return ret;
}

int Troops::fighters()
{
  return total() - count[UNIT_COMMANDERS];
}

void Troops::add(const Troops& other)
{
  for (int i = 0; i < UNIT_MAX; i++) {
    count[i] += other.count[i];
  }
}

// Percentage bonus from commanders: +10% per commander per 100 fighters, up to
// +50%.
static int commander_bonus(Troops& troops)
{
  int fighters = troops.fighters();
  if (fighters <= 0) {
    return 0;
  }
  int bonus = (10 * 100 * troops.count[UNIT_COMMANDERS]) / fighters;
  if (bonus > 50) {
    bonus = 50;
  }
  return bonus;
}

int troops_strength(Troops troops, Race race, int bonus)
{
  int raw = 0;
  for (int i = 1; i < UNIT_MAX; i++) {
    raw += troops.count[i] * Unit_data[i].attack;
  }
  int combat = (race > RACE_NULL && race < RACE_MAX ?
                Race_data[race]->base_combat : 10);
  long ret = long(raw) * combat / 10;
  ret = ret * (100 + commander_bonus(troops)) / 100;
  ret = ret * bonus / 100;
  return int(ret);
}

// Average defense of a set of troops, weighted by numbers.
static int average_defense(Troops& troops)
{
  int total = troops.total();
  if (total <= 0) {
    return 0;
  }
  int sum = 0;
  for (int i = 1; i < UNIT_MAX; i++) {
    sum += troops.count[i] * Unit_data[i].defense;
  }
  return sum / total;
}

static int race_hp(Race race)
{
  if (race > RACE_NULL && race < RACE_MAX && Race_data[race]->hp > 0) {
    return Race_data[race]->hp;
  }
  return 100;
}

/* Take <losses> soldiers out of troops, spread over the unit types by their
 * numbers.  Commanders are kept back from the line and fall at a quarter of
 * the rate.
 */
static int take_losses(Troops& troops, int losses)
{
  int total = troops.total();
  if (losses >= total) {
    troops.clear();
    return total;
  }
  int taken = 0;
  for (int i = 1; i < UNIT_MAX && taken < losses; i++) {
    if (troops.count[i] <= 0) {
      continue;
    }
    int share = (losses * troops.count[i]) / total;
    if (i == UNIT_COMMANDERS) {
      share /= 4;
    }
    if (share > troops.count[i]) {
      share = troops.count[i];
    }
    troops.count[i] -= share;
    taken += share;
  }
// Rounding leftovers come out of the biggest unit.
  while (taken < losses) {
    int biggest = 0;
    for (int i = 1; i < UNIT_MAX; i++) {
      if (i != UNIT_COMMANDERS &&
          (biggest == 0 || troops.count[i] > troops.count[biggest])) {
        biggest = i;
      }
    }
    if (biggest == 0 || troops.count[biggest] <= 0) {
      break;
    }
    troops.count[biggest]--;
    taken++;
  }
  return taken;
}

// Casualties caused by <damage> on troops of a race.
static int casualties_from(int damage, Troops& target, Race race)
{
  int per_soldier = race_hp(race) * (10 + average_defense(target)) / 10;
  if (per_soldier < 1) {
    per_soldier = 1;
  }
  int ret = damage / per_soldier;
// The remainder may still claim one more.
  if (rng(0, per_soldier - 1) < damage % per_soldier) {
    ret++;
  }
  return ret;
}

Battle_report::Battle_report()
{
  attacker_won = false;
  attackers_before = 0;
  defenders_before = 0;
  attackers_lost = 0;
  defenders_lost = 0;
  rounds = 0;
}

Battle_report fight_battle(Troops& attackers, Race att_race,
                           Troops& defenders, Race def_race,
                           int defense_bonus)
{
  Battle_report report;
  report.attackers_before = attackers.total();
  report.defenders_before = defenders.total();

  if (defenders.total() <= 0) {
    report.attacker_won = true;
    return report;
  }
  if (attackers.total() <= 0) {
    report.attacker_won = false;
    return report;
  }

  int att_combat = (att_race > RACE_NULL && att_race < RACE_MAX ?
                    Race_data[att_race]->base_combat : 10);
  int def_combat = (def_race > RACE_NULL && def_race < RACE_MAX ?
                    Race_data[def_race]->base_combat : 10);

// First, the archers loose a volley each.
  int att_volley = attackers.count[UNIT_ARCHERS] *
                   Unit_data[UNIT_ARCHERS].attack * att_combat / 10;
  int def_volley = defenders.count[UNIT_ARCHERS] *
                   Unit_data[UNIT_ARCHERS].attack * def_combat / 10;
  att_volley = att_volley * rng(150, 300) / 100;
  def_volley = def_volley * rng(150, 300) / 100 * defense_bonus / 100;
  int def_loss = casualties_from(att_volley, defenders, def_race);
  int att_loss = casualties_from(def_volley, attackers, att_race);
  report.defenders_lost += take_losses(defenders, def_loss);
  report.attackers_lost += take_losses(attackers, att_loss);

// Defenders behind walls hold on longer before they break.
  int att_break = (report.attackers_before + 1) / 2;
  int def_break = (defense_bonus > 100 ? (report.defenders_before * 2) / 3 :
                                         (report.defenders_before + 1) / 2);

  bool att_broke = false, def_broke = false;
  for (report.rounds = 1; report.rounds <= 12; report.rounds++) {
    int att_str = troops_strength(attackers, att_race, 100);
    int def_str = troops_strength(defenders, def_race, defense_bonus);
    int att_dmg = att_str * rng(15, 30) / 10;
    int def_dmg = def_str * rng(15, 30) / 10;
    report.defenders_lost += take_losses(defenders,
                                         casualties_from(att_dmg, defenders,
                                                         def_race));
    report.attackers_lost += take_losses(attackers,
                                         casualties_from(def_dmg, attackers,
                                                         att_race));
    att_broke = (report.attackers_lost >= att_break || attackers.total() <= 0);
    def_broke = (report.defenders_lost >= def_break || defenders.total() <= 0);
    if (att_broke || def_broke) {
      break;
    }
  }

// If neither side broke, the attack has failed: the defenders hold.
  if (def_broke && !att_broke) {
    report.attacker_won = true;
  } else if (def_broke && att_broke) {
// Both broke at once; whoever has more left holds the field.
    report.attacker_won = (attackers.total() > defenders.total());
  } else {
    report.attacker_won = false;
  }

// The side that flees leaves some behind, scattered or taken prisoner.
  if (report.attacker_won) {
    report.defenders_lost += take_losses(defenders, defenders.total() / 10);
  } else {
    report.attackers_lost += take_losses(attackers, attackers.total() / 10);
  }

  std::stringstream ss;
  ss << (report.attacker_won ? "The attackers won" : "The defenders held") <<
        " (" << report.attackers_lost << " of " << report.attackers_before <<
        " attackers and " << report.defenders_lost << " of " <<
        report.defenders_before << " defenders lost).";
  report.summary = ss.str();
  return report;
}

std::string army_order_name(Army_order order)
{
  switch (order) {
    case ARMY_ORDER_NULL:    return "none";
    case ARMY_ORDER_ATTACK:  return "marching to attack";
    case ARMY_ORDER_SIEGE:   return "besieging";
    case ARMY_ORDER_RETURN:  return "marching home";
    case ARMY_ORDER_RELIEVE: return "marching to defend";
    case ARMY_ORDER_MAX:     return "BUG - ARMY_ORDER_MAX";
  }
  return "BUG - Unnamed Army_order";
}

Army::Army()
{
  uid = -1;
  player_owned = false;
  home_uid = -1;
  target_uid = -1;
  kingdom_uid = -1;
  race = RACE_NULL;
  order = ARMY_ORDER_NULL;
  travel_points = 0;
  days_out = 0;
  siege_days = 0;
  supplies = 0;
}

std::string Army::save_data()
{
  std::stringstream ret;
  ret << uid << " " << (player_owned ? 1 : 0) << " " << home_uid << " " <<
         target_uid << " " << kingdom_uid << " " << int(race) << " " <<
         int(order) << " ";
  ret << troops.save_data() << " ";
  ret << pos.x << " " << pos.y << " " << path.size() << " ";
  for (int i = 0; i < path.size(); i++) {
    ret << path[i].x << " " << path[i].y << " ";
  }
  ret << travel_points << " " << days_out << " " << siege_days << " " <<
         supplies;
  return ret.str();
}

bool Army::load_data(std::istream& data)
{
  int tmpowned, tmprace, tmporder;
  data >> uid >> tmpowned >> home_uid >> target_uid >> kingdom_uid >> tmprace >>
          tmporder;
  player_owned = (tmpowned != 0);
  if (tmprace < 0 || tmprace >= RACE_MAX || tmporder < 0 ||
      tmporder >= ARMY_ORDER_MAX) {
    return false;
  }
  race = Race(tmprace);
  order = Army_order(tmporder);
  if (!troops.load_data(data)) {
    return false;
  }
  int num_steps;
  data >> pos.x >> pos.y >> num_steps;
  path.clear();
  for (int i = 0; i < num_steps; i++) {
    Point p;
    data >> p.x >> p.y;
    path.push_back(p);
  }
  data >> travel_points >> days_out >> siege_days >> supplies;
  return !data.fail();
}

std::string Army::get_name()
{
  City* home = GAME->world->lookup_city_uid(home_uid);
  if (player_owned) {
    return "your army";
  }
  if (home) {
    return "the army of " + home->get_name();
  }
  return "the raiders";
}

glyph Army::get_glyph()
{
  glyph ret('&', c_ltred, c_black);
  if (player_owned) {
    ret.fg = c_white;
  } else if (order == ARMY_ORDER_RETURN) {
    ret.fg = c_ltgray;
  }
  return ret;
}

bool Army::at_destination()
{
  return path.empty();
}
