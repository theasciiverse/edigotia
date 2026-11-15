#ifndef _MILITARY_H_
#define _MILITARY_H_

#include "army.h"
#include <string>
#include <istream>

// Soldiers of one type stationed in the player's city.
struct Military_unit
{
  Military_unit();
  ~Military_unit();

  std::string save_data();
  bool load_data(std::istream& data);

  Unit_type type; // Asciiverse: Whales' unit only had a count
  int count;
};

#endif
