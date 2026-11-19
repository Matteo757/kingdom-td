#include <QtTest>
#include "ktd/Game.h"
#include "ktd/MageTower.h"

using namespace ktd;

namespace {

Wave oneSkeleton()
{
    Wave wave;
    wave.enemies = {EnemyType::Skeleton};
    return wave;
}

Game makeGame(double pathLength, std::uint8_t lives)
{
    return Game(Path({{0, 0}, {pathLength, 0}}), Player(0, lives),
                WaveManager(std::vector<Wave>{oneSkeleton()}, 0.0));
}

}

class TestGame : public QObject {
    Q_OBJECT

private slots:
    void towerKillsEnemyAndGivesGold() {
        Game game = makeGame(2000.0, 5);
        game.addTower(std::make_unique<MageTower>(Vec2{500, 50}));

        int gold = 0;
        for (int i = 0; i < 300 && !game.isWon(); ++i) {
            gold += game.update(0.1).goldEarned;
        }
        QVERIFY(game.isWon());
        QCOMPARE(gold, 10);
        QCOMPARE(game.player().gold(), 10);
        QCOMPARE(int(game.player().lives()), 5);
    }

    void enemyReachingEndCostsLife() {
        Game game = makeGame(100.0, 3);

        int livesLost = 0;
        for (int i = 0; i < 40; ++i) {
            livesLost += game.update(0.1).livesLost;
        }
        QCOMPARE(livesLost, 1);
        QCOMPARE(int(game.player().lives()), 2);
        QVERIFY(game.enemies().empty());
    }

    void gameOverWhenNoLivesLeft() {
        Game game = makeGame(100.0, 1);
        for (int i = 0; i < 40; ++i) {
            game.update(0.1);
        }
        QVERIFY(game.isGameOver());
        QVERIFY(!game.isWon());
    }

    void spawnEventContainsEnemy() {
        Game game = makeGame(1000.0, 3);
        const FrameEvents events = game.update(0.1);
        QCOMPARE(events.spawned.size(), std::size_t(1));
        QCOMPARE(game.enemies().size(), std::size_t(1));
        QCOMPARE(events.spawned[0].enemy, game.enemies()[0].get());
    }
};

QTEST_APPLESS_MAIN(TestGame)
#include "test_game.moc"