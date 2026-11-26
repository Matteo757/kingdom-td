#include <QtTest>
#include <cstdint>
#include <random>
#include <vector>
#include "ktd/Game.h"
#include "ktd/RangeSelection.h"
#include "ktd/SlotMenu.h"
#include "ktd/TowerFactory.h"

using namespace ktd;

// Vereiste 53: 53. test-driven development (= written test plan or unit tests)
// Testplan voor het tonen van bereikzones:
//  (1) standaard is er geen enkele zone zichtbaar, ook niet als er torens staan;
//  (2) een toren aanklikken toont alleen zijn bereik, voor elke torensoort en met zijn huidige level;
//  (3) een andere toren kiezen vervangt de zone, deselecteren of een verkochte toren verbergt hem;
//  (4) tijdens het bouwen toont een voorbeeld het bereik van de toren die gebouwd gaat worden;
//  (5) annuleren verbergt het voorbeeld, na het bouwen blijft de nieuwe toren geselecteerd;
//  (6) er is nooit meer dan één zone, en bereik, richten en schade veranderen niet.
// Deze tests zijn geschreven vóór RangeSelection bestond.

namespace {

constexpr std::array<TowerType, 5> kAllTypes = {
    TowerType::Archer, TowerType::Cannon, TowerType::Mage, TowerType::Barracks, TowerType::Artillery
};

// Een rechte weg en zes bouwplekken ver genoeg uit elkaar.
Game makeGame()
{
    std::vector<Vec2> route = {{0.0, 100.0}, {1500.0, 100.0}};
    std::vector<Vec2> pads = {{100.0, 40.0}, {300.0, 40.0}, {500.0, 40.0}, {700.0, 40.0}, {900.0, 40.0}, {1100.0, 40.0}};
    return Game(Path(route), Player(100000, 20), WaveManager({}, 3.0, 6.0), pads);
}

// Bouwt alle vijf de torensoorten op de eerste vijf bouwplekken (de zesde blijft leeg).
void buildAllTypes(Game& game)
{
    for (std::size_t i = 0; i < kAllTypes.size(); ++i) {
        QVERIFY(game.buildTower(i, kAllTypes[i]));
    }
}

bool near(double a, double b)
{
    return std::abs(a - b) < 1e-9;
}

}

class TestRangeSelection : public QObject {
    Q_OBJECT

private slots:
    // ---- (1) standaard verborgen ----

    void nothingIsVisibleByDefault() {
        const Game game = makeGame();
        const RangeSelection ranges;
        QVERIFY(!ranges.visibleZone(game).has_value());
        QVERIFY(!ranges.selected().has_value());
        QVERIFY(!ranges.hasPreview());
    }

    void towersThatExistDoNotShowTheirRange() {
        Game game = makeGame();
        buildAllTypes(game);
        const RangeSelection ranges;
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    // ---- (2) een toren selecteren ----

    void selectingATowerShowsItsRangeForEveryTowerType() {
        Game game = makeGame();
        buildAllTypes(game);
        for (std::size_t i = 0; i < kAllTypes.size(); ++i) {
            RangeSelection ranges;
            ranges.select(i);
            const std::optional<RangeZone> zone = ranges.visibleZone(game);
            QVERIFY(zone.has_value());
            const Tower& tower = *game.buildSlots()[i].tower;
            QVERIFY(zone->kind == RangeZone::Kind::Selected);
            QCOMPARE(zone->slot, i);
            QVERIFY(near(zone->radius, tower.range()));
            QVERIFY(near(zone->center.x, tower.position().x) && near(zone->center.y, tower.position().y));
        }
    }

    void selectingAnEmptyPadShowsNothing() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(5);                                   // de lege zesde bouwplek
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    void rangeReflectsTheCurrentTowerLevel() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(0);
        const std::uint8_t levelBefore = game.buildSlots()[0].tower->level();
        QVERIFY(near(ranges.visibleZone(game)->radius, game.buildSlots()[0].tower->range()));

        QVERIFY(game.upgradeTower(0));
        QVERIFY(game.buildSlots()[0].tower->level() > levelBefore);
        QVERIFY(near(ranges.visibleZone(game)->radius, game.buildSlots()[0].tower->range()));   // gelezen op het moment van tekenen
    }

    // ---- (3) wisselen en deselecteren ----

