#include "ktd/MainWindow.h"

namespace ktd {

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("Kingdom TD");
    resize(1024, 640);
}

}  // namespace ktd