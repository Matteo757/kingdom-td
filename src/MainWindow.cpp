#include "ktd/MainWindow.h"

#include "ktd/GameView.h"

namespace ktd {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle("Kingdom TD");
    setCentralWidget(new GameView(this));
    resize(1060, 620);
}

}