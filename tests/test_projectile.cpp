#include <QtTest>
#include <memory>
#include <vector>
#include "ktd/ArcherTower.h"
#include "ktd/ArtilleryTower.h"
#include "ktd/Game.h"
#include "ktd/LaunchPoints.h"
#include "ktd/MageTower.h"
#include "ktd/Path.h"
#include "ktd/Projectile.h"
#include "ktd/Skeleton.h"

using namespace ktd;

namespace {

std::unique_ptr<Enemy> skeletonAfter(double seconds)
{
    auto skeleton = std::make_unique<Skeleton>();
    skeleton->update(seconds);
    return skeleton;
}

Projectile arrowAt(Enemy& target, const Path& path, double splash = 0.0, TowerType type = TowerType::Archer)
{
    return Projectile(type, 1, {0, 100}, target, path, {50.0, DamageType::Physical}, splash);
}

}

class TestProjectile : public QObject {
    Q_OBJECT

private slots:
    // ---- lanceerpunten: de pijlen beginnen bij de boogschutters ----

    void archerArrowsStartAtTheArcherPositionsAndAlternate() {
        Path path({{0, 0}, {2000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        ArcherTower tower({500, 300});
        const std::span<const LaunchPoint> points = launchPoints(TowerType::Archer, 1);
        QCOMPARE(points.size(), std::size_t(2));

        for (int shot = 0; shot < 4; ++shot) {
            const LaunchPoint& point = points[shot % 2];     // links, rechts, links, rechts
            const Projectile arrow = *tower.attack(*enemies[0], path);
            QCOMPARE(arrow.position().x, 500.0 + point.x);
            QCOMPARE(arrow.position().y, 300.0 + point.y);
        }
    }

    void archerArrowsDoNotStartInTheCenterOfTheTower() {
        Path path({{0, 0}, {2000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        ArcherTower tower({500, 300});
        const Projectile arrow = *tower.attack(*enemies[0], path);
        QVERIFY(arrow.position().x != 500.0 || arrow.position().y != 300.0);
    }

    void upgradedArcherTowerUsesTheLaunchPointsOfItsNewLevel() {
        Path path({{0, 0}, {2000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        ArcherTower tower({500, 300});
        tower.upgrade();
        QCOMPARE(int(tower.level()), 2);
        const std::span<const LaunchPoint> points = launchPoints(TowerType::Archer, 2);
        const Projectile arrow = *tower.attack(*enemies[0], path);
        QCOMPARE(arrow.position().x, 500.0 + points[0].x);
        QCOMPARE(arrow.position().y, 300.0 + points[0].y);
    }

    void everyArcherLevelHasTwoSideBySideShooters() {
        for (std::uint8_t level = 1; level <= 3; ++level) {
            const std::span<const LaunchPoint> points = launchPoints(TowerType::Archer, level);
            QCOMPARE(points.size(), std::size_t(2));
            QVERIFY(points[0].x < points[1].x);
        }
    }

    void towersWithoutLaunchPointsStillShootFromTheirCenter() {
        Path path({{0, 0}, {2000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        QVERIFY(launchPoints(TowerType::Mage, 1).empty());
        MageTower tower({500, 300});
        const Projectile bolt = *tower.attack(*enemies[0], path);
        QCOMPARE(bolt.position().x, 500.0);
        QCOMPARE(bolt.position().y, 300.0);
    }

    void damageOnlyOnImpact() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile projectile = arrowAt(*enemies[0], path);
        QVERIFY(!projectile.update(projectile.duration() / 2, enemies, path));
        QCOMPARE(enemies[0]->health(), 100.0);
        QVERIFY(projectile.update(projectile.duration(), enemies, path));
        QCOMPARE(enemies[0]->health(), 55.0);          // 50 schade, 10% armor
    }

    void speedsAreHalvedAndDiffer() {
        QCOMPARE(flightProfile(TowerType::Archer).speed, 325.0);
        QCOMPARE(flightProfile(TowerType::Mage).speed, 750.0);
        QCOMPARE(flightProfile(TowerType::Cannon).speed, 190.0);
        QCOMPARE(flightProfile(TowerType::Artillery).speed, 190.0);
        QVERIFY(flightProfile(TowerType::Mage).speed > flightProfile(TowerType::Archer).speed);
        QVERIFY(flightProfile(TowerType::Archer).speed > flightProfile(TowerType::Cannon).speed);
    }

    void mageFliesFasterThanCannonOverSameDistance() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        const Projectile mage = arrowAt(*enemies[0], path, 0.0, TowerType::Mage);
        const Projectile cannon = arrowAt(*enemies[0], path, 0.0, TowerType::Cannon);
        QVERIFY(mage.duration() < cannon.duration());
    }

    void rocketTakesLongerBecauseOfArc() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        const Projectile rocket = arrowAt(*enemies[0], path, 0.0, TowerType::Artillery);
        const Projectile cannon = arrowAt(*enemies[0], path, 0.0, TowerType::Cannon);
        QVERIFY(rocket.duration() > cannon.duration());   // zelfde snelheid, maar de boog is langer
    }

    void straightProjectileStaysOnTheLine() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));            // doel op (300, 0), kogel start op (0, 100)
        Projectile mage = arrowAt(*enemies[0], path, 0.0, TowerType::Mage);
        mage.update(mage.duration() / 2, enemies, path);
        QCOMPARE(mage.position().x, 150.0);
        QCOMPARE(mage.position().y, 50.0);
    }

    void arcedProjectileFliesAboveTheLine() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile rocket = arrowAt(*enemies[0], path, 0.0, TowerType::Artillery);
        rocket.update(rocket.duration() / 2, enemies, path);
        QVERIFY(rocket.position().y < 50.0);            // lijn zit op y = 50, de boog erboven
        QVERIFY(rocket.angleDegrees() > -90.0 && rocket.angleDegrees() < 90.0);
    }

    void followsMovingTargetAndAlwaysHits() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile projectile = arrowAt(*enemies[0], path);
        bool landed = false;
        for (int i = 0; i < 1000 && !landed; ++i) {
            enemies[0]->update(0.01);
            landed = projectile.update(0.01, enemies, path);
        }
        QVERIFY(landed);
        QVERIFY(enemies[0]->health() < 100.0);
    }

    void splashHitsEnemiesInRadiusAroundImpactPoint() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(3));
        enemies.push_back(skeletonAfter(8));
        enemies.push_back(skeletonAfter(9));
        Projectile rocket = arrowAt(*enemies[2], path, 60.0, TowerType::Artillery);
        rocket.update(rocket.duration(), enemies, path);
        QCOMPARE(enemies[2]->health(), 55.0);
        QCOMPARE(enemies[1]->health(), 55.0);
        QCOMPARE(enemies[0]->health(), 100.0);
    }

    void deadTargetTakesNoSecondHit() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile projectile = arrowAt(*enemies[0], path);
        enemies[0]->takeDamage({1000.0, DamageType::Magic});
        QVERIFY(projectile.update(projectile.duration(), enemies, path));
        QCOMPARE(enemies[0]->health(), 0.0);
        QVERIFY(!projectile.hasTarget());
    }

    void droppedTargetIsNeverTouched() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile projectile = arrowAt(*enemies[0], path);
        projectile.dropTarget(enemies[0].get());
        enemies.clear();                                // de vijand bestaat niet meer
        QVERIFY(projectile.update(projectile.duration(), enemies, path));   // geen crash
    }

