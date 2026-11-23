#include <QtTest>
#include <algorithm>
#include <cmath>
#include <random>
#include "ktd/BarracksTower.h"
#include "ktd/Game.h"
#include "ktd/RouteBounds.h"

using namespace ktd;

namespace {

// Onafhankelijke afstandsberekening (punt tot lijnstuk), los van Path, om RouteBounds mee te vergelijken.
double distanceToSegment(const Vec2& p, const Vec2& a, const Vec2& b)
{
    const Vec2 ab = b - a;
    const double lengthSquared = ab.x * ab.x + ab.y * ab.y;
    const double t = lengthSquared == 0.0 ? 0.0 : std::clamp(((p.x - a.x) * ab.x + (p.y - a.y) * ab.y) / lengthSquared, 0.0, 1.0);
    const Vec2 d = p - (a + ab * t);
    return std::hypot(d.x, d.y);
}

double distanceToPolyline(const Vec2& p, const std::vector<Vec2>& points)
{
    double best = 1e18;
    for (std::size_t i = 1; i < points.size(); ++i) {
        best = std::min(best, distanceToSegment(p, points[i - 1], points[i]));
    }
    return best;
}

// Een rechte weg met een scherpe bocht van 90 graden.
Path cornerPath()
{
    return Path({{0, 0}, {100, 0}, {100, 100}});
}

}

class TestRouteBounds : public QObject {
    Q_OBJECT

private slots:
    void pointsOnTheRouteAreInside() {
        const Path path = cornerPath();
        const RouteBounds bounds(path);
        for (double d = 0.0; d <= path.length(); d += 1.0) {
            QVERIFY(bounds.contains(path.pointAt(d)));
        }
    }

    void pointsFarFromTheRouteAreOutside() {
        const RouteBounds bounds(cornerPath());
        QVERIFY(!bounds.contains({50, 60}));
        QVERIFY(!bounds.contains({-50, 0}));
        QVERIFY(!bounds.contains({150, 150}));
    }

    void clampPutsOutsidePointExactlyOnTheWall() {
        const RouteBounds bounds(cornerPath());
        const Vec2 clamped = bounds.clamp({50, 200});
        QVERIFY(bounds.contains(clamped));
        QVERIFY(std::abs(bounds.distanceToRoute(clamped) - kRoadHalfWidth) < 1e-6);
    }

    void clampLeavesInsidePointUnchanged() {
        const RouteBounds bounds(cornerPath());
        const Vec2 inside{50, 10};
        const Vec2 clamped = bounds.clamp(inside);
        QVERIFY(clamped.x == inside.x && clamped.y == inside.y);
    }

    void wallsAreAnOffsetOfTheRouteWithoutGapsAtCorners() {
        // contains() moet voor elk punt precies overeenkomen met "afstand tot de route <= halve breedte",
        // ook rond de hoek. Een gat op de hoek zou hier opvallen.
        const Path path = cornerPath();
        const RouteBounds bounds(path);
        std::mt19937 rng(7);
        std::uniform_real_distribution<double> x(-60.0, 160.0);
        std::uniform_real_distribution<double> y(-60.0, 160.0);
        for (int i = 0; i < 20000; ++i) {
            const Vec2 p{x(rng), y(rng)};
            const double expected = distanceToPolyline(p, path.waypoints());
            if (std::abs(expected - kRoadHalfWidth) < 1e-3) {
                continue;   // precies op de muur: afrondingsmarge
            }
            QCOMPARE(bounds.contains(p), expected <= kRoadHalfWidth);
        }
    }

    void soldierCannotWalkThroughTheWall() {
        const RouteBounds bounds(cornerPath());
        // Vanaf het midden van de weg recht naar beneden, weg van de route.
        const Vec2 end = bounds.moveWithin({50, 0}, {50, 100});
        QVERIFY(bounds.contains(end));
        QVERIFY(std::abs(end.y - kRoadHalfWidth) < 1e-6);
    }

    void soldierDoesNotCutTheCorner() {
        const Path path = cornerPath();
        const RouteBounds bounds(path);
        // Een rechte lijn van het begin naar het eind van de bocht snijdt de hoek af; elke stap moet binnen blijven.
        Vec2 position{10, 0};
        for (int i = 0; i < 400; ++i) {
            position = bounds.moveWithin(position, {100, 100});
            QVERIFY(bounds.contains(position));
        }
    }

    void segmentInsideDetectsLeavingTheRoad() {
        const RouteBounds bounds(cornerPath());
        QVERIFY(bounds.segmentInside({10, 0}, {90, 0}));
        QVERIFY(!bounds.segmentInside({10, 0}, {10, 80}));
    }

    void randomMovesNeverLeaveTheRealRoutes() {
        for (int level : {1, 3}) {
            const Game game = Game::level(level);
            const RouteBounds bounds(game.path());
            std::mt19937 rng(level);
            std::uniform_real_distribution<double> x(0.0, 1024.0);
            std::uniform_real_distribution<double> y(0.0, 576.0);
            std::uniform_real_distribution<double> along(0.0, game.path().length());
            for (int i = 0; i < 3000; ++i) {
                const Vec2 from = game.path().pointAt(along(rng));
                const Vec2 to{x(rng), y(rng)};
                QVERIFY(bounds.contains(bounds.clamp(to)));
                QVERIFY(bounds.contains(bounds.moveWithin(from, to)));
            }
        }
    }

    void barracksSoldiersStandOnTheRoadAtEverySlot() {
        for (int level : {1, 3}) {
            const Game game = Game::level(level);
            const RouteBounds bounds(game.path());
            for (const BuildSlot& slot : game.buildSlots()) {
                const BarracksTower barracks(slot.position, game.path());
                const std::vector<Vec2> positions = barracks.unitPositions();
                QCOMPARE(positions.size(), BarracksTower::kSoldierCount);
                for (const Vec2& position : positions) {
                    QVERIFY(bounds.contains(position));
                }
            }
        }
    }

    void invalidWidthIsRejected() {
        bool thrown = false;
        try {
            RouteBounds bounds(cornerPath(), 0.0);
        } catch (const std::invalid_argument&) {
            thrown = true;
        }
        QVERIFY(thrown);
    }
};

QTEST_APPLESS_MAIN(TestRouteBounds)
#include "test_routebounds.moc"