#ifndef KTD_MAINWINDOW_H
#define KTD_MAINWINDOW_H

#include <QMainWindow>

namespace ktd {

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
};

}  // namespace ktd

#endif  // KTD_MAINWINDOW_H