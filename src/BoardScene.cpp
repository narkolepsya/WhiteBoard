#include "BoardScene.h"

#include <algorithm>
#include <cmath>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QFont>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QGraphicsPathItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsTextItem>
#include <QInputDialog>
#include <QPainter>
#include <QPixmap>

namespace {
constexpr auto kStroke = "stroke";
constexpr auto kHighlight = "highlight";
constexpr auto kRect = "rect";
constexpr auto kEllipse = "ellipse";
constexpr auto kLine = "line";
constexpr auto kText = "text";
constexpr auto kSticky = "sticky";
constexpr auto kImage = "image";
}

BoardScene::BoardScene(QObject *parent) : QGraphicsScene(parent) {
    setSceneRect(-500000, -500000, 1000000, 1000000);
}

void BoardScene::setTool(Tool tool) {
    m_tool = tool;
    emit panRequested(tool == Tool::Pan);
}

void BoardScene::setStrokeOpacity(qreal opacity) {
    m_strokeOpacity = std::clamp(opacity, 0.05, 1.0);
}

void BoardScene::setStabilization(int amount) {
    m_stabilization = std::clamp(amount, 0, 100);
}

void BoardScene::setGridVisible(bool visible) {
    if (m_gridVisible == visible) return;
    m_gridVisible = visible;
    invalidate(sceneRect(), QGraphicsScene::BackgroundLayer);
}

void BoardScene::makeInteractive(QGraphicsItem *item) {
    if (!item) return;
    item->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);
}

