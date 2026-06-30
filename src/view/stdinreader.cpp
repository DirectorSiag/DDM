#include "stdinreader.h"
#include <QTextStream>

StdinReader::StdinReader(QObject* parent) : QObject(parent) {}

void StdinReader::readLoop() {
    QTextStream in(stdin);
    while (true) {
        const QString l = in.readLine();
        if (l.isNull()) break;
        emit lineRead(l);
    }
    emit finished();
}
