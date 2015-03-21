#include "mainwindow.h"
#include "recognition.h"
#include <QApplication>

int main(int argc, char *argv[]) {
#ifdef DEFAULT_RECOGNITION_PRESET
    setRecognitionPreset(RecognitionPreset::DEFAULT_RECOGNITION_PRESET);
#else
#error Default recognition preset is not specified (can be Mouse or Touch)
#endif

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
