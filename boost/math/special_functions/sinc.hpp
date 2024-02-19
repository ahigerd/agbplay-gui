#pragma once

// this file exists to avoid needing to modify agbplay

#include <cmath>
#include <cassert>
#include <stdint.h>

namespace boost {
  namespace math {
    float sinc_pi(float x) {
      if (x == 0) {
        return x;
      }
      return std::sin(x) / x;
    }
  }
}
