#include <QtTest>
#include "ktd/Game.h"
#include "ktd/WaveManager.h"

using namespace ktd;

namespace {

Wave makeWave(std::size_t count, double interval = 1.0)
{
    Wave wave;
    wave.enemies.assign(count, EnemyType::Skeleton);
    wave.spawnInterval = interval;
    return wave;
}

WaveManager manualManager(std::vector<Wave> waves, double waveDelay = 20.0)
{
    return WaveManager(std::move(waves), 3.0, waveDelay, StartMode::Manual);
}

// Start de eerste wave en laat alle vijanden van die wave verschijnen (1 s interval), de bonustimer staat dan op waveDelay.
void startAndSpawnWholeWave(WaveManager& manager, std::size_t count)
{
    manager.startNextWave();
    manager.update(0.01);                                // eerste vijand
    for (std::size_t i = 1; i < count; ++i) {
        manager.update(1.0);                             // de rest, 1 per seconde
    }
}

Game manualGame(std::vector<Wave> waves)
{
    return Game(Path({{0, 0}, {100000, 0}}), Player(0, 5), manualManager(std::move(waves)));
}

}

class TestWaveStart : public QObject {
    Q_OBJECT

private slots:
    // ---- 1. Eerste wave handmatig
    void nothingSpawnsUntilStarted() {
        WaveManager manager = manualManager({makeWave(2)});
        QVERIFY(manager.update(100.0).empty());
        QCOMPARE(manager.currentWave(), std::size_t(0));
        QVERIFY(manager.canStartNextWave());
        QVERIFY(manager.firstWavePending());
    }

    void firstWaveGivesNoBonus() {
        WaveManager manager = manualManager({makeWave(2)});
        manager.update(100.0);
        QCOMPARE(manager.earlyBonus(), 0);
        QCOMPARE(manager.startNextWave(), 0);
    }

    void firstEnemyAppearsImmediatelyAfterStart() {
        WaveManager manager = manualManager({makeWave(2)});
        manager.startNextWave();
        QCOMPARE(manager.update(0.016).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(1));
        QVERIFY(!manager.firstWavePending());
    }

