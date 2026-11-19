#pragma once

#include <memory>
#include <string>
#include "ktd/Path.h"
#include "ktd/Tower.h"
#include "ktd/Vec2.h"

namespace ktd {

std::unique_ptr<Tower> makeTower(TowerType type, const Vec2& position, const Path& path);
std::string towerName(TowerType type);

}