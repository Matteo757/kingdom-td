#ifndef KTD_MAINWINDOW_H
#define KTD_MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include "ktd/Game.h"
#include "ktd/LevelLoader.h"

namespace ktd {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void checkLevelLoaded();

private:
    void showGame(Game game, int level);

    AsyncLevelLoader m_loader;
    QTimer m_pollTimer;
};

}

#endif  // KTD_MAINWINDOW_H
