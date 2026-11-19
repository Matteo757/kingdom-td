#include <QtTest>
#include <memory>
#include <vector>
#include "ktd/Fireball.h"
#include "ktd/Game.h"
#include "ktd/Skeleton.h"

using namespace ktd;

namespace {

std::unique_ptr<Enemy> skeletonAfter(double seconds)
{
    auto skeleton = std::make_unique<Skeleton>();
    skeleton->update(seconds);
    return skeleton;
}

Game makeGame()
{
    Wave wave;
    wave.enemies = {EnemyType::Skeleton};
    return Game(Path({{0, 0}, {1000, 0}}), Player(0, 5), WaveManager(std::vector<Wave>{wave}, 0.0));
}

}

class TestSpell : public QObject {
    Q_OBJECT

private slots:
    void fireballDamagesEnemiesInRadius() {
        Path path({{0, 0}, {1000, 0}});
        Fireball fireball;
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(8));
        enemies.push_back(skeletonAfter(9));
        enemies.push_back(skeletonAfter(3));
        fireball.cast(enemies, path, {500, 0});
        QCOMPARE(enemies[0]->health(), 20.0);
        QCOMPARE(enemies[1]->health(), 20.0);
        QCOMPARE(enemies[2]->health(), 100.0);
    }

    void fireballMissesEnemyOutsideRadius() {
        Path path({{0, 0}, {1000, 0}});
        Fireball fireball;
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(10));
        fireball.cast(enemies, path, {500, 0});
        QCOMPARE(enemies[0]->health(), 100.0);
    }

    void spellHasCooldown() {
        Path path({{0, 0}, {1000, 0}});
        Fireball fireball;
        std::vector<std::unique_ptr<Enemy>> enemies;
        QVERIFY(fireball.ready());
        QVERIFY(fireball.tryCast(enemies, path, {0, 0}));
        QVERIFY(!fireball.ready());
        QVERIFY(!fireball.tryCast(enemies, path, {0, 0}));
        fireball.update(24.0);
        QVERIFY(!fireball.ready());
        fireball.update(1.0);
        QVERIFY(fireball.ready());
    }

    void gameCastsFireballOnlyWhenReady() {
        Game game = makeGame();
        game.update(0.1);
        QVERIFY(game.fireballReady());
        QVERIFY(game.castFireball({6, 0}));
        QCOMPARE(game.enemies()[0]->health(), 20.0);
        QVERIFY(!game.fireballReady());
        QVERIFY(!game.castFireball({6, 0}));
        QCOMPARE(game.enemies()[0]->health(), 20.0);
    }

    void gameFireballRechargesOverTime() {
        Game game = makeGame();
        game.update(0.1);
        game.castFireball({6, 0});
        game.update(26.0);
        QVERIFY(game.fireballReady());
    }
};

QTEST_APPLESS_MAIN(TestSpell)
#include "test_spell.moc"