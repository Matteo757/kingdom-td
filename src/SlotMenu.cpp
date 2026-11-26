#include "ktd/SlotMenu.h"

#include "ktd/Prices.h"
#include "ktd/TowerFactory.h"

namespace ktd {

std::vector<MenuEntry> slotMenuEntries(const Game& game, std::size_t slot)
{
    std::vector<MenuEntry> entries;
    if (slot >= game.buildSlots().size()) {
        return entries;
    }

    const Tower* tower = game.buildSlots()[slot].tower;
    if (tower == nullptr) {
        for (const TowerType type : kBuildableTowers) {
            const int price = buildCost(type);
            entries.push_back({MenuAction::Build, type, price, game.canBuild(slot, type),
                               towerName(type) + " (" + std::to_string(price) + " goud)"});
        }
        return entries;
    }

    if (tower->level() >= Tower::kMaxLevel) {
        entries.push_back({MenuAction::MaxLevel, tower->type(), 0, false, "Maximaal level"});
    } else {
        const int price = upgradeCost(tower->level());
        entries.push_back({MenuAction::Upgrade, tower->type(), price, game.canUpgrade(slot),
                           "Upgrade naar level " + std::to_string(int(tower->level()) + 1)
                               + " (" + std::to_string(price) + " goud)"});
    }

    // De verkoopknop staat alleen in het menu van een bouwplek met een toren. Het bedrag komt uit dezelfde functie
    // (Game::sellValue) die ook uitbetaalt, dus wat hier staat is altijd wat de speler krijgt.
    const int refund = game.sellValue(slot);
    entries.push_back({MenuAction::Sell, tower->type(), refund, game.canSell(slot),
                       "Verkopen (+" + std::to_string(refund) + " goud)"});
    return entries;
}

}
