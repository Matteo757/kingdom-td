#include <QtTest>
#include <memory>
#include <vector>
#include "ktd/ArcherTower.h"
#include "ktd/CannonTower.h"
#include "ktd/Game.h"
#include "ktd/SlotMenu.h"

using namespace ktd;

namespace {

Game makeGame(int gold)
{
    Wave wave;
    wave.enemies = {EnemyType::Goblin, EnemyType::Goblin, EnemyType::Goblin};
    wave.spawnInterval = 10.0;
    Game game(Path({{0, 0}, {1000, 0}}), Player(gold, 5),
              WaveManager(std::vector<Wave>{wave}, 0.0), {{500, 50}, {300, 50}});
    for (int i = 0; i < 3; ++i) {
        game.addTower(std::make_unique<CannonTower>(Vec2{100, 40}));
    }
    return game;
}

bool earnGoldUntil(Game& game, int target)
{
    for (int step = 0; step < 4000 && game.player().gold() < target; ++step) {
        game.update(0.05);
    }
    return game.player().gold() >= target;
}

bool anyEnabled(const std::vector<MenuEntry>& entries)
{
    for (const MenuEntry& entry : entries) {
        if (entry.enabled) {
            return true;
        }
    }
    return false;
}

const MenuEntry* find(const std::vector<MenuEntry>& entries, TowerType type)
{
    for (const MenuEntry& entry : entries) {
        if (entry.type == type) {
            return &entry;
        }
    }
    return nullptr;
}

}

class TestSlotMenu : public QObject {
    Q_OBJECT

private slots:
    void emptySlotListsEveryTowerWithItsPrice() {
        Game game = makeGame(1000);
        const std::vector<MenuEntry> entries = slotMenuEntries(game, 0);
        QCOMPARE(entries.size(), kBuildableTowers.size());
        for (std::size_t i = 0; i < entries.size(); ++i) {
            QVERIFY(entries[i].action == MenuAction::Build);
            QVERIFY(entries[i].type == kBuildableTowers[i]);
            QCOMPARE(entries[i].price, buildCost(kBuildableTowers[i]));
            QVERIFY(entries[i].enabled);
            QVERIFY(entries[i].text.find(std::to_string(entries[i].price) + " goud") != std::string::npos);
        }
    }

    void nothingIsAffordableWithTooLittleGold() {
        Game game = makeGame(buildCost(TowerType::Archer) - 1);
        QVERIFY(!anyEnabled(slotMenuEntries(game, 0)));
    }

    void earningGoldEnablesAnEntryWithoutReopeningTheMenu() {
        const int archer = buildCost(TowerType::Archer);
        Game game = makeGame(archer - 1);
        QVERIFY(!find(slotMenuEntries(game, 0), TowerType::Archer)->enabled);
        QVERIFY(earnGoldUntil(game, archer));
        QVERIFY(find(slotMenuEntries(game, 0), TowerType::Archer)->enabled);
    }

    void spendingGoldDisablesEntriesOfAnotherSlot() {
        const int archer = buildCost(TowerType::Archer);
        Game game = makeGame(archer + 5);
        QVERIFY(find(slotMenuEntries(game, 1), TowerType::Archer)->enabled);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(!find(slotMenuEntries(game, 1), TowerType::Archer)->enabled);
    }

    void entriesStayInSyncWithGameAfterEveryChange() {
        Game game = makeGame(buildCost(TowerType::Mage) - 1);
        for (int round = 0; round < 3; ++round) {
            const int goldBefore = game.player().gold();
            QVERIFY(earnGoldUntil(game, goldBefore + 1));
            for (const MenuEntry& entry : slotMenuEntries(game, 0)) {
                QCOMPARE(entry.enabled, game.player().gold() >= entry.price);
            }
        }
    }

    void upgradeEntryShowsNextLevelAndPrice() {
        Game game = makeGame(1000);
        game.buildTower(0, TowerType::Archer);
        const std::vector<MenuEntry> entries = slotMenuEntries(game, 0);
        QCOMPARE(entries.size(), std::size_t(2));   // upgrade (of max level) + verkopen
        QVERIFY(entries[0].action == MenuAction::Upgrade);
        QCOMPARE(entries[0].price, upgradeCost(1));
        QVERIFY(entries[0].text.find("level 2") != std::string::npos);
        QVERIFY(entries[0].enabled);
    }

    void earningGoldEnablesTheUpgradeWithoutReopeningTheMenu() {
        Game game = makeGame(buildCost(TowerType::Archer) + upgradeCost(1) - 1);
        game.buildTower(0, TowerType::Archer);
        QVERIFY(!slotMenuEntries(game, 0)[0].enabled);
        QVERIFY(earnGoldUntil(game, upgradeCost(1)));
        QVERIFY(slotMenuEntries(game, 0)[0].enabled);
    }

    void spendingGoldDisablesTheUpgrade() {
        Game game = makeGame(buildCost(TowerType::Archer) + upgradeCost(1) + 5);
        game.buildTower(0, TowerType::Archer);
        QVERIFY(slotMenuEntries(game, 0)[0].enabled);
        QVERIFY(game.buildTower(1, TowerType::Archer) || game.player().gold() < buildCost(TowerType::Archer));
        QVERIFY(!slotMenuEntries(game, 0)[0].enabled);
    }

    void upgradeEntryFollowsTheLevelAfterAnUpgrade() {
        Game game = makeGame(1000);
        game.buildTower(0, TowerType::Archer);
        QVERIFY(game.upgradeTower(0));
        const std::vector<MenuEntry> entries = slotMenuEntries(game, 0);
        QCOMPARE(entries[0].price, upgradeCost(2));
        QVERIFY(entries[0].text.find("level 3") != std::string::npos);
    }

    void maxLevelTowerOnlyShowsADisabledEntry() {
        Game game = makeGame(1000);
        game.buildTower(0, TowerType::Archer);
        game.upgradeTower(0);
        game.upgradeTower(0);
        const std::vector<MenuEntry> entries = slotMenuEntries(game, 0);
        QCOMPARE(entries.size(), std::size_t(2));   // upgrade (of max level) + verkopen
        QVERIFY(entries[0].action == MenuAction::MaxLevel);
        QVERIFY(!entries[0].enabled);
        QCOMPARE(entries[0].price, 0);
        QVERIFY(entries[0].text == "Maximaal level");
    }

    void builtSlotHasADifferentMenuThanAnEmptySlot() {
        Game game = makeGame(1000);
        QCOMPARE(slotMenuEntries(game, 0).size(), kBuildableTowers.size());
        game.buildTower(0, TowerType::Mage);
        QCOMPARE(slotMenuEntries(game, 0).size(), std::size_t(2));
        QCOMPARE(slotMenuEntries(game, 1).size(), kBuildableTowers.size());
    }

    void invalidSlotGivesNoEntries() {
        Game game = makeGame(1000);
        QVERIFY(slotMenuEntries(game, 99).empty());
    }

    void buildingAndUpgradingStillSpendTheRightAmount() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Cannon));
        QCOMPARE(game.player().gold(), 1000 - buildCost(TowerType::Cannon));
        QVERIFY(game.upgradeTower(0));
        QCOMPARE(game.player().gold(), 1000 - buildCost(TowerType::Cannon) - upgradeCost(1));
    }
};

QTEST_APPLESS_MAIN(TestSlotMenu)
#include "test_slotmenu.moc"
