#ifndef KTD_SLOTMENU_H
#define KTD_SLOTMENU_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "ktd/Game.h"

namespace ktd {

// De torens die op een lege bouwplek gebouwd kunnen worden, in de volgorde van het menu.
constexpr std::array<TowerType, 5> kBuildableTowers = {
    TowerType::Archer, TowerType::Cannon, TowerType::Mage, TowerType::Barracks, TowerType::Artillery
};

enum class MenuAction : std::uint8_t { Build, Upgrade, MaxLevel };

// Eén regel van het bouw- of upgrademenu van een bouwplek.
struct MenuEntry {
    MenuAction action;
    TowerType type;      // bij Build: de toren die gebouwd wordt
    int price;           // 0 bij MaxLevel
    bool enabled;        // true zolang de speler genoeg goud heeft
    std::string text;
};

// Zuivere logica (geen Qt): bepaalt de regels van het menu van één bouwplek aan de hand van de toestand van Game.
// Het goud wordt bij elke aanroep opnieuw uit Game gelezen (Game blijft de enige bron). De GUI roept deze functie
// opnieuw aan zodra het goud verandert, zodat een open menu meteen bijgewerkt wordt.
// Een ongeldige bouwplek geeft een lege lijst.
std::vector<MenuEntry> slotMenuEntries(const Game& game, std::size_t slot);

}

#endif  // KTD_SLOTMENU_H
