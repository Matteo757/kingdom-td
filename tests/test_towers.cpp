#include <QtTest>
#include <memory>
#include <vector>
#include "ktd/ArtilleryTower.h"
#include "ktd/BarracksTower.h"
#include "ktd/CannonTower.h"
#include "ktd/Path.h"
#include "ktd/Skeleton.h"

using namespace ktd;

namespace {

std::unique_ptr<Enemy> skeletonAfter(double seconds)
{
    auto skeleton = std::make_unique<Skeleton>();
    skeleton->update(seconds);
    return skeleton;
}

}

class TestTowers : public QObject {
    Q_OBJECT

private slots:
    void closestDistanceOnStraightPath() {
        Path path({{0, 0}, {1000, 0}});
        QCOMPARE(path.closestDistance({450, 50}), 450.0);
    }

    void closestDistanceOnSecondSegment() {
        Path path({{0, 0}, {100, 0}, {100, 100}});
        QCOMPARE(path.closestDistance({150, 60}), 160.0);
    }

    void cannonDealsHeavyPhysicalDamage() {
        Path path({{0, 0}, {1000, 0}});
        CannonTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(7));
        QCOMPARE(tower.update(0.0, enemies, path), enemies[0].get());
        QCOMPARE(enemies[0]->health(), 46.0);
    }

    void cannonIsSlowerThanArcher() {
        Path path({{0, 0}, {1000, 0}});
        CannonTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(7));
        tower.update(0.0, enemies, path);
        QVERIFY(tower.update(1.0, enemies, path) == nullptr);
        QCOMPARE(enemies[0]->health(), 46.0);
    }

    void artilleryHitsEnemiesWithinSplashRadius() {
        Path path({{0, 0}, {1000, 0}});
        ArtilleryTower tower({400, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(3));
        enemies.push_back(skeletonAfter(8));
        enemies.push_back(skeletonAfter(9));
        QCOMPARE(tower.update(0.0, enemies, path), enemies[2].get());
        QCOMPARE(enemies[2]->health(), 59.5);
        QCOMPARE(enemies[1]->health(), 59.5);
        QCOMPARE(enemies[0]->health(), 100.0);
    }

    void barracksBlocksEnemyInReach() {
        Path path({{0, 0}, {1000, 0}});
        BarracksTower tower({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(8));
        tower.update(0.0, enemies, path);
        QVERIFY(enemies[0]->isBlocked());
        enemies[0]->update(1.0);
        QCOMPARE(enemies[0]->distance(), 480.0);
    }

    void barracksIgnoresEnemyOutOfReach() {
        Path path({{0, 0}, {1000, 0}});
        BarracksTower tower({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(3));
        tower.update(0.0, enemies, path);
        QVERIFY(!enemies[0]->isBlocked());
    }

    void barracksBlocksAtMostThreeEnemies() {
        Path path({{0, 0}, {1000, 0}});
        BarracksTower tower({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        for (int i = 0; i < 4; ++i) {
            enemies.push_back(skeletonAfter(8));
        }
        tower.update(0.0, enemies, path);
        int blocked = 0;
        for (const auto& enemy : enemies) {
            blocked += enemy->isBlocked() ? 1 : 0;
        }
        QCOMPARE(blocked, 3);
    }

    void soldierDamagesBlockedEnemy() {
        Path path({{0, 0}, {1000, 0}});
        BarracksTower tower({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(8));
        tower.update(0.0, enemies, path);
        QCOMPARE(enemies[0]->health(), 91.0);
    }

    void enemyIsReleasedWhenSoldierDies() {
        Path path({{0, 0}, {1000, 0}});
        BarracksTower tower({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(8));
        tower.update(0.0, enemies, path);
        tower.update(10.0, enemies, path);
        QVERIFY(!enemies[0]->isBlocked());
    }

    void soldierRespawnsAfterDelay() {
        Soldier soldier(40.0, 10.0, 1.0, 8.0);
        soldier.takeDamage(100.0);
        QVERIFY(!soldier.alive());
        soldier.update(7.9);
        QVERIFY(!soldier.alive());
        soldier.update(0.2);
        QVERIFY(soldier.alive());
        QCOMPARE(soldier.health(), 40.0);
    }
};

QTEST_APPLESS_MAIN(TestTowers)
#include "test_towers.moc"