#ifndef KTD_WAVEFACTORY_H
#define KTD_WAVEFACTORY_H

#include <vector>
#include "ktd/WaveManager.h"

namespace ktd {

constexpr double kWaveDelay = 20.0;

Wave buildWave(int number);
std::vector<Wave> buildWaves(int count);

}

#endif  // KTD_WAVEFACTORY_H
