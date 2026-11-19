#include <QtTest>
#include "ktd/Player.h"

using namespace ktd;

class TestPlayer : public QObject {
    Q_OBJECT

private slots:
    void startsWithGivenValues() {
        Player player(100, 20);
        QCOMPARE(player.gold(), 100);
        QCOMPARE(int(player.lives()), 20);
    }

    void addGoldIncreasesGold() {
        Player player(100, 20);
        player.addGold(25);
        QCOMPARE(player.gold(), 125);
    }

    void spendReducesGold() {
        Player player(100, 20);
        QVERIFY(player.spend(40));
        QCOMPARE(player.gold(), 60);
    }

    void spendFailsWhenNotEnoughGold() {
        Player player(30, 20);
        QVERIFY(!player.spend(40));
        QCOMPARE(player.gold(), 30);
    }

    void loseLifeReducesLives() {
        Player player(0, 2);
        player.loseLife();
        QCOMPARE(int(player.lives()), 1);
        QVERIFY(!player.isDead());
    }

    void loseLifeStopsAtZero() {
        Player player(0, 1);
        player.loseLife();
        player.loseLife();
        QCOMPARE(int(player.lives()), 0);
        QVERIFY(player.isDead());
    }
};

QTEST_APPLESS_MAIN(TestPlayer)
#include "test_player.moc"