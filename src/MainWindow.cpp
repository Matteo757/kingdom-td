#include "ktd/MainWindow.h"

#include <QCoreApplication>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <exception>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include "ktd/GameView.h"
#include "ktd/Hud.h"

namespace ktd {
namespace {

constexpr int kStartLevel = 1;
constexpr int kPollIntervalMs = 30;

// Het levelbestand staat naast het programma: CMake kopieert assets/levels na het bouwen.
std::filesystem::path levelFile(int number)
{
    const std::filesystem::path folder(QCoreApplication::applicationDirPath().toStdWString());
    return folder / "levels" / ("level" + std::to_string(number) + ".json");
}

}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Kingdom TD");

    auto* loading = new QLabel("Level laden...", this);
    loading->setAlignment(Qt::AlignCenter);
    setCentralWidget(loading);
    resize(1060, 680);

    // Het levelbestand wordt in een aparte thread ingelezen; de GUI blijft intussen reageren.
    m_loader.start(levelFile(kStartLevel));
    connect(&m_pollTimer, &QTimer::timeout, this, &MainWindow::checkLevelLoaded);
    m_pollTimer.start(kPollIntervalMs);
}

void MainWindow::checkLevelLoaded()
{
    if (!m_loader.isFinished()) {
        return;
    }
    m_pollTimer.stop();

    std::optional<Game> game;
    int level = kStartLevel;
    try {
        const LevelData data = m_loader.result();
        game.emplace(Game::fromLevel(data));
        level = data.number;
    } catch (const std::exception& error) {
        // Ontbrekend of ongeldig bestand: val terug op het ingebouwde level in plaats van te stoppen.
        qWarning() << "Level kon niet geladen worden, ingebouwd level wordt gebruikt:" << error.what();
        game.emplace(Game::level(kStartLevel));
    }
    showGame(std::move(*game), level);
}

void MainWindow::showGame(Game game, int level)
{
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    auto* hud = new Hud(central);
    auto* view = new GameView(std::move(game), level, central);
    layout->addWidget(hud);
    layout->addWidget(view);
    setCentralWidget(central);   // vervangt (en verwijdert) het label "Level laden..."

    connect(view, &GameView::goldChanged, hud, &Hud::setGold);
    connect(view, &GameView::livesChanged, hud, &Hud::setLives);
    connect(view, &GameView::waveChanged, hud, &Hud::setWave);
    hud->setGold(view->player().gold());
    hud->setLives(view->player().lives());
    hud->setWave(0, view->totalWaves());
}

}