    void startButtonDisappearsAfterStart() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        manager.startNextWave();
        QVERIFY(!manager.canStartNextWave());
        manager.update(0.016);
        QVERIFY(!manager.canStartNextWave());           // wave is nog bezig met verschijnen
        QCOMPARE(manager.startNextWave(), 0);           // nogmaals klikken doet niets
        QCOMPARE(manager.currentWave(), std::size_t(1));
    }

    // ---- 2. Volgende wave handmatig
    void nextWaveBecomesAvailableAfterLastEnemySpawned() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        manager.startNextWave();
        manager.update(0.01);
        QVERIFY(!manager.canStartNextWave());
        manager.update(1.0);                            // tweede (laatste) vijand
        QVERIFY(manager.canStartNextWave());
        QVERIFY(!manager.firstWavePending());
    }

    void nextWaveStartsByItselfAfterTwentySeconds() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        QVERIFY(manager.update(19.9).empty());
        QCOMPARE(manager.currentWave(), std::size_t(1));
        QVERIFY(manager.canStartNextWave());
        QCOMPARE(manager.update(0.2).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void nextWaveStartsImmediatelyOnClick() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.startNextWave();
        QCOMPARE(manager.update(0.016).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void noWaveToStartAfterLastWave() {
        WaveManager manager = manualManager({makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        QVERIFY(manager.finished());
        QVERIFY(!manager.canStartNextWave());
        QCOMPARE(manager.startNextWave(), 0);
    }

    // ---- 3. Bonus
    void timerStartsAtTwentySecondsAfterLastEnemy() {
        WaveManager manager = manualManager({makeWave(3), makeWave(1)});
        startAndSpawnWholeWave(manager, 3);
        QCOMPARE(manager.timeUntilNextWave(), 20.0);
        QCOMPARE(manager.earlyBonus(), 60);
    }

    void bonusFollowsThreeGoldPerSecondTable() {
        const int remaining[] = {20, 19, 18, 15, 10, 5, 1, 0};
        const int expected[] = {60, 57, 54, 45, 30, 15, 3, 0};
        for (int i = 0; i < 8; ++i) {
            WaveManager manager = manualManager({makeWave(1), makeWave(1)});
            startAndSpawnWholeWave(manager, 1);
            manager.update(20.0 - remaining[i]);
            QCOMPARE(manager.earlyBonus(), expected[i]);
            QCOMPARE(manager.startNextWave(), expected[i]);
        }
    }

    void bonusUsesWholeSecondsRoundedDown() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.update(0.5);                            // 19.5 s over
        QCOMPARE(manager.earlyBonus(), 57);
        manager.update(0.6);                            // 18.9 s over
        QCOMPARE(manager.earlyBonus(), 54);
    }

    void bonusIsZeroJustBeforeTheTimerRunsOut() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.update(19.9);
        QVERIFY(manager.timeUntilNextWave() < 0.2);
        QCOMPARE(manager.earlyBonus(), 0);
        QVERIFY(manager.canStartNextWave());            // starten mag nog, alleen zonder bonus
    }

    void bonusIsClampedToSixty() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)}, 40.0);
        startAndSpawnWholeWave(manager, 1);
        QCOMPARE(manager.earlyBonus(), 60);
    }

    void timerContinuesWhileWaveStillRunsAndRestartsPerWave() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.update(5.0);
        QCOMPARE(manager.startNextWave(), 45);
        manager.update(0.01);                           // wave 2 verschijnt en is meteen klaar
        QCOMPARE(manager.timeUntilNextWave(), 20.0);    // timer begint opnieuw
        QCOMPARE(manager.earlyBonus(), 60);
    }

    void automaticModeIsUnchanged() {
        WaveManager manager({makeWave(1), makeWave(1)}, 1.0, 4.0);
        QVERIFY(!manager.canStartNextWave());
        QCOMPARE(manager.startNextWave(), 0);
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));
        QCOMPARE(manager.update(4.0).size(), std::size_t(1));   // start vanzelf na 4 s
    }

    // ---- Game: gouden bonus en overlap
    void gameDoesNotStartWavesByItself() {
        Game game = manualGame({makeWave(2)});
        for (int i = 0; i < 100; ++i) {
            game.update(1.0);
        }
        QVERIFY(game.enemies().empty());
        QVERIFY(game.canStartNextWave());
    }

    void gameAwardsEarlyBonusOnNextWaveClick() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        QCOMPARE(game.startNextWave(), 0);              // eerste wave: geen bonus
        QCOMPARE(game.player().gold(), 0);
        game.update(0.01);                              // laatste vijand van wave 1 verschijnt, timer = 20
        game.update(5.0);                               // 15 s over
        QCOMPARE(game.startNextWave(), 45);
        QCOMPARE(game.player().gold(), 45);
    }

    void wavesOverlapAndOldEnemiesStay() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.startNextWave();
        game.update(0.1);
        QCOMPARE(game.enemies().size(), std::size_t(1));
        QVERIFY(game.canStartNextWave());               // vijand van wave 1 leeft nog
        game.startNextWave();
        game.update(0.1);
        QCOMPARE(game.enemies().size(), std::size_t(2));
    }

    void cannotStartWaveWhenGameIsOver() {
        Game game(Path({{0, 0}, {10, 0}}), Player(0, 1), manualManager({makeWave(1), makeWave(1)}));
        game.startNextWave();
        for (int i = 0; i < 200 && !game.isGameOver(); ++i) {
            game.update(0.1);
        }
        QVERIFY(game.isGameOver());
        QVERIFY(!game.canStartNextWave());
        QCOMPARE(game.startNextWave(), 0);
    }

    void builtInLevelsWaitForThePlayer() {
        Game game = Game::level(1);
        for (int i = 0; i < 100; ++i) {
            game.update(1.0);
        }
        QVERIFY(game.enemies().empty());
        QCOMPARE(game.waves().currentWave(), std::size_t(0));
        QVERIFY(game.waves().firstWavePending());
    }
};

QTEST_APPLESS_MAIN(TestWaveStart)
#include "test_wavestart.moc"