void BoardScene::beginStroke(const QPointF &pos, bool highlighter) {
    m_drawing = true;
    m_smoothedPos = pos;
    m_path = QPainterPath(pos);
    auto *item = addPath(m_path);

    QColor strokeColor = m_color;
    const qreal opacity = highlighter ? std::min(m_strokeOpacity, 0.38) : m_strokeOpacity;
    strokeColor.setAlphaF(opacity);

    QPen pen(strokeColor, highlighter ? m_strokeWidth * 4.0 : m_strokeWidth,
             Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    item->setPen(pen);
    item->setData(TypeRole, highlighter ? kHighlight : kStroke);
    m_currentStroke = item;
}

void BoardScene::updateStroke(const QPointF &pos) {
    if (!m_currentStroke) return;

    QPointF point = pos;
    if (m_stabilization > 0) {
        const qreal amount = static_cast<qreal>(m_stabilization) / 100.0;
        const qreal responsiveness = 1.0 - (0.82 * amount);
        m_smoothedPos += (pos - m_smoothedPos) * responsiveness;
        point = m_smoothedPos;
    }

    m_path.lineTo(point);
    m_currentStroke->setPath(m_path);
}

void BoardScene::eraseAt(const QPointF &pos) {
    const auto hitItems = items(QRectF(pos.x() - 10, pos.y() - 10, 20, 20),
                                Qt::IntersectsItemShape, Qt::DescendingOrder);
    for (auto *item : hitItems) {
        if (item && !item->parentItem()) {
            removeItem(item);
            delete item;
            emit contentChanged();
            break;
        }
    }
}

void BoardScene::addTextAt(const QPointF &pos, bool sticky) {
    bool ok = false;
    const QString text = QInputDialog::getMultiLineText(nullptr,
        sticky ? tr("Nota adhesiva") : tr("Texto"), tr("Contenido:"), {}, &ok);
    if (!ok || text.trimmed().isEmpty()) return;

    auto *item = addText(text);
    item->setDefaultTextColor(sticky ? QColor("#3b2f00") : m_color);
    item->setFont(QFont(QStringLiteral("Sans Serif"), sticky ? 13 : 12));
    if (sticky) {
        item->setHtml(QString("<div style='background:#FFE999; padding:14px; min-width:180px;'>%1</div>")
                      .arg(text.toHtmlEscaped().replace("\n", "<br>")));
        item->setData(TypeRole, kSticky);
    } else {
        item->setData(TypeRole, kText);
    }
    item->setTextInteractionFlags(Qt::TextEditorInteraction);
    item->setPos(pos);
    makeInteractive(item);
    emit contentChanged();
}

void BoardScene::insertPixmapAt(const QPixmap &source, const QPointF &pos, const QString &sourcePath) {
    if (source.isNull()) return;

    QPixmap pixmap = source;
    if (pixmap.width() > 1600 || pixmap.height() > 1200) {
        pixmap = pixmap.scaled(1600, 1200, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    auto *item = addPixmap(pixmap);
    item->setData(TypeRole, kImage);
    item->setData(PayloadRole, sourcePath);
    item->setPos(pos);
    makeInteractive(item);
    emit contentChanged();
}

bool BoardScene::pasteImageAt(const QPointF &pos) {
    const QClipboard *clipboard = QApplication::clipboard();
    const QPixmap pixmap = clipboard->pixmap();
    if (pixmap.isNull()) return false;
    insertPixmapAt(pixmap, pos);
    return true;
}

void BoardScene::addImageAt(const QPointF &pos) {
    const QClipboard *clipboard = QApplication::clipboard();
    if (!clipboard->pixmap().isNull()) {
        insertPixmapAt(clipboard->pixmap(), pos);
        return;
    }

    const QString sourcePath = QFileDialog::getOpenFileName(
        nullptr, tr("Insertar imagen"), {}, tr("Imágenes (*.png *.jpg *.jpeg *.webp *.bmp)"));
    if (sourcePath.isEmpty()) return;

    QPixmap pixmap(sourcePath);
    if (pixmap.isNull()) return;
    insertPixmapAt(pixmap, pos, sourcePath);
}

void BoardScene::finishShape(const QPointF &pos) {
    const QRectF rect(m_startPos, pos);
    QColor color = m_color;
    color.setAlphaF(m_strokeOpacity);
    const QPen pen(color, m_strokeWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QGraphicsItem *item = nullptr;

    switch (m_tool) {
    case Tool::Rectangle: {
        auto *shape = addRect(rect.normalized(), pen, Qt::NoBrush);
        shape->setData(TypeRole, kRect);
        item = shape;
        break;
    }
    case Tool::Ellipse: {
        auto *shape = addEllipse(rect.normalized(), pen, Qt::NoBrush);
        shape->setData(TypeRole, kEllipse);
        item = shape;
        break;
    }
    case Tool::Line: {
        auto *shape = addLine(QLineF(m_startPos, pos), pen);
        shape->setData(TypeRole, kLine);
        item = shape;
        break;
    }
    default:
        break;
    }

    makeInteractive(item);
    if (item) emit contentChanged();
}

void BoardScene::mousePressEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() != Qt::LeftButton) {
        QGraphicsScene::mousePressEvent(event);
        return;
    }

    m_startPos = event->scenePos();

    switch (m_tool) {
    case Tool::Pen:
        beginStroke(m_startPos, false);
        event->accept();
        return;
    case Tool::Highlighter:
        beginStroke(m_startPos, true);
        event->accept();
        return;
    case Tool::Eraser:
        eraseAt(m_startPos);
        event->accept();
        return;
    case Tool::Text:
        addTextAt(m_startPos, false);
        event->accept();
        return;
    case Tool::StickyNote:
        addTextAt(m_startPos, true);
        event->accept();
        return;
    case Tool::Image:
        addImageAt(m_startPos);
        event->accept();
        return;
    case Tool::Rectangle:
    case Tool::Ellipse:
    case Tool::Line:
        m_drawing = true;
        event->accept();
        return;
    case Tool::Pan:
        event->ignore();
        return;
    case Tool::Select:
        break;
    }

    QGraphicsScene::mousePressEvent(event);
}

void BoardScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event) {
    switch (m_tool) {
    case Tool::Pen:
    case Tool::Highlighter:
        if (m_drawing) {
            updateStroke(event->scenePos());
            event->accept();
            return;
        }
        break;
    case Tool::Eraser:
        if (event->buttons() & Qt::LeftButton) {
            eraseAt(event->scenePos());
            event->accept();
            return;
        }
        break;
    default:
        break;
    }
    QGraphicsScene::mouseMoveEvent(event);
}

void BoardScene::mouseReleaseEvent(QGraphicsSceneMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        if ((m_tool == Tool::Pen || m_tool == Tool::Highlighter) && m_drawing) {
            updateStroke(event->scenePos());
            if (m_stabilization > 0) {
                m_path.lineTo(event->scenePos());
                if (m_currentStroke) m_currentStroke->setPath(m_path);
            }
            makeInteractive(m_currentStroke);
            m_currentStroke = nullptr;
            m_drawing = false;
            emit contentChanged();
            event->accept();
            return;
        }
        if ((m_tool == Tool::Rectangle || m_tool == Tool::Ellipse || m_tool == Tool::Line) && m_drawing) {
            m_drawing = false;
            finishShape(event->scenePos());
            event->accept();
            return;
        }
    }
    QGraphicsScene::mouseReleaseEvent(event);
    if (m_tool == Tool::Select && event->button() == Qt::LeftButton)
        emit contentChanged();
}

void BoardScene::drawBackground(QPainter *painter, const QRectF &rect) {
    painter->fillRect(rect, QColor("#f5f5f6"));
    if (!m_gridVisible) return;

    constexpr qreal grid = 32.0;
    const qreal left = std::floor(rect.left() / grid) * grid;
    const qreal top = std::floor(rect.top() / grid) * grid;

    QPen pen(QColor(218, 221, 226), 0);
    painter->setPen(pen);
    for (qreal x = left; x < rect.right(); x += grid)
        painter->drawLine(QLineF(x, rect.top(), x, rect.bottom()));
    for (qreal y = top; y < rect.bottom(); y += grid)
        painter->drawLine(QLineF(rect.left(), y, rect.right(), y));
}
