#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    QApplication::setApplicationName("StudyBoard");
    QApplication::setApplicationDisplayName("StudyBoard");
    QApplication::setOrganizationName("StudyBoard");
    QApplication::setWindowIcon(QIcon(":/icons/studyboard.svg"));
    app.setStyle("Fusion");

    MainWindow window;
    window.show();
    return app.exec();
}
