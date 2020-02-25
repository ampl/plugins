#include "utils.hpp"

bool double_is_int(double val) {
   double absolute = abs(val);
   return absolute == floor(absolute);
}
