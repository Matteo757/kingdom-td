#include <QtTest>
#include <algorithm>
#include <stdexcept>
#include "ktd/WaveFactory.h"

using namespace ktd;

namespace {

bool contains(const Wave& wave, EnemyType type)
{
    return std::find(wave.enemies.begin(), wave.enemies.end(), type) != wave.enemies.end();
}

}

class TestProgression : public QObject {
    Q_OBJECT

private slots:
    void firstTwoWavesHaveOnlyGoblinsAndSkeletons() {
        for (int number = 1; number <= 2; ++number) {
            const Wave wave = buildWave(number);
            QVERIFY(contains(wave, EnemyType::Goblin));
            QVERIFY(contains(wave, EnemyType::Skeleton));
            QVERIFY(!contains(wave, EnemyType::Orc));
            QVERIFY(!contains(wave, EnemyType::DarkMage));
        }
    }

    void orcAppearsFromWaveThree() {
        QVERIFY(!contains(buildWave(2), EnemyType::Orc));
        QVERIFY(contains(buildWave(3), EnemyType::Orc));
        QVERIFY(contains(buildWave(4), EnemyType::Orc));
        QVERIFY(!contains(buildWave(4), EnemyType::DarkMage));
    }

    void darkMageAppearsFromWaveFive() {
        QVERIFY(!contains(buildWave(4), EnemyType::DarkMage));
        QVERIFY(contains(buildWave(5), EnemyType::DarkMage));
    }

    void allTypesFromWaveFiveOn() {
        for (int number = 5; number <= 10; ++number) {
            const Wave wave = buildWave(number);
            QVERIFY(contains(wave, EnemyType::Goblin));
            QVERIFY(contains(wave, EnemyType::Skeleton));
            QVERIFY(contains(wave, EnemyType::Orc));
            QVERIFY(contains(wave, EnemyType::DarkMage));
        }
    }

    void everyWaveIsBiggerThanThePreviousOne() {
        for (int number = 2; number <= 10; ++number) {
            QVERIFY(buildWave(number).enemies.size() > buildWave(number - 1).enemies.size());
        }
    }

    void enemiesSpawnFasterButIntervalStaysPositive() {
        QVERIFY(buildWave(10).spawnInterval < buildWave(1).spawnInterval);
        QVERIFY(buildWave(100).spawnInterval > 0.0);
    }

    void buildWavesReturnsRequestedCount() {
        QCOMPARE(buildWaves(10).size(), std::size_t(10));
    }

    void nextWaveStartsTwentySecondsAfterLastEnemy() {
        WaveManager manager(buildWaves(2), 0.0, kWaveDelay);
        const std::size_t firstWaveSize = buildWave(1).enemies.size();
        std::size_t spawned = 0;
        double time = 0.0;
        double lastSpawnOfWaveOne = 0.0;
        double firstSpawnOfWaveTwo = 0.0;
        while (firstSpawnOfWaveTwo == 0.0 && time < 200.0) {
            const std::size_t now = manager.update(0.05).size();
            time += 0.05;
            if (now == 0) {
                continue;
            }
            if (spawned < firstWaveSize) {
                spawned += now;
                if (spawned == firstWaveSize) {
                    lastSpawnOfWaveOne = time;
                }
            } else {
                firstSpawnOfWaveTwo = time;
            }
        }
        QVERIFY(qAbs((firstSpawnOfWaveTwo - lastSpawnOfWaveOne) - kWaveDelay) < 0.1);
    }
    
    void invalidWaveNumberThrows() {
        QVERIFY_EXCEPTION_THROWN(buildWave(0), std::invalid_argument);
    }
};

QTEST_APPLESS_MAIN(TestProgression)
#include "test_progression.moc"