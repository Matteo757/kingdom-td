#include "ktd/LevelLoader.h"

#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>
#include <utility>
#include <nlohmann/json.hpp>

namespace ktd {
namespace {

using json = nlohmann::json;

// Leest een geheel getal uit het JSON-object en controleert of het binnen [minimum, maximum] valt.
int readInt(const json& root, const char* key, long long minimum, long long maximum)
{
    if (!root.contains(key) || !root[key].is_number_integer()) {
        throw LevelError(std::string("Missing or invalid integer: ") + key);
    }
    const long long value = root[key].get<long long>();
    if (value < minimum || value > maximum) {
        throw LevelError(std::string("Value out of range: ") + key);
    }
    return static_cast<int>(value);
}

// Leest een lijst punten [[x, y], [x, y], ...] en eist minstens minimumCount punten.
std::vector<Vec2> readPoints(const json& root, const char* key, std::size_t minimumCount)
{
    if (!root.contains(key) || !root[key].is_array()) {
        throw LevelError(std::string("Missing or invalid list of points: ") + key);
    }

    std::vector<Vec2> points;
    for (const json& entry : root[key]) {
        const bool isPoint = entry.is_array() && entry.size() == 2
                             && entry[0].is_number() && entry[1].is_number();
        if (!isPoint) {
            throw LevelError(std::string("Invalid point in ") + key);
        }
        points.push_back({entry[0].get<double>(), entry[1].get<double>()});
    }

    if (points.size() < minimumCount) {
        throw LevelError(std::string("Too few points in ") + key);
    }
    return points;
}

}  // namespace

// Vereiste 58: 58. useful (non trivial) usage of an external library (not Qt)
// nlohmann/json (third_party/nlohmann/json.hpp, MIT-licentie) leest en valideert de levelbestanden, zodat
// routes, bouwplekken en startwaarden in data staan in plaats van in de code.
LevelData parseLevel(const std::string& jsonText)
{
    json root;
    try {
        root = json::parse(jsonText);
    } catch (const json::exception& error) {
        throw LevelError(std::string("Invalid JSON: ") + error.what());
    }
    if (!root.is_object()) {
        throw LevelError("A level must be a JSON object");
    }

    LevelData data;
    data.number = readInt(root, "number", 1, 99);
    data.startGold = readInt(root, "gold", 0, 1000000);
    data.startLives = readInt(root, "lives", 1, std::numeric_limits<std::uint8_t>::max());  // Player bewaart levens als uint8_t
    data.waveCount = readInt(root, "waves", 1, 100);
    data.route = readPoints(root, "route", 2);   // een weg heeft minstens een begin en een einde
    data.slotPositions = readPoints(root, "slots", 0);
    return data;
}

// Vereiste 47: 47. useful usage of (modern) file-I/O
// std::ifstream en std::filesystem lezen het levelbestand; een ontbrekend bestand wordt een LevelError.
std::string readTextFile(const std::filesystem::path& path)
{
    if (!std::filesystem::is_regular_file(path)) {
        throw LevelError("Level file not found: " + path.filename().string());
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw LevelError("Cannot open level file: " + path.filename().string());
    }
    std::ostringstream content;
    content << file.rdbuf();
    return content.str();
}

LevelData loadLevel(const std::filesystem::path& path)
{
    return parseLevel(readTextFile(path));
}

// Vereiste 24: 24. at least 2 destructors
// De destructor wacht op de werkthread. Zonder join() zou een std::thread die nog loopt het programma
// beëindigen (std::terminate) en zou de thread data van een al vernietigd object gebruiken.
AsyncLevelLoader::~AsyncLevelLoader()
{
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void AsyncLevelLoader::start(const std::filesystem::path& path)
{
    if (m_started) {
        throw std::logic_error("AsyncLevelLoader was already started");
    }
    m_started = true;

    // Vereiste 49: 49. useful usage of a lambda function
    // De lambda geeft de thread een functie mee die run() op dit object aanroept met een eigen kopie van het pad.
    m_thread = std::thread([this, path] { run(path); });
}

bool AsyncLevelLoader::isFinished() const
{
    return m_finished.load();
}

LevelData AsyncLevelLoader::result()
{
    if (!m_started) {
        throw std::logic_error("AsyncLevelLoader was never started");
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }

    const std::lock_guard<std::mutex> lock(m_mutex);
    if (m_error) {
        std::rethrow_exception(m_error);
    }
    return m_data;
}

std::thread::id AsyncLevelLoader::workerThreadId() const
{
    const std::lock_guard<std::mutex> lock(m_mutex);
    return m_workerId;
}

void AsyncLevelLoader::run(const std::filesystem::path& path)
{
    LevelData data;
    std::exception_ptr error;
    try {
        data = loadLevel(path);
    } catch (...) {
        // Een exception mag de thread nooit verlaten; ze wordt bewaard en door result() opnieuw gegooid.
        error = std::current_exception();
    }

    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_data = std::move(data);
        m_error = error;
        m_workerId = std::this_thread::get_id();
    }
    m_finished = true;
}

}  // namespace ktd
