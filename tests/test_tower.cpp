#include <QtTest>
#include <memory>
#include <vector>
#include "ktd/ArcherTower.h"
#include "ktd/MageTower.h"
#include "ktd/Cooldown.h"
#include "ktd/Grunt.h"
#include "ktd/Path.h"

using namespace ktd;

namespace {

std::unique_ptr<Enemy> gruntAfter(double seconds)
{
    auto grunt = std::make_unique<Grunt>();
    grunt->update(seconds);
    return grunt;
}

}

class TestTower : public QObject {
    Q_OBJECT

private slots:
    void cooldownStartsReady() {
        Cooldown<double> cooldown(1.0);
        QVERIFY(cooldown.ready());
    }

    void cooldownNotReadyAfterReset() {
        Cooldown<double> cooldown(1.0);
        cooldown.reset();
        QVERIFY(!cooldown.ready());
    }

    void cooldownReadyAfterInterval() {
        Cooldown<double> cooldown(1.0);
        cooldown.reset();
        cooldown.update(0.4);
        QVERIFY(!cooldown.ready());
        cooldown.update(0.6);
        QVERIFY(cooldown.ready());
    }

    void targetsFurthestEnemyInRange() {
        Path path({{0, 0}, {1000, 0}});
        ArcherTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(gruntAfter(7));
        enemies.push_back(gruntAfter(8));
        enemies.push_back(gruntAfter(14));
        QCOMPARE(tower.findTarget(enemies, path), enemies[1].get());
    }

    void targetIgnoresDeadEnemies() {
        Path path({{0, 0}, {1000, 0}});
        ArcherTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(gruntAfter(7));
        enemies.push_back(gruntAfter(8));
        enemies[1]->takeDamage({1000.0, DamageType::Magic});
        QCOMPARE(tower.findTarget(enemies, path), enemies[0].get());
    }

    void noTargetWhenOutOfRange() {
        Path path({{0, 0}, {1000, 0}});
        ArcherTower tower({0, 500});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(gruntAfter(7));
        QVERIFY(tower.findTarget(enemies, path) == nullptr);
    }

    void towerFiresOnCooldown() {
        Path path({{0, 0}, {1000, 0}});
        ArcherTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(gruntAfter(7));

        QCOMPARE(tower.update(0.0, enemies, path), enemies[0].get());
        QCOMPARE(enemies[0]->health(), 82.0);

        QVERIFY(tower.update(0.5, enemies, path) == nullptr);
        QCOMPARE(enemies[0]->health(), 82.0);

        QCOMPARE(tower.update(0.6, enemies, path), enemies[0].get());
        QCOMPARE(enemies[0]->health(), 64.0);
    }

    void upgradeRaisesLevelAndDamageUpToMax() {
        ArcherTower tower({0, 0});
        QCOMPARE(int(tower.level()), 1);
        for (int i = 0; i < 5; ++i) {
            tower.upgrade();
        }
        QCOMPARE(int(tower.level()), 3);
        QCOMPARE(tower.damage(), 31.25);
    }
    
    void mageIgnoresArmor() {
        Path path({{0, 0}, {1000, 0}});
        MageTower tower({450, 0});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(gruntAfter(7));
        QCOMPARE(tower.update(0.0, enemies, path), enemies[0].get());
        QCOMPARE(enemies[0]->health(), 65.0);
    }
};

QTEST_APPLESS_MAIN(TestTower)
#include "test_tower.moc"