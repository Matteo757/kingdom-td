#include <QtTest>
#include <cmath>
#include <stdexcept>
#include "ktd/Game.h"

using namespace ktd;

namespace {

constexpr double kSceneWidth = 1024.0;
constexpr double kSceneHeight = 576.0;
constexpr double kMaxTowerRange = 150.0;
constexpr int kLevels[] = {1, 3};

bool insideScene(const Vec2& p)
{
    return p.x >= 0.0 && p.x <= kSceneWidth && p.y >= 0.0 && p.y <= kSceneHeight;
}

double distanceToRoute(const Path& path, const Vec2& point)
{
    const Vec2 closest = path.pointAt(path.closestDistance(point));
    return std::hypot(closest.x - point.x, closest.y - point.y);
}

}

class TestLevel : public QObject {
    Q_OBJECT

private slots:
    void unknownLevelThrows() {
        QVERIFY_EXCEPTION_THROWN(Game::level(2), std::invalid_argument);
    }

    void defaultLevelIsLevelOne() {
        const Game defaultGame = Game::defaultLevel();
        const Game levelOne = Game::level(1);
        QCOMPARE(defaultGame.path().length(), levelOne.path().length());
    }

    void levelOneRunsFromLeftToTheCastleOnTheRight() {
        const Game game = Game::level(1);
        const std::vector<Vec2>& waypoints = game.path().waypoints();
        QVERIFY(waypoints.front().x < 10.0);
        QVERIFY(waypoints.back().x > 900.0);
    }

    void routeStaysInsideScene() {
        for (const int number : kLevels) {
            const Game game = Game::level(number);
            for (const Vec2& p : game.path().waypoints()) {
                QVERIFY(insideScene(p));
            }
        }
    }

    void routeHasEnoughLength() {
        for (const int number : kLevels) {
            const Game game = Game::level(number);
            QVERIFY(game.path().length() > 1000.0);
        }
    }

    void buildSlotsStayInsideScene() {
        for (const int number : kLevels) {
            const Game game = Game::level(number);
            for (const BuildSlot& buildSlot : game.buildSlots()) {
                QVERIFY(insideScene(buildSlot.position));
            }
        }
    }

    void buildSlotsDoNotOverlap() {
        for (const int number : kLevels) {
            const Game game = Game::level(number);
            const std::vector<BuildSlot>& allSlots = game.buildSlots();
            for (std::size_t i = 0; i < allSlots.size(); ++i) {
                for (std::size_t j = i + 1; j < allSlots.size(); ++j) {
                    const Vec2 d = allSlots[i].position - allSlots[j].position;
                    QVERIFY(std::hypot(d.x, d.y) > 60.0);
                }
            }
        }
    }

    void mostBuildSlotsAreWithinTowerRangeOfTheRoute() {
        for (const int number : kLevels) {
            const Game game = Game::level(number);
            const std::vector<BuildSlot>& allSlots = game.buildSlots();
            std::size_t reachable = 0;
            for (const BuildSlot& buildSlot : allSlots) {
                if (distanceToRoute(game.path(), buildSlot.position) <= kMaxTowerRange) {
                    ++reachable;
                }
            }
            QVERIFY(reachable + 2 >= allSlots.size());
        }
    }
};

QTEST_APPLESS_MAIN(TestLevel)
#include "test_level.moc"
