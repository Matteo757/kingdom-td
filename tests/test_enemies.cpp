#include <QtTest>
#include "ktd/DarkMage.h"
#include "ktd/EnemyFactory.h"
#include "ktd/Goblin.h"
#include "ktd/Orc.h"
#include "ktd/Skeleton.h"

using namespace ktd;

class TestEnemies : public QObject {
    Q_OBJECT

private slots:
    void orcResistsPhysicalDamage() {
        Orc orc;
        Skeleton skeleton;
        orc.takeDamage({40.0, DamageType::Physical});        // 40 * (100-40)/100 = 24
        skeleton.takeDamage({40.0, DamageType::Physical});   // 40 * (100-10)/100 = 36
        QCOMPARE(orc.health(), 226.0);
        QCOMPARE(skeleton.health(), 64.0);
    }

    void darkMageResistsMagicDamage() {
        DarkMage mage;
        mage.takeDamage({40.0, DamageType::Magic});          // 40 * (100-50)/100 = 20
        QCOMPARE(mage.health(), 100.0);
    }

    void darkMageTakesFullPhysicalDamage() {
        DarkMage mage;
        mage.takeDamage({40.0, DamageType::Physical});
        QCOMPARE(mage.health(), 80.0);
    }

    void goblinIsFasterThanOrc() {
        Goblin goblin;
        Orc orc;
        QVERIFY(goblin.speed() > orc.speed());
    }

    void factoryCreatesCorrectTypes() {
        QCOMPARE(QString::fromStdString(makeEnemy(EnemyType::Goblin)->name()), QString("Goblin"));
        QCOMPARE(QString::fromStdString(makeEnemy(EnemyType::Skeleton)->name()), QString("Skeleton"));
        QCOMPARE(QString::fromStdString(makeEnemy(EnemyType::Orc)->name()), QString("Orc"));
        QCOMPARE(QString::fromStdString(makeEnemy(EnemyType::DarkMage)->name()), QString("DarkMage"));
    }
};

QTEST_APPLESS_MAIN(TestEnemies)
#include "test_enemies.moc"