#include <QtTest>
#include "ktd/WaveManager.h"

using namespace ktd;

namespace {

Wave makeWave(std::size_t count, double interval)
{
    Wave wave;
    wave.enemies.assign(count, EnemyType::Grunt);
    wave.spawnInterval = interval;
    return wave;
}

}

class TestWave : public QObject {
    Q_OBJECT

private slots:
    void nothingBeforeStartDelay() {
        WaveManager manager({makeWave(2, 1.0)}, 3.0, 5.0);
        QVERIFY(manager.update(2.5).empty());
        QCOMPARE(manager.currentWave(), std::size_t(0));
    }

    void firstEnemyAfterStartDelay() {
        WaveManager manager({makeWave(2, 1.0)}, 3.0, 5.0);
        QCOMPARE(manager.update(3.0).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(1));
    }

    void enemiesFollowSpawnInterval() {
        WaveManager manager({makeWave(3, 1.0)}, 1.0, 5.0);
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));
        QCOMPARE(manager.update(0.5).size(), std::size_t(0));
        QCOMPARE(manager.update(0.5).size(), std::size_t(1));
    }

    void pauseBetweenWaves() {
        WaveManager manager({makeWave(1, 1.0), makeWave(1, 1.0)}, 1.0, 4.0);
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(1));
        QCOMPARE(manager.update(3.0).size(), std::size_t(0));
        QCOMPARE(manager.update(1.0).size(), std::size_t(1));
        QCOMPARE(manager.currentWave(), std::size_t(2));
    }

    void finishedAfterLastEnemy() {
        WaveManager manager({makeWave(2, 1.0)}, 0.5, 5.0);
        QVERIFY(!manager.finished());
        manager.update(0.5);
        QVERIFY(!manager.finished());
        manager.update(1.0);
        QVERIFY(manager.finished());
        QVERIFY(manager.update(100.0).empty());
    }

    void largeStepSpawnsSeveralEnemies() {
        WaveManager manager({makeWave(3, 1.0)}, 0.5, 6.0);
        QCOMPARE(manager.update(10.0).size(), std::size_t(3));
    }
};

QTEST_APPLESS_MAIN(TestWave)
#include "test_wave.moc"