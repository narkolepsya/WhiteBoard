#pragma once

#include <QByteArray>
#include <QString>

class BoardScene;

class BoardSerializer {
public:
    static QByteArray toJson(const BoardScene &scene);
    static bool fromJson(BoardScene &scene, const QByteArray &data, QString *error = nullptr);
    static bool saveFile(const BoardScene &scene, const QString &path, QString *error = nullptr);
    static bool loadFile(BoardScene &scene, const QString &path, QString *error = nullptr);
};
