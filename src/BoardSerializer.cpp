#include "BoardSerializer.h"
#include "BoardScene.h"

#include <QBuffer>
#include <QFile>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainterPath>
#include <QPen>

namespace {
QJsonObject penToJson(const QPen &pen) {
    return {{"color", pen.color().name(QColor::HexArgb)}, {"width", pen.widthF()}};
}
QPen penFromJson(const QJsonObject &o) {
    return QPen(QColor(o["color"].toString()), o["width"].toDouble(2.0),
                Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
}
QJsonObject baseItem(const QGraphicsItem *item) {
    return {{"type", item->data(BoardScene::TypeRole).toString()},
            {"x", item->pos().x()}, {"y", item->pos().y()},
            {"z", item->zValue()}, {"rotation", item->rotation()},
            {"scale", item->scale()}, {"opacity", item->opacity()}};
}
void applyBase(QGraphicsItem *item, const QJsonObject &o) {
    item->setPos(o["x"].toDouble(), o["y"].toDouble());
    item->setZValue(o["z"].toDouble());
    item->setRotation(o["rotation"].toDouble());
    item->setScale(o["scale"].toDouble(1.0));
    item->setOpacity(o["opacity"].toDouble(1.0));
    item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}
}

QByteArray BoardSerializer::toJson(const BoardScene &scene) {
    QJsonArray itemsArray;
    for (auto *item : scene.items(Qt::AscendingOrder)) {
        if (!item || item->parentItem()) continue;
        QString type = item->data(BoardScene::TypeRole).toString();
        if (type.isEmpty()) continue;

        QJsonObject o = baseItem(item);
        if (auto *path = qgraphicsitem_cast<QGraphicsPathItem *>(item)) {
            QJsonArray pts;
            const QPainterPath p = path->path();
            for (int i = 0; i < p.elementCount(); ++i) {
                const auto e = p.elementAt(i);
                pts.append(QJsonArray{e.x, e.y});
            }
            o["points"] = pts;
            o["pen"] = penToJson(path->pen());
        } else if (auto *r = qgraphicsitem_cast<QGraphicsRectItem *>(item)) {
            const QRectF g = r->rect();
            o["rect"] = QJsonArray{g.x(), g.y(), g.width(), g.height()};
            o["pen"] = penToJson(r->pen());
        } else if (auto *e = qgraphicsitem_cast<QGraphicsEllipseItem *>(item)) {
            const QRectF g = e->rect();
            o["rect"] = QJsonArray{g.x(), g.y(), g.width(), g.height()};
            o["pen"] = penToJson(e->pen());
        } else if (auto *l = qgraphicsitem_cast<QGraphicsLineItem *>(item)) {
            const QLineF line = l->line();
            o["line"] = QJsonArray{line.x1(), line.y1(), line.x2(), line.y2()};
            o["pen"] = penToJson(l->pen());
        } else if (auto *t = qgraphicsitem_cast<QGraphicsTextItem *>(item)) {
            o["html"] = t->toHtml();
            o["textColor"] = t->defaultTextColor().name(QColor::HexArgb);
            o["fontFamily"] = t->font().family();
            o["fontSize"] = t->font().pointSizeF();
        } else if (auto *px = qgraphicsitem_cast<QGraphicsPixmapItem *>(item)) {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            px->pixmap().save(&buffer, "PNG");
            o["png"] = QString::fromLatin1(bytes.toBase64());
        }
        itemsArray.append(o);
    }

    QJsonObject root{{"format", "StudyBoard"}, {"version", 1},
                     {"grid", scene.gridVisible()}, {"items", itemsArray}};
    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

bool BoardSerializer::fromJson(BoardScene &scene, const QByteArray &data, QString *error) {
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        if (error) *error = parseError.errorString();
        return false;
    }
    const auto root = doc.object();
    if (root["format"].toString() != "StudyBoard") {
        if (error) *error = QObject::tr("Formato de archivo no reconocido.");
        return false;
    }

    scene.clear();
    scene.setGridVisible(root["grid"].toBool());

    for (const auto &value : root["items"].toArray()) {
        const auto o = value.toObject();
        const QString type = o["type"].toString();
        QGraphicsItem *item = nullptr;

        if (type == "stroke" || type == "highlight") {
            QPainterPath path;
            bool first = true;
            for (const auto &v : o["points"].toArray()) {
                const auto p = v.toArray();
                QPointF pt(p[0].toDouble(), p[1].toDouble());
                if (first) { path.moveTo(pt); first = false; }
                else path.lineTo(pt);
            }
            auto *x = scene.addPath(path, penFromJson(o["pen"].toObject()));
            item = x;
        } else if (type == "rect") {
            const auto a = o["rect"].toArray();
            item = scene.addRect(QRectF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble()),
                                 penFromJson(o["pen"].toObject()));
        } else if (type == "ellipse") {
            const auto a = o["rect"].toArray();
            item = scene.addEllipse(QRectF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble()),
                                    penFromJson(o["pen"].toObject()));
        } else if (type == "line") {
            const auto a = o["line"].toArray();
            item = scene.addLine(QLineF(a[0].toDouble(), a[1].toDouble(), a[2].toDouble(), a[3].toDouble()),
                                 penFromJson(o["pen"].toObject()));
        } else if (type == "text" || type == "sticky") {
            auto *x = scene.addText(QString());
            x->setHtml(o["html"].toString());
            x->setDefaultTextColor(QColor(o["textColor"].toString()));
            QFont font(o["fontFamily"].toString());
            font.setPointSizeF(o["fontSize"].toDouble(12.0));
            x->setFont(font);
            x->setTextInteractionFlags(Qt::TextEditorInteraction);
            item = x;
        } else if (type == "image") {
            const QByteArray bytes = QByteArray::fromBase64(o["png"].toString().toLatin1());
            QPixmap pixmap;
            pixmap.loadFromData(bytes, "PNG");
            item = scene.addPixmap(pixmap);
        }

        if (item) {
            item->setData(BoardScene::TypeRole, type);
            applyBase(item, o);
        }
    }
    return true;
}

bool BoardSerializer::saveFile(const BoardScene &scene, const QString &path, QString *error) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (error) *error = f.errorString();
        return false;
    }
    if (f.write(toJson(scene)) < 0) {
        if (error) *error = f.errorString();
        return false;
    }
    return true;
}

bool BoardSerializer::loadFile(BoardScene &scene, const QString &path, QString *error) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        if (error) *error = f.errorString();
        return false;
    }
    return fromJson(scene, f.readAll(), error);
}
