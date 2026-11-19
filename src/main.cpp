#include <QApplication>
#include "ktd/MainWindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    ktd::MainWindow window;
    window.show();
    return app.exec();
}