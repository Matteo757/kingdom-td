#include <QtTest>
#include <memory>
#include <string>
#include "ktd/ArcherTower.h"
#include "ktd/BarracksTower.h"
#include "ktd/Game.h"
#include "ktd/Prices.h"
#include "ktd/SlotMenu.h"

using namespace ktd;

namespace {

Game makeGame(int gold, double pathLength = 5000.0)
{
    return Game(Path({{0, 0}, {pathLength, 0}}), Player(gold, 5),
                WaveManager(std::vector<Wave>{}), {{500, 50}, {300, 50}, {700, 50}});
}

// Bouwt een toren en upgradet hem tot het gevraagde level.
void buildAtLevel(Game& game, std::size_t slot, TowerType type, int level)
{
    QVERIFY(game.buildTower(slot, type));
    while (game.buildSlots()[slot].tower->level() < level) {
        QVERIFY(game.upgradeTower(slot));
    }
}

}

// Vereiste 53: 53. test-driven development (= written test plan or unit tests)
// Testplan verkopen: (1) de verkoopprijs is 60% van alles wat in een toren gestoken is, naar beneden afgerond,
// (2) die investering bevat de bouwprijs en elke upgrade, (3) het bedrag in het menu is altijd het bedrag dat
// uitbetaald wordt, voor elk torentype en elk level, (4) een toren is maar één keer te verkopen,
// (5) na de verkoop is de bouwplek leeg en gaat het spel gewoon door.
class TestSell : public QObject {
    Q_OBJECT

private slots:
    // ---- de formule ----

    void refundIsSixtyPercent() {
        QCOMPARE(sellRefund(100), 60);
        QCOMPARE(sellRefund(150), 90);
        QCOMPARE(sellRefund(200), 120);
        QCOMPARE(sellRefund(250), 150);
        QCOMPARE(sellRefund(300), 180);
        QCOMPARE(sellRefund(175), 105);   // 100 bouwen + 75 upgrade
    }

    void refundRoundsDown() {
        QCOMPARE(sellRefund(99), 59);     // 59,4
        QCOMPARE(sellRefund(1), 0);
        QCOMPARE(sellRefund(83), 49);     // 49,8
        QCOMPARE(sellRefund(0), 0);
        QCOMPARE(sellRefund(-10), 0);
    }

    // ---- de investering per toren ----

