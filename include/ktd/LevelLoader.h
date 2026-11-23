#ifndef KTD_LEVELLOADER_H
#define KTD_LEVELLOADER_H

#include <atomic>
#include <exception>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include "ktd/LevelData.h"

namespace ktd {

// Vereiste 48: 48. useful exception handling
// Alles wat misgaat bij het inlezen van een level (bestand ontbreekt, ongeldige JSON, ongeldige waarden)
// wordt als LevelError gemeld. De aanroeper hoeft zo maar één type te vangen en kan terugvallen op het
// ingebouwde level in plaats van te crashen.
class LevelError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

// Zet JSON-tekst om naar LevelData. Gooit LevelError bij ongeldige of ontbrekende gegevens.
LevelData parseLevel(const std::string& jsonText);

// Leest een heel tekstbestand. Gooit LevelError als het bestand niet geopend kan worden.
std::string readTextFile(const std::filesystem::path& path);

// Leest en verwerkt een levelbestand (readTextFile + parseLevel).
LevelData loadLevel(const std::filesystem::path& path);

// Vereiste 50: 50. useful usage of threads
// Laadt een levelbestand in een aparte thread zodat de GUI niet vastloopt terwijl er van schijf gelezen wordt.
// De werkthread schrijft het resultaat onder een mutex weg; de GUI-thread vraagt met isFinished() (atomic)
// of de thread klaar is en haalt het resultaat daarna op met result().
class AsyncLevelLoader {
public:
    AsyncLevelLoader() = default;
    ~AsyncLevelLoader();

    // Een loader beheert een thread en mag dus niet gekopieerd worden.
    AsyncLevelLoader(const AsyncLevelLoader&) = delete;
    AsyncLevelLoader& operator=(const AsyncLevelLoader&) = delete;

    void start(const std::filesystem::path& path);   // gooit std::logic_error als er al gestart is
    bool isFinished() const;                          // niet-blokkerend
    LevelData result();                               // wacht op de thread; gooit de fout van het laden opnieuw
    std::thread::id workerThreadId() const;           // id van de thread die het werk deed (voor de test)

private:
    void run(const std::filesystem::path& path);

    std::thread m_thread;
    mutable std::mutex m_mutex;       // beschermt m_data, m_error en m_workerId
    std::atomic<bool> m_finished{false};
    bool m_started = false;
    LevelData m_data;
    std::exception_ptr m_error;
    std::thread::id m_workerId;
};

}  // namespace ktd

#endif  // KTD_LEVELLOADER_H
