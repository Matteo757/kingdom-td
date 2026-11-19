#pragma once

#include <QMainWindow>

namespace ktd {

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
};

}