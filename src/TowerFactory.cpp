#include "ktd/TowerFactory.h"

#include <stdexcept>
#include "ktd/ArcherTower.h"
#include "ktd/ArtilleryTower.h"
#include "ktd/BarracksTower.h"
#include "ktd/CannonTower.h"
#include "ktd/MageTower.h"

namespace ktd {

std::unique_ptr<Tower> makeTower(TowerType type, const Vec2& position, const Path& path)
{
    switch (type) {
    case TowerType::Archer:
        return std::make_unique<ArcherTower>(position);
    case TowerType::Cannon:
        return std::make_unique<CannonTower>(position);
    case TowerType::Mage:
        return std::make_unique<MageTower>(position);
    case TowerType::Barracks:
        return std::make_unique<BarracksTower>(position, path);
    case TowerType::Artillery:
        return std::make_unique<ArtilleryTower>(position);
    }
    throw std::invalid_argument("Unknown tower type");
}

std::string towerName(TowerType type)
{
    switch (type) {
    case TowerType::Archer:    return "Archer Tower";
    case TowerType::Cannon:    return "Cannon Tower";
    case TowerType::Mage:      return "Mage Tower";
    case TowerType::Barracks:  return "Barracks";
    case TowerType::Artillery: return "Artillery Tower";
    }
    return "Unknown";
}

}