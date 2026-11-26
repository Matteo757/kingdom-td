#include <QtTest>
#include <vector>
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

WaveManager manualManager(std::vector<Wave> waves)
{
    return WaveManager(std::move(waves), 3.0, 20.0, StartMode::Manual);
}

// Start de eerste wave en laat alle vijanden van die wave verschijnen; de bonustimer staat dan op 20 s.
void startAndSpawnWholeWave(WaveManager& manager, std::size_t count)
{
    manager.startNextWave();
    manager.update(0.01);
    for (std::size_t i = 1; i < count; ++i) {
        manager.update(1.0);
    }
}

// Lange weg, zodat vijanden die niet geraakt worden lang blijven leven.
Game manualGame(std::vector<Wave> waves)
{
    return Game(Path({{0, 0}, {100000, 0}}), Player(0, 5), manualManager(std::move(waves)));
}

}

class TestAutoStart : public QObject {
    Q_OBJECT

private slots:
    // ---- Eerste wave: nooit automatisch
    void firstWaveNeverStartsByItself() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        for (int i = 0; i < 1000; ++i) {
            QVERIFY(manager.update(1.0).empty());
        }
        QCOMPARE(manager.currentWave(), std::size_t(0));
        QVERIFY(manager.firstWavePending());
        QVERIFY(manager.canStartNextWave());
    }

    void firstWaveStillGivesNoBonusAfterAVeryLongPreparation() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.update(500.0);
        QCOMPARE(game.startNextWave(), 0);
        QCOMPARE(game.player().gold(), 0);
    }

    // ---- Volgende wave: start vanzelf als de timer op is
    void nothingStartsBeforeTheTimerExpires() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        QCOMPARE(manager.timeUntilNextWave(), 20.0);
        QVERIFY(manager.update(19.9).empty());
        QCOMPARE(manager.currentWave(), std::size_t(1));
        QVERIFY(manager.canStartNextWave());
    }

    void nextWaveStartsAfterTwentySecondsWithoutAClick() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        double waited = 0.0;
        std::size_t spawned = 0;
        while (spawned == 0 && waited < 60.0) {
            spawned += manager.update(0.05).size();
            waited += 0.05;
        }
        QVERIFY(qAbs(waited - 20.0) < 0.1);
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void buttonDisappearsWhenTheWaveStartsAutomatically() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        manager.update(20.0);
        QVERIFY(!manager.canStartNextWave());
    }

    void automaticStartHappensExactlyOnce() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        std::size_t spawned = manager.update(20.0).size();
        QCOMPARE(spawned, std::size_t(1));
        QCOMPARE(manager.startNextWave(), 0);            // een klik erbovenop start geen tweede wave
        spawned += manager.update(1.0).size();
        QCOMPARE(spawned, std::size_t(2));               // precies de twee vijanden van wave 2
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void everyWaveStartsAutomaticallyUntilTheLastOne() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2), makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        std::size_t total = 2;
        std::vector<double> startTimes;
        std::size_t lastWave = manager.currentWave();
        double time = 0.0;
        while (!manager.finished() && time < 500.0) {
            total += manager.update(0.1).size();
            time += 0.1;
            if (manager.currentWave() != lastWave) {
                lastWave = manager.currentWave();
                startTimes.push_back(time);
            }
        }
        QVERIFY(manager.finished());
        QCOMPARE(manager.currentWave(), std::size_t(4));
        QCOMPARE(total, std::size_t(8));
        QCOMPARE(startTimes.size(), std::size_t(3));
        for (std::size_t i = 1; i < startTimes.size(); ++i) {
            QVERIFY(qAbs((startTimes[i] - startTimes[i - 1]) - 21.0) < 0.3);   // 20 s timer + 1 s tot de tweede vijand
        }
    }

    void noAutomaticStartAfterTheLastWave() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.update(25.0);
        QVERIFY(manager.finished());
        QVERIFY(manager.update(1000.0).empty());
        QVERIFY(!manager.canStartNextWave());
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void automaticModeIsStillUnchanged() {
        WaveManager manager({makeWave(1), makeWave(1)}, 1.0, 4.0);
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));
        QCOMPARE(manager.update(4.0).size(), std::size_t(1));
    }

    // ---- Handmatig vroeg starten annuleert de automatische start
    void manualStartBeforeExpiryCancelsTheAutomaticStart() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        manager.update(10.5);                             // nog 9,5 s
        QCOMPARE(manager.startNextWave(), 27);            // bonus ongewijzigd: floor(9,5) * 3
        manager.update(0.01);
        manager.update(1.0);                              // wave 2 is klaar met verschijnen, timer begint opnieuw
        QCOMPARE(manager.currentWave(), std::size_t(2));
        QCOMPARE(manager.timeUntilNextWave(), 20.0);

        // Het moment waarop de oude timer afgelopen zou zijn (9,5 s na de klik) gebeurt er niets.
        for (int i = 0; i < 38; ++i) {
            QVERIFY(manager.update(0.5).empty());
        }
        QCOMPARE(manager.currentWave(), std::size_t(2));

        // De nieuwe timer start wave 3 pas na zijn eigen 20 seconden.
        QCOMPARE(manager.update(1.5).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(3));
    }

    // ---- Klik en einde van de timer tegelijk
    void clickAfterTheTimerRanOutInTheSameFrameStartsNoSecondWave() {
        WaveManager manager = manualManager({makeWave(2), makeWave(2), makeWave(2)});
        startAndSpawnWholeWave(manager, 2);
        manager.update(19.99);
        QCOMPARE(manager.update(0.05).size(), std::size_t(1));   // de timer loopt af: wave 2 start vanzelf
        QVERIFY(!manager.canStartNextWave());                    // de knop is meteen weg
        QCOMPARE(manager.startNextWave(), 0);                    // de klik in hetzelfde frame doet niets meer
        QCOMPARE(manager.currentWave(), std::size_t(2));
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));    // alleen de tweede vijand van wave 2
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void aHugeTimeStepStartsTheNextWaveWithOnlyItsFirstEnemy() {
        WaveManager manager = manualManager({makeWave(3), makeWave(3)});
        startAndSpawnWholeWave(manager, 3);
        QCOMPARE(manager.update(500.0).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void clickBeforeTheTimerRunsOutInTheSameFrameStartsNoSecondWave() {
        WaveManager manager = manualManager({makeWave(1), makeWave(1), makeWave(1)});
        startAndSpawnWholeWave(manager, 1);
        manager.update(19.99);
        QCOMPARE(manager.startNextWave(), 0);                    // 0,01 s over: floor geeft geen bonus
        QCOMPARE(manager.update(0.05).size(), std::size_t(1));   // alleen de wave van de klik verschijnt
        QVERIFY(manager.update(0.05).empty());
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    // ---- Game: goud en overlap
    void automaticStartAwardsNoGold() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.startNextWave();
        game.update(0.01);
        for (int i = 0; i < 300 && game.waves().currentWave() < 2; ++i) {
            game.update(0.1);
        }
        QCOMPARE(game.waves().currentWave(), std::size_t(2));
        QCOMPARE(game.player().gold(), 0);
    }

    void earlyStartStillAwardsTheBonus() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        QCOMPARE(game.startNextWave(), 0);
        game.update(0.01);
        game.update(4.5);                                         // nog 15,5 s
        QCOMPARE(game.startNextWave(), 45);
        QCOMPARE(game.player().gold(), 45);
    }

    void earlyBonusIsCappedAtSixtyGold() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.startNextWave();
        game.update(0.01);
        QCOMPARE(game.startNextWave(), 60);
        QCOMPARE(game.player().gold(), 60);
    }

    void automaticallyStartedWavesOverlapWithEarlierWaves() {
        Game game = manualGame({makeWave(2), makeWave(2), makeWave(2)});
        game.startNextWave();
        for (int i = 0; i < 2000 && game.waves().currentWave() < 3; ++i) {
            game.update(0.1);
        }
        QCOMPARE(game.waves().currentWave(), std::size_t(3));
        QVERIFY(game.enemies().size() >= 5);                       // niemand is verwijderd bij een nieuwe wave
        QVERIFY(game.enemies().front()->distance() > game.enemies().back()->distance());
    }

    // ---- Pauze
    void pausedGameDoesNotCountDown() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.startNextWave();
        game.update(0.01);
        game.update(5.0);
        QCOMPARE(game.waves().timeUntilNextWave(), 15.0);

        game.setPaused(true);
        QVERIFY(game.isPaused());
        game.update(100.0);
        QCOMPARE(game.waves().timeUntilNextWave(), 15.0);
        QCOMPARE(game.waves().currentWave(), std::size_t(1));

        game.setPaused(false);
        game.update(5.0);
        QCOMPARE(game.waves().timeUntilNextWave(), 10.0);
    }

    void pausedGameDoesNotStartTheNextWaveWhenTheTimerWouldExpire() {
        Game game = manualGame({makeWave(1), makeWave(1)});
        game.startNextWave();
        game.update(0.01);
        game.update(19.5);                                         // nog 0,5 s
        game.setPaused(true);
        game.update(10.0);
        QCOMPARE(game.waves().currentWave(), std::size_t(1));
        QCOMPARE(game.enemies().size(), std::size_t(1));

        game.setPaused(false);
        game.update(0.6);
        QCOMPARE(game.waves().currentWave(), std::size_t(2));
        QCOMPARE(game.enemies().size(), std::size_t(2));
        QCOMPARE(game.player().gold(), 0);
    }

    void pausedGameFreezesEnemies() {
        Game game = manualGame({makeWave(1)});
        game.startNextWave();
        game.update(0.5);
        const double before = game.enemies().front()->distance();
        game.setPaused(true);
        game.update(5.0);
        QCOMPARE(game.enemies().front()->distance(), before);
    }

    void startingTheFirstWaveWhilePausedOnlySpawnsAfterResuming() {
        Game game = manualGame({makeWave(1)});
        game.setPaused(true);
        game.startNextWave();
        game.update(1.0);
        QVERIFY(game.enemies().empty());
        game.setPaused(false);
        game.update(0.01);
        QCOMPARE(game.enemies().size(), std::size_t(1));
    }

    void gameIsNotPausedByDefault() {
        Game game = manualGame({makeWave(1)});
        QVERIFY(!game.isPaused());
    }
};

QTEST_APPLESS_MAIN(TestAutoStart)
#include "test_autostart.moc"
