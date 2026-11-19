#include <QtTest>
#include <stdexcept>
#include "ktd/Path.h"
#include "ktd/Skeleton.h"
#include "ktd/Goblin.h"

using namespace ktd;

class TestCore : public QObject {
    Q_OBJECT

private slots:
    void pathLength() {
        Path path({{0, 0}, {100, 0}, {100, 50}});
        QCOMPARE(path.length(), 150.0);
    }

    void pathPointAtStart() {
        Path path({{0, 0}, {100, 0}});
        QCOMPARE(path.pointAt(0).x, 0.0);
        QCOMPARE(path.pointAt(0).y, 0.0);
    }

    void pathPointAtSecondSegment() {
        Path path({{0, 0}, {100, 0}, {100, 50}});
        Vec2 p = path.pointAt(125);
        QCOMPARE(p.x, 100.0);
        QCOMPARE(p.y, 25.0);
    }

    void pathPointAtClampsToEnd() {
        Path path({{0, 0}, {100, 0}});
        Vec2 p = path.pointAt(9999);
        QCOMPARE(p.x, 100.0);
        QCOMPARE(p.y, 0.0);
    }

    void pathNeedsTwoWaypoints() {
        QVERIFY_EXCEPTION_THROWN(Path({{0, 0}}), std::invalid_argument);
    }

    void physicalDamageUsesArmor() {
        Skeleton skeleton;
        skeleton.takeDamage({50.0, DamageType::Physical});
        QCOMPARE(skeleton.health(), 55.0);
    }

    void magicDamageIgnoresArmor() {
        Skeleton skeleton;
        skeleton.takeDamage({50.0, DamageType::Magic});
        QCOMPARE(skeleton.health(), 50.0);
    }

    void enemyDiesAtZeroHealth() {
        Goblin goblin;
        QVERIFY(!goblin.isDead());
        goblin.takeDamage({1000.0, DamageType::Physical});
        QVERIFY(goblin.isDead());
        QCOMPARE(goblin.health(), 0.0);
    }

    void enemyMovesWithSpeed() {
        Goblin goblin;
        goblin.update(2.0);
        QCOMPARE(goblin.distance(), 220.0);
    }

    void deadEnemyDoesNotMove() {
        Goblin goblin;
        goblin.takeDamage({1000.0, DamageType::Magic});
        goblin.update(1.0);
        QCOMPARE(goblin.distance(), 0.0);
    }
};

QTEST_APPLESS_MAIN(TestCore)
#include "test_core.moc"