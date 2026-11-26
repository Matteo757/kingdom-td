#include "ktd/RangeSelection.h"

#include "ktd/TowerFactory.h"

namespace ktd {

void RangeSelection::select(std::size_t slot)
{
    m_preview.reset();   // selectie en voorbeeld sluiten elkaar uit
    m_selected = slot;
}

void RangeSelection::clearSelection()
{
    m_selected.reset();
}

std::optional<std::size_t> RangeSelection::selected() const
{
    return m_selected;
}

void RangeSelection::showPreview(std::size_t slot, TowerType type)
{
    m_selected.reset();
    m_preview = std::make_pair(slot, type);
}

void RangeSelection::hidePreview()
{
    m_preview.reset();
}

bool RangeSelection::hasPreview() const
{
    return m_preview.has_value();
}

std::optional<RangeZone> RangeSelection::visibleZone(const Game& game) const
{
    const std::vector<BuildSlot>& buildSlots = game.buildSlots();

    if (m_selected.has_value()) {
        if (*m_selected >= buildSlots.size() || buildSlots[*m_selected].tower == nullptr) {
            return std::nullopt;   // geen toren (meer) op deze bouwplek: niets te tonen
        }
        const Tower& tower = *buildSlots[*m_selected].tower;
        return RangeZone{RangeZone::Kind::Selected, *m_selected, tower.position(), tower.range()};
    }

    if (m_preview.has_value()) {
        const auto& [slot, type] = *m_preview;
        if (slot >= buildSlots.size() || buildSlots[slot].tower != nullptr) {
            return std::nullopt;   // een voorbeeld hoort bij een lege bouwplek
        }
        const Vec2& pad = buildSlots[slot].position;
        // Het bereik komt van een echte toren van dit type (level 1), dus het voorbeeld kan nooit afwijken van wat je
        // krijgt. Er wordt niets gebouwd: dit tijdelijke object verdwijnt meteen weer.
        const std::unique_ptr<Tower> sample = makeTower(type, pad, game.path());
        return RangeZone{RangeZone::Kind::Preview, slot, pad, sample->range()};
    }
    return std::nullopt;
}

}