    void selectingAnotherTowerReplacesTheFirst() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(0);
        ranges.select(1);
        const std::optional<RangeZone> zone = ranges.visibleZone(game);
        QVERIFY(zone.has_value());
        QCOMPARE(zone->slot, std::size_t(1));               // alleen de nieuwe, de vorige is weg
        QVERIFY(near(zone->radius, game.buildSlots()[1].tower->range()));
        QCOMPARE(*ranges.selected(), std::size_t(1));
    }

    void deselectingHidesTheRange() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(2);
        QVERIFY(ranges.visibleZone(game).has_value());
        ranges.clearSelection();
        QVERIFY(!ranges.visibleZone(game).has_value());
        QVERIFY(!ranges.selected().has_value());
    }

    void deselectingTwiceIsHarmless() {
        Game game = makeGame();
        RangeSelection ranges;
        ranges.clearSelection();
        ranges.clearSelection();
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    void aSoldTowerShowsNoRange() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(1);
        QVERIFY(game.sellTower(1));
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    // ---- (4) voorbeeld tijdens het bouwen ----

    void previewShowsTheRangeOfTheTowerThatWillBeBuilt() {
        Game game = makeGame();
        for (const TowerType type : kAllTypes) {
            RangeSelection ranges;
            ranges.showPreview(5, type);
            const std::optional<RangeZone> zone = ranges.visibleZone(game);
            QVERIFY(zone.has_value());
            QVERIFY(zone->kind == RangeZone::Kind::Preview);
            const Vec2 pad = game.buildSlots()[5].position;
            QVERIFY(near(zone->center.x, pad.x) && near(zone->center.y, pad.y));
            QVERIFY(near(zone->radius, makeTower(type, pad, game.path())->range()));   // dezelfde berekening als de echte toren
        }
    }

    void previewDoesNotBuildAnything() {
        Game game = makeGame();
        const int goldBefore = game.player().gold();
        RangeSelection ranges;
        ranges.showPreview(5, TowerType::Mage);
        QVERIFY(ranges.visibleZone(game).has_value());
        QVERIFY(game.buildSlots()[5].tower == nullptr);
        QCOMPARE(game.player().gold(), goldBefore);
    }

    void previewIsOnlyForEmptyPads() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.showPreview(0, TowerType::Mage);              // bouwplek 0 is al bezet
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    void previewSwitchesWhenAnotherTowerIsHovered() {
        Game game = makeGame();
        RangeSelection ranges;
        ranges.showPreview(5, TowerType::Archer);
        ranges.showPreview(5, TowerType::Artillery);
        const Vec2 pad = game.buildSlots()[5].position;
        QVERIFY(near(ranges.visibleZone(game)->radius, makeTower(TowerType::Artillery, pad, game.path())->range()));
    }

    // ---- (5) annuleren en afronden ----

    void cancellingPlacementHidesThePreview() {
        Game game = makeGame();
        RangeSelection ranges;
        ranges.showPreview(5, TowerType::Cannon);
        QVERIFY(ranges.hasPreview());
        ranges.hidePreview();
        QVERIFY(!ranges.hasPreview());
        QVERIFY(!ranges.visibleZone(game).has_value());
    }

    void finishingPlacementKeepsTheNewTowerSelected() {
        Game game = makeGame();
        RangeSelection ranges;
        ranges.showPreview(5, TowerType::Mage);
        QVERIFY(game.buildTower(5, TowerType::Mage));
        ranges.select(5);                                    // wat GameView::build() doet
        QVERIFY(!ranges.hasPreview());
        const std::optional<RangeZone> zone = ranges.visibleZone(game);
        QVERIFY(zone.has_value());
        QVERIFY(zone->kind == RangeZone::Kind::Selected);
        QVERIFY(near(zone->radius, game.buildSlots()[5].tower->range()));
    }

    void hidingThePreviewKeepsTheSelectionOfAnotherTower() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(0);
        ranges.hidePreview();                                // er was geen voorbeeld: niets verandert
        QVERIFY(ranges.visibleZone(game).has_value());
    }

    // ---- (6) nooit meer dan één zone, niets anders verandert ----

    void selectionAndPreviewNeverShowTogether() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        ranges.select(0);
        ranges.showPreview(5, TowerType::Archer);
        QVERIFY(!ranges.selected().has_value());             // het voorbeeld verving de selectie
        QVERIFY(ranges.visibleZone(game)->kind == RangeZone::Kind::Preview);
        ranges.select(1);
        QVERIFY(!ranges.hasPreview());                       // en andersom
        QVERIFY(ranges.visibleZone(game)->kind == RangeZone::Kind::Selected);
    }

    void neverMoreThanOneZoneInAnySequenceOfActions() {
        Game game = makeGame();
        buildAllTypes(game);
        RangeSelection ranges;
        std::mt19937 random(12345);
        for (int step = 0; step < 500; ++step) {
            switch (random() % 4) {
            case 0: ranges.select(random() % 6); break;
            case 1: ranges.clearSelection(); break;
            case 2: ranges.showPreview(random() % 6, kAllTypes[random() % kAllTypes.size()]); break;
            default: ranges.hidePreview(); break;
            }
            QVERIFY(!(ranges.selected().has_value() && ranges.hasPreview()));
            const std::optional<RangeZone> zone = ranges.visibleZone(game);   // één zone of geen: nooit twee
            if (zone.has_value()) {
                QVERIFY(zone->kind == (ranges.selected().has_value() ? RangeZone::Kind::Selected : RangeZone::Kind::Preview));
            }
        }
    }

    void showingRangesDoesNotChangeRangeOrDamage() {
        Game game = makeGame();
        buildAllTypes(game);
        std::vector<double> ranges;
        std::vector<double> damages;
        for (const BuildSlot& buildSlot : game.buildSlots()) {
            if (buildSlot.tower != nullptr) {
                ranges.push_back(buildSlot.tower->range());
                damages.push_back(buildSlot.tower->damage());
            }
        }
        RangeSelection selection;
        for (std::size_t i = 0; i < 6; ++i) {
            selection.select(i);
            selection.visibleZone(game);
            selection.showPreview(i, TowerType::Archer);
            selection.visibleZone(game);
        }
        std::size_t index = 0;
        for (const BuildSlot& buildSlot : game.buildSlots()) {
            if (buildSlot.tower != nullptr) {
                QVERIFY(near(buildSlot.tower->range(), ranges[index]));
                QVERIFY(near(buildSlot.tower->damage(), damages[index]));
                ++index;
            }
        }
    }
};

QTEST_APPLESS_MAIN(TestRangeSelection)
#include "test_rangeselection.moc"
