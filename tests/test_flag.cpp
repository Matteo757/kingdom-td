#include <QtTest>
#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include "ktd/BarracksTower.h"
#include "ktd/Game.h"
#include "ktd/Skeleton.h"
#include "ktd/Soldier.h"

using namespace ktd;

// Vereiste 53: 53. test-driven development (= written test plan or unit tests)
// Testplan voor de vlag van de Barracks:
//  (1) plaatsing: alleen op de weg; geweigerd op gras, net naast de weg, buiten de kaart en op een bouwplek;
//  (2) de vlag verplaatsen kan altijd; een ongeldige plek laat de oude vlag staan; elke Barracks heeft een eigen vlag;
//  (3) soldaten lopen met een vaste snelheid naar de vlag en draaien om als de vlag opnieuw verplaatst wordt;
//  (4) soldaten blijven op elk moment op de weg en volgen de bochten, ook met meerdere soldaten tegelijk;
//  (5) vijanden bij de nieuwe vlag worden geblokkeerd, vechtende soldaten laten los als de vlag verplaatst;
//  (6) een nieuwe soldaat verschijnt bij de barracks en loopt dan naar de vlag.

namespace {

// Een rechte weg van links naar rechts met twee bouwplekken naast de weg en één bouwplek midden op de weg.
Game makeStraightGame()
{
    std::vector<Vec2> route = {{0.0, 100.0}, {1000.0, 100.0}};
    std::vector<Vec2> padPositions = {{500.0, 50.0}, {300.0, 50.0}, {800.0, 100.0}};
    return Game(Path(route), Player(1000, 20), WaveManager({}, 3.0, 6.0), padPositions);
}

BarracksTower* barracksAt(Game& game, std::size_t slot)
{
    return dynamic_cast<BarracksTower*>(game.buildSlots()[slot].tower);
}

void run(BarracksTower& tower, std::vector<std::unique_ptr<Enemy>>& enemies, const Path& path, double seconds)
{
    constexpr double kStep = 0.05;
    for (double t = 0.0; t < seconds - 1e-9; t += kStep) {
        tower.update(kStep, enemies, path);
    }
}

// Een vijand die vrijwel onverwoestbaar is en een soldaat in één stap verslaat.
class Brute : public Enemy {
public:
    Brute() : Enemy(1e9, 0, 0) {}
    double speed() const override { return 100.0; }
    std::string name() const override { return "Brute"; }
    int reward() const override { return 0; }
    double attackPower() const override { return 1000.0; }
};

bool near(double a, double b, double tolerance = 1e-6)
{
    return std::abs(a - b) <= tolerance;
}

// Eenheidsvector loodrecht op de weg op afstand d van het begin.
Vec2 normalAt(const Path& path, double d)
{
    const Vec2 ahead = path.pointAt(d + 1.0);
    const Vec2 behind = path.pointAt(d - 1.0);
    const Vec2 direction{ahead.x - behind.x, ahead.y - behind.y};
    const double length = std::hypot(direction.x, direction.y);
    return {-direction.y / length, direction.x / length};
}

}

class TestFlag : public QObject {
    Q_OBJECT

private slots:
    // ---- (1) plaatsing: alleen op de weg ----

    void flagCanBePlacedOnTheRoadAlongTheWholeRoute() {
        for (const int level : {1, 3}) {
            const Game game = Game::level(level);
            for (double d = 30.0; d <= game.path().length() - 30.0; d += 25.0) {
                QVERIFY(game.canPlaceFlag(game.path().pointAt(d)));
            }
        }
    }

    void flagCanBePlacedAtTheEdgeOfTheRoad() {
        const Game game = Game::level(1);
        const Vec2 center = game.path().pointAt(300.0);
        const Vec2 side = normalAt(game.path(), 300.0);
        QVERIFY(game.canPlaceFlag(center + side * (kRoadHalfWidth - 2.0)));
        QVERIFY(game.canPlaceFlag(center + side * (-(kRoadHalfWidth - 2.0))));
    }

