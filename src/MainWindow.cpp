#include "ktd/MainWindow.h"

#include <QVBoxLayout>
#include "ktd/GameView.h"
#include "ktd/Hud.h"

namespace ktd {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Kingdom TD");

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    auto* hud = new Hud(central);
    auto* view = new GameView(central);
    layout->addWidget(hud);
    layout->addWidget(view);
    setCentralWidget(central);

    connect(view, &GameView::goldChanged, hud, &Hud::setGold);
    connect(view, &GameView::livesChanged, hud, &Hud::setLives);
    hud->setGold(view->player().gold());
    hud->setLives(view->player().lives());

    resize(1060, 680);
}

}