    void investmentStartsAtTheBuildCost() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Mage));
        QCOMPARE(game.buildSlots()[0].tower->invested(), buildCost(TowerType::Mage));
    }

    void investmentIncludesEveryUpgrade() {
        Game game = makeGame(1000);
        buildAtLevel(game, 0, TowerType::Archer, 3);
        QCOMPARE(game.buildSlots()[0].tower->invested(), buildCost(TowerType::Archer) + upgradeCost(1) + upgradeCost(2));
    }

    void failedBuildOrUpgradeAddsNoInvestment() {
        Game game = makeGame(buildCost(TowerType::Archer));   // precies genoeg om te bouwen, niet om te upgraden
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(!game.upgradeTower(0));
        QCOMPARE(game.buildSlots()[0].tower->invested(), buildCost(TowerType::Archer));
        QVERIFY(!game.buildTower(1, TowerType::Artillery));
        QCOMPARE(game.towers().size(), std::size_t(1));
    }

    void everyTowerHasItsOwnInvestment() {
        Game game = makeGame(2000);
        buildAtLevel(game, 0, TowerType::Archer, 3);
        QVERIFY(game.buildTower(1, TowerType::Cannon));
        QCOMPARE(game.buildSlots()[0].tower->invested(), 70 + 90 + 140);
        QCOMPARE(game.buildSlots()[1].tower->invested(), 120);
    }

    // ---- uitbetalen: elk type op elk level ----

    void sellingPaysTheExpectedRefundForEveryTowerAndLevel() {
        struct Row { TowerType type; int refund[3]; };
        // bouwprijs, +90 voor level 2, +140 voor level 3, daarvan 60% naar beneden afgerond
        const Row table[] = {
            {TowerType::Archer,    {42,  96, 180}},   //  70 /  160 /  300
            {TowerType::Cannon,    {72, 126, 210}},   // 120 /  210 /  350
            {TowerType::Mage,      {60, 114, 198}},   // 100 /  190 /  330
            {TowerType::Barracks,  {48, 102, 186}},   //  80 /  170 /  310
            {TowerType::Artillery, {90, 144, 228}},   // 150 /  240 /  380
        };
        for (const Row& row : table) {
            for (int level = 1; level <= 3; ++level) {
                Game game = makeGame(5000);
                buildAtLevel(game, 0, row.type, level);
                const int before = game.player().gold();
                const int expected = row.refund[level - 1];

                QCOMPARE(game.sellValue(0), expected);
                QVERIFY(game.sellTower(0));
                QCOMPARE(game.player().gold(), before + expected);
            }
        }
    }

    void exampleFromTheSpecificationWorksOutLikeThis() {
        // Een toren met 100 goud bouwen en daarna 75 en 125 aan upgrades: 100 -> 60, 175 -> 105, 300 -> 180.
        QCOMPARE(sellRefund(100), 60);
        QCOMPARE(sellRefund(100 + 75), 105);
        QCOMPARE(sellRefund(100 + 75 + 125), 180);
    }

    // ---- het getoonde bedrag is het uitbetaalde bedrag ----

    void menuShowsExactlyWhatIsPaidOutAtEveryLevel() {
        for (const TowerType type : kBuildableTowers) {
            for (int level = 1; level <= 3; ++level) {
                Game game = makeGame(5000);
                buildAtLevel(game, 0, type, level);

                const std::vector<MenuEntry> entries = slotMenuEntries(game, 0);
                QVERIFY(!entries.empty());
                const MenuEntry& sell = entries.back();
                QVERIFY(sell.action == MenuAction::Sell);
                QVERIFY(sell.enabled);
                QCOMPARE(sell.text, "Verkopen (+" + std::to_string(sell.price) + " goud)");

                const int before = game.player().gold();
                QVERIFY(game.sellTower(0));
                QCOMPARE(game.player().gold() - before, sell.price);   // getoond == ontvangen
            }
        }
    }

    void menuAmountUpdatesAfterAnUpgrade() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QCOMPARE(slotMenuEntries(game, 0).back().price, 42);
        QVERIFY(game.upgradeTower(0));
        QCOMPARE(slotMenuEntries(game, 0).back().price, 96);
        QVERIFY(game.upgradeTower(0));
        QCOMPARE(slotMenuEntries(game, 0).back().price, 180);
    }

    void sellEntryIsOnlyShownForABuiltSlot() {
        Game game = makeGame(1000);
        for (const MenuEntry& entry : slotMenuEntries(game, 0)) {
            QVERIFY(entry.action != MenuAction::Sell);
        }
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(slotMenuEntries(game, 0).back().action == MenuAction::Sell);
        for (const MenuEntry& entry : slotMenuEntries(game, 1)) {   // een andere, lege plek
            QVERIFY(entry.action != MenuAction::Sell);
        }
    }

    void sellEntryStaysEnabledWhenThePlayerHasNoGold() {
        Game game = makeGame(70);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QCOMPARE(game.player().gold(), 0);
        QVERIFY(slotMenuEntries(game, 0).back().enabled);   // verkopen kost niets
    }

    // ---- de toren verdwijnt ----

    void sellingRemovesTheTowerAndFreesTheSlot() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(game.sellTower(0));
        QVERIFY(game.towers().empty());
        QVERIFY(game.buildSlots()[0].tower == nullptr);
        QVERIFY(game.canBuild(0, TowerType::Mage));          // de plek is weer bouwbaar
        QVERIFY(game.buildTower(0, TowerType::Mage));
        QCOMPARE(game.buildSlots()[0].tower->invested(), buildCost(TowerType::Mage));   // geen oude investering
    }

    void sellingOneTowerLeavesTheOthersAlone() {
        Game game = makeGame(2000);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(game.buildTower(1, TowerType::Mage));
        QVERIFY(game.buildTower(2, TowerType::Cannon));
        const Tower* mage = game.buildSlots()[1].tower;
        const Tower* cannon = game.buildSlots()[2].tower;

        QVERIFY(game.sellTower(1));
        QCOMPARE(game.towers().size(), std::size_t(2));
        QVERIFY(game.buildSlots()[0].tower != nullptr);
        QVERIFY(game.buildSlots()[1].tower == nullptr);
        QVERIFY(game.buildSlots()[2].tower == cannon);
        QVERIFY(game.buildSlots()[2].tower != mage);
        QCOMPARE(int(game.buildSlots()[0].tower->type()), int(TowerType::Archer));
    }

    // ---- niet twee keer ----

    void aTowerCannotBeSoldTwice() {
        Game game = makeGame(1000);
        QVERIFY(game.buildTower(0, TowerType::Archer));
        QVERIFY(game.sellTower(0));
        const int goldAfterFirstSale = game.player().gold();
        QVERIFY(!game.canSell(0));
        QVERIFY(!game.sellTower(0));
        QVERIFY(!game.sellTower(0));
        QCOMPARE(game.player().gold(), goldAfterFirstSale);
        QCOMPARE(game.sellValue(0), 0);
    }

    void emptyOrInvalidSlotsCannotBeSold() {
        Game game = makeGame(1000);
        QVERIFY(!game.canSell(0));
        QVERIFY(!game.sellTower(0));
        QVERIFY(!game.sellTower(99));
        QCOMPARE(game.sellValue(99), 0);
        QCOMPARE(game.player().gold(), 1000);
    }

    void buyingAgainAfterSellingCostsFullPriceAndEarnsTheNewRefund() {
        Game game = makeGame(70);
        QVERIFY(game.buildTower(0, TowerType::Archer));    // 0 goud over
        QVERIFY(game.sellTower(0));                         // +42
        QCOMPARE(game.player().gold(), 42);
        QVERIFY(!game.canBuild(0, TowerType::Archer));      // 70 is meer dan 42: verkopen is dus nooit winst
    }

    // ---- het spel gaat gewoon door ----

    void barracksReleasesTheEnemiesItWasHoldingWhenSold() {
        Path path({{0, 0}, {2000, 0}});
        BarracksTower barracks({500, 50}, path);
        std::vector<std::unique_ptr<Enemy>> enemies;
        enemies.push_back(makeEnemy(EnemyType::Skeleton));
        barracks.attack(*enemies[0], path);        // een soldaat neemt de vijand aan en blokkeert hem
        QVERIFY(enemies[0]->isBlocked());

        barracks.onSold(enemies);
        QVERIFY(!enemies[0]->isBlocked());         // anders bleef hij voor altijd staan
    }

    void barracksDoesNotTouchEnemiesThatAreAlreadyGone() {
        Path path({{0, 0}, {2000, 0}});
        BarracksTower barracks({500, 50}, path);
        std::unique_ptr<Enemy> gone = makeEnemy(EnemyType::Skeleton);
        barracks.attack(*gone, path);
        QVERIFY(gone->isBlocked());

        const std::vector<std::unique_ptr<Enemy>> noEnemies;   // de vijand staat niet meer in de lijst van Game
        barracks.onSold(noEnemies);                             // mag hem dus niet aanraken
        QVERIFY(gone->isBlocked());
    }

    void sellingATowerDoesNotStopEnemiesOrBreakFlyingProjectiles() {
        Wave wave;
        wave.enemies = {EnemyType::Skeleton};
        Game game(Path({{0, 0}, {5000, 0}}), Player(1000, 5), WaveManager(std::vector<Wave>{wave}, 0.0),
                  {{200, 50}});
        QVERIFY(game.buildTower(0, TowerType::Archer));

        for (int i = 0; i < 200 && game.projectiles().empty(); ++i) {
            game.update(0.05);
        }
        QVERIFY(!game.projectiles().empty());         // er vliegt een pijl
        QVERIFY(!game.enemies().empty());
        const double before = game.enemies()[0]->distance();

        QVERIFY(game.sellTower(0));
        QVERIFY(game.towers().empty());
        for (int i = 0; i < 40; ++i) {
            game.update(0.05);                         // geen crash, de pijl landt gewoon
        }
        QVERIFY(game.projectiles().empty());
        if (!game.enemies().empty()) {
            QVERIFY(game.enemies()[0]->distance() > before);   // de vijand loopt gewoon door
        }
    }
};

QTEST_APPLESS_MAIN(TestSell)
#include "test_sell.moc"
