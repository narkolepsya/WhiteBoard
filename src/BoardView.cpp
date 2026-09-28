#include "BoardView.h"

#include <QKeyEvent>
#include <QPainter>
#include <QWheelEvent>

BoardView::BoardView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent) {
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::StrongFocus);
}

void BoardView::setPanMode(bool active) {
    m_panMode = active;
    setDragMode((m_panMode || m_spacePan) ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
}

void BoardView::resetZoom() {
    resetTransform();
    m_zoom = 1.0;
}

void BoardView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    const qreal next = m_zoom * factor;
    if (next >= 0.08 && next <= 12.0) {
        scale(factor, factor);
        m_zoom = next;
    }
    event->accept();
}

void BoardView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePan = true;
        setDragMode(QGraphicsView::ScrollHandDrag);
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void BoardView::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePan = false;
        setDragMode(m_panMode ? QGraphicsView::ScrollHandDrag : QGraphicsView::NoDrag);
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}