    void flagIsRejectedOnGrass() {
        const Game game = Game::level(1);
        const Vec2 center = game.path().pointAt(300.0);
        const Vec2 side = normalAt(game.path(), 300.0);
        QVERIFY(!game.canPlaceFlag(center + side * 60.0));
        QVERIFY(!game.canPlaceFlag(center + side * (-60.0)));
        QVERIFY(!game.canPlaceFlag({500.0, 520.0}));   // bos onderaan de kaart
    }

    void flagIsRejectedJustOutsideTheRoad() {
        const Game game = Game::level(1);
        const Vec2 center = game.path().pointAt(300.0);
        const Vec2 side = normalAt(game.path(), 300.0);
        QVERIFY(!game.canPlaceFlag(center + side * (kRoadHalfWidth + 3.0)));
    }

    void flagIsRejectedOutsideTheMap() {
        const Game game = Game::level(1);
        QVERIFY(!game.canPlaceFlag({-5.0, 239.0}));      // de weg begint op de rand: net daarbuiten mag niet
        QVERIFY(!game.canPlaceFlag({1030.0, 380.0}));
        QVERIFY(!game.canPlaceFlag({500.0, -10.0}));
        QVERIFY(!game.canPlaceFlag({500.0, 600.0}));
        QVERIFY(!game.canPlaceFlag({0.0, 239.0}));       // precies op de rand telt ook niet: de vlag zou half buiten beeld staan
    }

    void flagIsRejectedOnEveryBuildPad() {
        for (const int level : {1, 3}) {
            const Game game = Game::level(level);
            for (const BuildSlot& buildSlot : game.buildSlots()) {
                QVERIFY(!game.canPlaceFlag(buildSlot.position));
            }
        }
    }

    void flagIsRejectedOnAPadThatLiesOnTheRoad() {
        const Game game = makeStraightGame();   // bouwplek 2 ligt midden op de weg
        QVERIFY(!game.canPlaceFlag({800.0, 100.0}));
        QVERIFY(!game.canPlaceFlag({800.0 + kPadClearance - 1.0, 100.0}));
        QVERIFY(game.canPlaceFlag({800.0 + kPadClearance + 1.0, 100.0}));
        QVERIFY(game.canPlaceFlag({800.0 - kPadClearance - 1.0, 100.0}));
    }

    // ---- (2) de vlag plaatsen en verplaatsen ----

    void flagStartsOnTheRoadNextToTheBarracks() {
        Game game = makeStraightGame();
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        const std::optional<Vec2> flag = game.flagPosition(0);
        QVERIFY(flag.has_value());
        QVERIFY(near(flag->x, 500.0) && near(flag->y, 100.0));
    }

    void flagCanOnlyBePlacedForABarracks() {
        Game game = makeStraightGame();
        QVERIFY(!game.placeFlag(0, {700.0, 100.0}));          // lege bouwplek
        QVERIFY(!game.flagPosition(0).has_value());
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(!game.placeFlag(0, {700.0, 100.0}));          // boogschutterstoren heeft geen soldaten
        QVERIFY(!game.flagPosition(0).has_value());
        QVERIFY(!game.placeFlag(99, {700.0, 100.0}));         // bouwplek bestaat niet
    }

    void flagCanBeMovedWheneverThePlayerWants() {
        Game game = makeStraightGame();
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        QVERIFY(game.placeFlag(0, {700.0, 100.0}));
        QVERIFY(near(game.flagPosition(0)->x, 700.0));
        QVERIFY(game.placeFlag(0, {200.0, 100.0}));
        QVERIFY(near(game.flagPosition(0)->x, 200.0));
        QVERIFY(game.placeFlag(0, {900.0, 112.0}));
        QVERIFY(near(game.flagPosition(0)->x, 900.0) && near(game.flagPosition(0)->y, 112.0));
    }

