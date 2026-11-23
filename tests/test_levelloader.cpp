#include <QCoreApplication>
#include <QtTest>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include "ktd/Game.h"
#include "ktd/LevelLoader.h"

using namespace ktd;

// Vereiste 53: 53. test-driven development (= written test plan or unit tests)
// Testplan: (1) JSON-tekst verwerken: geldige en ongeldige invoer, (2) bestanden lezen, (3) een Game bouwen uit
// LevelData, (4) de loader op een aparte thread (resultaat, fouten, dubbel starten, vernietigen terwijl hij
// loopt), (5) de meegeleverde levelbestanden komen overeen met de ingebouwde levels.
// Deze tests zijn geschreven vóór LevelLoader zelf bestond.

namespace {

// Een geldig level als JSON-tekst; losse velden kunnen vervangen worden om ongeldige varianten te maken.
std::string levelJson(const std::string& gold = "300",
                      const std::string& lives = "15",
                      const std::string& waves = "8",
                      const std::string& route = "[[0, 100], [200, 100], [200, 300]]")
{
    return R"({"number": 2, "gold": )" + gold + R"(, "lives": )" + lives + R"(, "waves": )" + waves
           + R"(, "route": )" + route + R"(, "slots": [[50, 50], [150, 200]]})";
}

// Maakt een tijdelijk bestand en verwijdert het weer als het object verdwijnt.
class TempFile {
public:
    TempFile(const std::string& name, const std::string& content)
        : m_path(std::filesystem::temp_directory_path() / name)
    {
        std::ofstream file(m_path, std::ios::binary);
        file << content;
    }

    ~TempFile()
    {
        std::error_code ignored;
        std::filesystem::remove(m_path, ignored);
    }

    const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

}

class TestLevelLoader : public QObject {
    Q_OBJECT

private slots:
    // ---- JSON-tekst verwerken ----

    void parsesAValidLevel() {
        const LevelData data = parseLevel(levelJson());
        QCOMPARE(data.number, 2);
        QCOMPARE(data.startGold, 300);
        QCOMPARE(data.startLives, 15);
        QCOMPARE(data.waveCount, 8);
        QCOMPARE(data.route.size(), std::size_t(3));
        QCOMPARE(data.slotPositions.size(), std::size_t(2));
    }

    void keepsTheOrderAndValuesOfThePoints() {
        const LevelData data = parseLevel(levelJson());
        QVERIFY(data.route[0].x == 0.0 && data.route[0].y == 100.0);
        QVERIFY(data.route[2].x == 200.0 && data.route[2].y == 300.0);
        QVERIFY(data.slotPositions[1].x == 150.0 && data.slotPositions[1].y == 200.0);
    }

