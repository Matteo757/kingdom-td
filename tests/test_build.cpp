#include <QtTest>
#include "ktd/Game.h"
#include "ktd/Soldier.h"

using namespace ktd;

namespace {

Game makeGame(int gold)
{
    return Game(Path({{0, 0}, {1000, 0}}), Player(gold, 5),
                WaveManager(std::vector<Wave>{}), {{500, 50}, {300, 50}});
}

}

class TestBuild : public QObject {
    Q_OBJECT

private slots:
    void buildSpendsGoldAndPlacesTower() {
        Game game = makeGame(200);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QCOMPARE(game.player().gold(), 200 - buildCost(TowerType::Archer));
        QVERIFY(game.buildSlots()[0].tower != nullptr);
        QCOMPARE(int(game.buildSlots()[0].tower->type()), int(TowerType::Archer));
        QCOMPARE(game.towers().size(), std::size_t(1));
    }

    void buildFailsWithoutEnoughGold() {
        Game game = makeGame(50);
        QVERIFY(!game.canBuild(0, TowerType::Archer));
        QVERIFY(!game.buildTower(0, TowerType::Archer));
        QCOMPARE(game.player().gold(), 50);
        QVERIFY(game.buildSlots()[0].tower == nullptr);
    }

    void buildFailsOnOccupiedSlot() {
        Game game = makeGame(500);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(!game.buildTower(0, TowerType::Mage));
        QCOMPARE(game.towers().size(), std::size_t(1));
    }

    void buildFailsOnInvalidSlot() {
        Game game = makeGame(500);
        QVERIFY(!game.buildTower(9, TowerType::Archer));
        QCOMPARE(game.player().gold(), 500);
    }

    void everyTowerTypeCanBeBuilt() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        QVERIFY(game.buildTower(1, TowerType::Artillery));
        QCOMPARE(int(game.buildSlots()[0].tower->type()), int(TowerType::Barracks));
        QCOMPARE(int(game.buildSlots()[1].tower->type()), int(TowerType::Artillery));
    }

    void upgradeSpendsGoldAndRaisesLevel() {
        Game game = makeGame(300);
        game.buildTower(0, TowerType::Archer);
        QVERIFY(game.upgradeTower(0));
        QCOMPARE(int(game.buildSlots()[0].tower->level()), 2);
        QCOMPARE(game.player().gold(), 300 - buildCost(TowerType::Archer) - upgradeCost(1));
    }

    void upgradeFailsWithoutGold() {
        Game game = makeGame(buildCost(TowerType::Archer));
        game.buildTower(0, TowerType::Archer);
        QVERIFY(!game.canUpgrade(0));
        QVERIFY(!game.upgradeTower(0));
        QCOMPARE(int(game.buildSlots()[0].tower->level()), 1);
    }

    void upgradeFailsOnEmptySlot() {
        Game game = makeGame(500);
        QVERIFY(!game.upgradeTower(0));
        QCOMPARE(game.player().gold(), 500);
    }

    void upgradeStopsAtMaxLevel() {
        Game game = makeGame(1000);
        game.buildTower(0, TowerType::Archer);
        QVERIFY(game.upgradeTower(0));
        QVERIFY(game.upgradeTower(0));
        const int gold = game.player().gold();
        QVERIFY(!game.upgradeTower(0));
        QCOMPARE(int(game.buildSlots()[0].tower->level()), int(Tower::kMaxLevel));
        QCOMPARE(game.player().gold(), gold);
    }

    void upgradeCostsRiseWithLevel() {
        QVERIFY(upgradeCost(2) > upgradeCost(1));
    }

    void soldierUpgradeRaisesHealth() {
        Soldier soldier(40.0, 10.0, 1.0, 8.0);
        soldier.upgrade();
        QCOMPARE(soldier.health(), 50.0);
    }
};

QTEST_APPLESS_MAIN(TestBuild)
#include "test_build.moc"