    void invalidPlacementKeepsTheOldFlag() {
        Game game = makeStraightGame();
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        QVERIFY(game.placeFlag(0, {700.0, 100.0}));
        QVERIFY(!game.placeFlag(0, {700.0, 300.0}));          // gras
        QVERIFY(!game.placeFlag(0, {-20.0, 100.0}));          // buiten de kaart
        QVERIFY(!game.placeFlag(0, {800.0, 100.0}));          // bouwplek
        QVERIFY(near(game.flagPosition(0)->x, 700.0) && near(game.flagPosition(0)->y, 100.0));
    }

    void barracksItselfRefusesAFlagOnTheGrass() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        QVERIFY(!tower.setFlag({500.0, 200.0}));
        QVERIFY(near(tower.flagDistance(), 500.0));
        QVERIFY(tower.setFlag({600.0, 110.0}));
        QVERIFY(near(tower.flagDistance(), 600.0));
    }

    void everyBarracksHasItsOwnFlag() {
        Game game = makeStraightGame();
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        QVERIFY(game.buildTower(1, TowerType::Barracks));
        QVERIFY(game.placeFlag(0, {700.0, 100.0}));
        QVERIFY(near(game.flagPosition(0)->x, 700.0));
        QVERIFY(near(game.flagPosition(1)->x, 300.0));        // de tweede is niet meeverplaatst
        QVERIFY(game.placeFlag(1, {100.0, 100.0}));
        QVERIFY(near(game.flagPosition(0)->x, 700.0));
        QVERIFY(near(game.flagPosition(1)->x, 100.0));
    }

    // ---- (3) soldaten lopen naar de vlag ----

    void soldierWalksAtItsSpeedWithoutOvershooting() {
        Soldier soldier(40.0, 10.0, 1.0, 8.0);
        soldier.place(100.0);
        soldier.walkTo(500.0, 0.5);
        QVERIFY(near(soldier.distance(), 100.0 + Soldier::kSpeed * 0.5));
        soldier.walkTo(110.0, 10.0);                           // heel grote stap: stopt op het doel
        QVERIFY(near(soldier.distance(), 110.0));
        soldier.walkTo(50.0, 0.1);                             // ook achteruit langs de weg
        QVERIFY(near(soldier.distance(), 110.0 - Soldier::kSpeed * 0.1));
    }

    void deadSoldierDoesNotWalk() {
        Soldier soldier(40.0, 10.0, 1.0, 8.0);
        soldier.place(100.0);
        soldier.takeDamage(100.0);
        soldier.walkTo(500.0, 1.0);
        QVERIFY(near(soldier.distance(), 100.0));
    }

    void newSoldiersStandAtTheFlag() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        const BarracksTower tower({500.0, 50.0}, path);
        const std::vector<Vec2> positions = tower.unitPositions();
        QCOMPARE(positions.size(), BarracksTower::kSoldierCount);
        for (const Vec2& position : positions) {
            QVERIFY(std::abs(position.x - 500.0) <= 20.0);
        }
    }

    void soldiersWalkToTheNewFlagAndArrive() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        QVERIFY(tower.setFlag({800.0, 100.0}));

        run(tower, enemies, path, 0.5);                       // onderweg: nog niet aangekomen
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(soldier.distance() > 500.0 && soldier.distance() < 700.0);
        }
        run(tower, enemies, path, 5.0);                       // genoeg tijd om 300 px te lopen
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(std::abs(soldier.distance() - 800.0) <= 16.0 + 1e-9);
        }
    }

    void soldiersNeverMoveFasterThanTheirSpeed() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        tower.setFlag({900.0, 100.0});
        std::vector<double> before;
        for (const Soldier& soldier : tower.soldiers()) {
            before.push_back(soldier.distance());
        }
        for (int i = 0; i < 100; ++i) {
            tower.update(0.05, enemies, path);
            for (std::size_t s = 0; s < tower.soldiers().size(); ++s) {
                const double now = tower.soldiers()[s].distance();
                QVERIFY(std::abs(now - before[s]) <= Soldier::kSpeed * 0.05 + 1e-9);
                before[s] = now;
            }
        }
    }

    void soldiersTurnAroundWhenTheFlagMovesAgain() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        tower.setFlag({900.0, 100.0});
        run(tower, enemies, path, 1.0);                       // 90 px naar rechts
        const double midway = tower.soldiers()[1].distance();
        QVERIFY(midway > 560.0);
        tower.setFlag({100.0, 100.0});
        run(tower, enemies, path, 0.5);
        QVERIFY(tower.soldiers()[1].distance() < midway);     // nu terug naar links
        run(tower, enemies, path, 8.0);
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(std::abs(soldier.distance() - 100.0) <= 16.0 + 1e-9);
        }
    }

    // ---- (4) soldaten blijven op de weg en volgen de bochten ----

    void soldiersFollowTheRoadAroundACorner() {
        const Path path({{0.0, 0.0}, {500.0, 0.0}, {500.0, 500.0}});
        const RouteBounds bounds(path);
        BarracksTower tower({250.0, -40.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        QVERIFY(tower.setFlag({500.0, 400.0}));               // achter de hoek

        bool passedTheCorner = false;
        for (int i = 0; i < 400; ++i) {
            tower.update(0.05, enemies, path);
            for (const Vec2& position : tower.unitPositions()) {
                QVERIFY(bounds.contains(position));           // nooit dwars door het gras
            }
            for (const Soldier& soldier : tower.soldiers()) {
                if (soldier.distance() > 520.0) {
                    passedTheCorner = true;
                }
            }
        }
        QVERIFY(passedTheCorner);
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(std::abs(soldier.distance() - 900.0) <= 16.0 + 1e-9);   // vlag op 500 + 400 langs de weg
        }
    }

    void soldiersStayOnTheRoadAtAllTimesOnTheRealLevels() {
        for (const int level : {1, 3}) {
            for (std::size_t slotIndex = 0; slotIndex < 3; ++slotIndex) {
                Game game = Game::level(level);
                const RouteBounds bounds(game.path());
                QVERIFY(game.buildTower(slotIndex, TowerType::Barracks));
                BarracksTower* barracks = barracksAt(game, slotIndex);
                QVERIFY(barracks != nullptr);

                // Een vlag ver weg op de weg: de soldaten moeten alle bochten daarheen volgen.
                for (double fraction = 0.9; fraction > 0.1; fraction -= 0.05) {
                    const Vec2 flag = game.path().pointAt(game.path().length() * fraction);
                    if (game.placeFlag(slotIndex, flag)) {
                        break;
                    }
                }
                std::vector<std::unique_ptr<Enemy>> none;
                for (int i = 0; i < 1500; ++i) {              // 75 seconden
                    barracks->update(0.05, none, game.path());
                    for (const Vec2& position : barracks->unitPositions()) {
                        QVERIFY(bounds.contains(position));
                    }
                }
            }
        }
    }

    void severalSoldiersWorkAtTheSameTimeAndDoNotStack() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        tower.setFlag({800.0, 100.0});
        run(tower, enemies, path, 6.0);
        const std::vector<Vec2> positions = tower.unitPositions();
        QCOMPARE(positions.size(), BarracksTower::kSoldierCount);
        for (std::size_t a = 0; a < positions.size(); ++a) {
            QVERIFY(std::abs(positions[a].x - 800.0) <= 24.0);   // alle drie bij de nieuwe vlag
            for (std::size_t b = a + 1; b < positions.size(); ++b) {
                QVERIFY(std::hypot(positions[a].x - positions[b].x, positions[a].y - positions[b].y) > 6.0);
            }
        }
    }

    void gameUpdateMovesTheSoldiersToo() {
        Game game = makeStraightGame();
        QVERIFY(game.buildTower(0, TowerType::Barracks));
        QVERIFY(game.placeFlag(0, {700.0, 100.0}));
        for (int i = 0; i < 100; ++i) {
            game.update(0.05);                                // vijf seconden
        }
        for (const Soldier& soldier : barracksAt(game, 0)->soldiers()) {
            QVERIFY(std::abs(soldier.distance() - 700.0) <= 16.0 + 1e-9);
        }
    }

    // ---- (5) vijanden en gevechten ----

    void enemiesAtTheNewFlagGetBlockedAndAtTheOldFlagNot() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        QVERIFY(tower.setFlag({800.0, 100.0}));
        std::vector<std::unique_ptr<Enemy>> enemies;
        auto atOldFlag = std::make_unique<Skeleton>();
        atOldFlag->update(8.0);                               // afstand 480
        auto atNewFlag = std::make_unique<Skeleton>();
        atNewFlag->update(13.0);                              // afstand 780
        enemies.push_back(std::move(atOldFlag));
        enemies.push_back(std::move(atNewFlag));

        tower.update(0.0, enemies, path);
        QVERIFY(!enemies[0]->isBlocked());
        QVERIFY(enemies[1]->isBlocked());
    }

    void fightingSoldiersLetGoWhenTheFlagMoves() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        auto skeleton = std::make_unique<Skeleton>();
        skeleton->update(8.0);                                // afstand 480, in reach van de vlag op 500
        enemies.push_back(std::move(skeleton));
        tower.update(0.0, enemies, path);
        QVERIFY(enemies[0]->isBlocked());

        QVERIFY(tower.setFlag({900.0, 100.0}));
        tower.update(0.016, enemies, path);
        QVERIFY(!enemies[0]->isBlocked());                    // de vijand loopt weer door
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(soldier.target() == nullptr);             // en de soldaten gaan naar de nieuwe vlag
        }
    }

    void fightingSoldierWalksTowardsItsEnemy() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        auto skeleton = std::make_unique<Skeleton>();
        skeleton->update(8.7);                                // afstand 522
        enemies.push_back(std::move(skeleton));
        run(tower, enemies, path, 1.0);
        const Soldier* fighter = nullptr;
        for (const Soldier& soldier : tower.soldiers()) {
            if (soldier.target() != nullptr) {
                fighter = &soldier;
            }
        }
        QVERIFY(fighter != nullptr);
        QVERIFY(std::abs(fighter->distance() - enemies[0]->distance()) <= 20.0);
    }

    // ---- (6) een nieuwe soldaat verschijnt bij de barracks ----

    void respawnedSoldierAppearsAtTheBarracksAndWalksToTheFlag() {
        const Path path({{0.0, 100.0}, {1000.0, 100.0}});
        BarracksTower tower({500.0, 50.0}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        auto brute = std::make_unique<Brute>();
        brute->update(5.0);                                   // afstand 500: bij de vlag
        enemies.push_back(std::move(brute));
        tower.update(0.1, enemies, path);                     // één soldaat vecht en sneuvelt meteen
        enemies.clear();
        QCOMPARE(tower.unitPositions().size(), BarracksTower::kSoldierCount - 1);

        QVERIFY(tower.setFlag({900.0, 100.0}));
        std::size_t dead = 0;
        while (dead < tower.soldiers().size() && tower.soldiers()[dead].alive()) {
            ++dead;
        }
        QVERIFY(dead < tower.soldiers().size());

        bool respawned = false;
        for (int i = 0; i < 400 && !respawned; ++i) {
            tower.update(0.05, enemies, path);
            if (tower.soldiers()[dead].alive()) {
                respawned = true;
                // Net terug: hoogstens één stap van de opstelling bij de barracks (afstand 500 + plek in de rij).
                const double home = 500.0 + (static_cast<double>(dead) - 1.0) * 16.0;
                QVERIFY(std::abs(tower.soldiers()[dead].distance() - home) <= Soldier::kSpeed * 0.05 + 1e-9);
            }
        }
        QVERIFY(respawned);
        run(tower, enemies, path, 10.0);                      // daarna lopen alle drie naar de vlag
        for (const Soldier& soldier : tower.soldiers()) {
            QVERIFY(std::abs(soldier.distance() - 900.0) <= 16.0 + 1e-9);
        }
    }
};

QTEST_APPLESS_MAIN(TestFlag)
#include "test_flag.moc"
