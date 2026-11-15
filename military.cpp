#include "military.h"
#include <sstream>

Military_unit::Military_unit()
{
  type = UNIT_MILITIA;
  count = 0;
}

Military_unit::~Military_unit()
{
}

std::string Military_unit::save_data()
{
  std::stringstream ret;
  ret << int(type) << " " << count << " ";

  return ret.str();
}

bool Military_unit::load_data(std::istream& data)
{
  int tmptype;
  data >> tmptype >> count;
  if (tmptype <= UNIT_NULL || tmptype >= UNIT_MAX) {
    tmptype = UNIT_MILITIA;
  }
  type = Unit_type(tmptype);

  return true;
}