    void acceptsALevelWithoutBuildSlots() {
        const std::string text = R"({"number": 1, "gold": 0, "lives": 1, "waves": 1,
                                      "route": [[0, 0], [10, 10]], "slots": []})";
        QCOMPARE(parseLevel(text).slotPositions.size(), std::size_t(0));
    }

    void rejectsBrokenJson() {
        QVERIFY_EXCEPTION_THROWN(parseLevel("{ this is not json"), LevelError);
        QVERIFY_EXCEPTION_THROWN(parseLevel(""), LevelError);
    }

    void rejectsJsonThatIsNotAnObject() {
        QVERIFY_EXCEPTION_THROWN(parseLevel("[1, 2, 3]"), LevelError);
    }

    void rejectsAMissingRoute() {
        const std::string text = R"({"number": 1, "gold": 0, "lives": 1, "waves": 1, "slots": []})";
        QVERIFY_EXCEPTION_THROWN(parseLevel(text), LevelError);
    }

    void rejectsARouteWithOnlyOnePoint() {
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "15", "8", "[[0, 0]]")), LevelError);
    }

    void rejectsPointsThatAreNotPairsOfNumbers() {
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "15", "8", "[[0, 0, 5], [1, 1]]")), LevelError);
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "15", "8", "[[0, \"a\"], [1, 1]]")), LevelError);
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "15", "8", "[0, 1]")), LevelError);
    }

    void rejectsNumbersOutsideTheirRange() {
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("-1")), LevelError);     // negatief goud
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "0")), LevelError);     // geen levens
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "256")), LevelError);   // past niet in uint8_t
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "15", "0")), LevelError);   // geen waves
    }

    void rejectsValuesOfTheWrongType() {
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("\"lots\"")), LevelError);
        QVERIFY_EXCEPTION_THROWN(parseLevel(levelJson("300", "2.5")), LevelError);
    }

    // ---- bestanden lezen ----

    void readingAMissingFileFails() {
        QVERIFY_EXCEPTION_THROWN(readTextFile("this-file-does-not-exist.json"), LevelError);
    }

    void readingADirectoryFails() {
        QVERIFY_EXCEPTION_THROWN(readTextFile(std::filesystem::temp_directory_path()), LevelError);
    }

    void loadsALevelFromAFile() {
        const TempFile file("ktd_test_level_file.json", levelJson());
        const LevelData data = loadLevel(file.path());
        QCOMPARE(data.startGold, 300);
        QCOMPARE(data.route.size(), std::size_t(3));
    }

    // ---- een spel bouwen ----

    void buildsAGameFromLevelData() {
        const Game game = Game::fromLevel(parseLevel(levelJson()));
        QCOMPARE(game.player().gold(), 300);
        QCOMPARE(int(game.player().lives()), 15);
        QCOMPARE(game.waves().waveCount(), std::size_t(8));
        QCOMPARE(game.buildSlots().size(), std::size_t(2));
        QVERIFY(game.path().length() > 0.0);
    }

    void gameRejectsLevelDataWithAnInvalidRoute() {
        LevelData data;
        data.startGold = 100;
        data.startLives = 5;
        data.waveCount = 1;
        data.route = {{0.0, 0.0}};   // één punt is geen weg
        QVERIFY_EXCEPTION_THROWN(Game::fromLevel(data), std::invalid_argument);
    }

    // ---- laden op een aparte thread ----

    void loadsOnAnotherThread() {
        const TempFile file("ktd_test_level_thread.json", levelJson());
        AsyncLevelLoader loader;
        loader.start(file.path());
        QTRY_VERIFY_WITH_TIMEOUT(loader.isFinished(), 5000);

        const LevelData data = loader.result();
        QCOMPARE(data.startGold, 300);
        QVERIFY(loader.workerThreadId() != std::this_thread::get_id());
    }

    void reportsLoadingErrorsThroughResult() {
        AsyncLevelLoader loader;
        loader.start("this-file-does-not-exist.json");
        QTRY_VERIFY_WITH_TIMEOUT(loader.isFinished(), 5000);   // de thread eindigt netjes, ook bij een fout
        QVERIFY_EXCEPTION_THROWN(loader.result(), LevelError);
    }

    void resultCanBeAskedMoreThanOnce() {
        const TempFile file("ktd_test_level_twice.json", levelJson());
        AsyncLevelLoader loader;
        loader.start(file.path());
        QCOMPARE(loader.result().startGold, 300);
        QCOMPARE(loader.result().startGold, 300);
    }

    void cannotBeStartedTwice() {
        const TempFile file("ktd_test_level_start.json", levelJson());
        AsyncLevelLoader loader;
        loader.start(file.path());
        QVERIFY_EXCEPTION_THROWN(loader.start(file.path()), std::logic_error);
    }

    void resultBeforeStartThrows() {
        AsyncLevelLoader loader;
        QVERIFY(!loader.isFinished());
        QVERIFY_EXCEPTION_THROWN(loader.result(), std::logic_error);
    }

    void destroyingTheLoaderWhileItRunsIsSafe() {
        const TempFile file("ktd_test_level_destroy.json", levelJson());
        {
            AsyncLevelLoader loader;
            loader.start(file.path());
        }   // de destructor wacht op de thread; zonder join() zou het programma crashen
        QVERIFY(true);
    }

    // ---- meegeleverde levelbestanden ----

    void shippedLevelFilesMatchTheBuiltInLevels() {
        const std::filesystem::path folder(QCoreApplication::applicationDirPath().toStdWString());
        for (const int number : {1, 3}) {
            const LevelData data = loadLevel(folder / "levels" / ("level" + std::to_string(number) + ".json"));
            const Game builtIn = Game::level(number);
            const std::vector<Vec2>& waypoints = builtIn.path().waypoints();

            QCOMPARE(data.number, number);
            QCOMPARE(data.startGold, builtIn.player().gold());
            QCOMPARE(data.startLives, int(builtIn.player().lives()));
            QCOMPARE(std::size_t(data.waveCount), builtIn.waves().waveCount());

            QCOMPARE(data.route.size(), waypoints.size());
            for (std::size_t i = 0; i < waypoints.size(); ++i) {
                QVERIFY(data.route[i].x == waypoints[i].x && data.route[i].y == waypoints[i].y);
            }
            QCOMPARE(data.slotPositions.size(), builtIn.buildSlots().size());
            for (std::size_t i = 0; i < data.slotPositions.size(); ++i) {
                const Vec2& position = builtIn.buildSlots()[i].position;
                QVERIFY(data.slotPositions[i].x == position.x && data.slotPositions[i].y == position.y);
            }
        }
    }
};

QTEST_GUILESS_MAIN(TestLevelLoader)
#include "test_levelloader.moc"
