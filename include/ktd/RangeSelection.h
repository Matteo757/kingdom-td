#ifndef KTD_RANGESELECTION_H
#define KTD_RANGESELECTION_H

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include "ktd/Game.h"
#include "ktd/TowerType.h"
#include "ktd/Vec2.h"

namespace ktd {

// De ene bereikzone die nu getekend moet worden.
struct RangeZone {
    enum class Kind : std::uint8_t {
        Selected,   // het bereik van een toren die staat en aangeklikt is
        Preview     // het bereik van een toren die gebouwd gaat worden (nog niet gebouwd)
    };

    Kind kind;
    std::size_t slot;
    Vec2 center;
    double radius;
};

// Zuivere logica (geen Qt) die bepaalt welke bereikzone zichtbaar is. Standaard is er geen enkele zone zichtbaar.
//  - select(): een toren is aangeklikt, alleen zijn bereik wordt getoond (een eerdere selectie verdwijnt);
//  - showPreview(): tijdens het kiezen van een toren om te bouwen wordt het bereik van die toren op de lege
//    bouwplek getoond; hidePreview() haalt het weg (geannuleerd of klaar);
//  - selectie en voorbeeld sluiten elkaar uit, dus er is nooit meer dan één zone.
// De straal wordt pas gelezen op het moment van tekenen (visibleZone), rechtstreeks bij de toren zelf. Zo klopt hij
// altijd met het huidige level en verandert er niets aan de berekening van bereik, richten of schade.
class RangeSelection {
public:
    void select(std::size_t slot);
    void clearSelection();
    std::optional<std::size_t> selected() const;

    void showPreview(std::size_t slot, TowerType type);
    void hidePreview();
    bool hasPreview() const;

    // De zone die nu getekend moet worden, of niets.
    std::optional<RangeZone> visibleZone(const Game& game) const;

private:
    std::optional<std::size_t> m_selected;
    std::optional<std::pair<std::size_t, TowerType>> m_preview;
};

}

#endif  // KTD_RANGESELECTION_H
