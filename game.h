#ifndef _GAME_H_
#define _GAME_H_

#include "date.h"
#include "race.h"
#include <string>
#include <vector>

class  City;
class  Player_city;
struct World_design;
class  World_map;
class  Kingdom;
class  Realm;

class Game
{
public:
  Game();
  ~Game();

// Initialization
// returns false if we cancel when picking a city site
  bool start_new_game();
// returns true if we've generated or loaded our world
  bool is_world_ready();
// Also loads kingdoms.  Returns false if no saved world exists
  bool load_world();
// returns false on failure
  bool generate_world(World_design* design);

// Saves the game data and the current city in SAVE_DIR/cities/<cityname>.sav
  bool save_game();
// Loads a game from the specified file
  bool load_game(std::string filename);
// Asciiverse: once the player's city is loaded (Interface::load_game()), put
// it on the world map and load its Realm, the living world around it.
  void finish_loading();
// The file the Realm of the current city is kept in.
  std::string realm_filename();
  void set_date(Date new_date) { date = new_date; }

// Mutators
  void advance_time(int days = 1, City* city = NULL);

// Accessors
/* If we pass a length to get_date_str() (i.e. length != -1), then we colorize
 * it properly (white on blue) and make sure it fills the field (put in blue-on-
 * blue x until the tagless length matches the parameter).
 */
  Date get_date();
  std::string get_date_str(int length = -1);

  Kingdom* get_kingdom_for_race(Race race);

  int get_city_uid(); // Returns next_city_uid and advances next_city_uid.
  int peek_city_uid() { return next_city_uid; }
  void set_next_city_uid(int n) { next_city_uid = n; }

  World_map* world;
  std::vector<Kingdom*> kingdoms;
  Player_city* city;
  Realm* realm;

private:
  void generate_kingdoms(World_design* design);
  bool save_kingdoms();
  bool load_kingdoms();

  Date date;
  int next_city_uid;
  bool world_ready;
};

#endif