    void droppedSplashProjectileStillExplodes() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        enemies.push_back(skeletonAfter(5));
        Projectile rocket = arrowAt(*enemies[0], path, 60.0, TowerType::Artillery);
        rocket.dropTarget(enemies[0].get());
        QVERIFY(rocket.update(rocket.duration(), enemies, path));
        QCOMPARE(enemies[1]->health(), 55.0);
    }

    void levelIsClampedToSpriteStages() {
        Path path({{0, 0}, {1000, 0}});
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(skeletonAfter(5));
        Projectile low(TowerType::Archer, 0, {0, 0}, *enemies[0], path, {1.0, DamageType::Magic});
        Projectile high(TowerType::Archer, 9, {0, 0}, *enemies[0], path, {1.0, DamageType::Magic});
        QCOMPARE(int(low.level()), 1);
        QCOMPARE(int(high.level()), 3);
    }

    // ---- in het spel
    void gameKeepsProjectileInTheAirBeforeDamage() {
        Game game(Path({{0, 0}, {2000, 0}}), Player(0, 5), WaveManager(std::vector<Wave>{[] {
                      Wave wave; wave.enemies = {EnemyType::Skeleton}; return wave; }()}, 0.0));
        game.addTower(std::make_unique<MageTower>(Vec2{50, 100}));
        game.update(0.01);
        QCOMPARE(game.projectiles().size(), std::size_t(1));
        QCOMPARE(game.enemies()[0]->health(), 100.0);
        QCOMPARE(game.projectiles()[0].id(), std::uint64_t(1));
        for (int i = 0; i < 100 && !game.projectiles().empty(); ++i) {
            game.update(0.01);
        }
        QVERIFY(game.projectiles().empty());
        QVERIFY(game.enemies()[0]->health() < 100.0);
    }

    void gameDropsProjectileTargetsWhenEnemyLeaves() {
        // Pad van 100 px: de vijand bereikt het einde terwijl er een langzame kogel naar hem toe vliegt.
        Game game(Path({{0, 0}, {100, 0}}), Player(0, 5), WaveManager(std::vector<Wave>{[] {
                      Wave wave; wave.enemies = {EnemyType::Skeleton}; return wave; }()}, 0.0));
        game.addTower(std::make_unique<ArtilleryTower>(Vec2{50, 100}));
        for (int i = 0; i < 400; ++i) {
            game.update(0.05);                          // mag niet crashen
        }
        QVERIFY(game.enemies().empty());
        QVERIFY(game.projectiles().empty());
    }
};

QTEST_APPLESS_MAIN(TestProjectile)
#include "test_projectile.moc"
