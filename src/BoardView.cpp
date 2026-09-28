#include "BoardView.h"

#include <QKeyEvent>
#include <QPainter>
#include <QWheelEvent>

BoardView::BoardView(QGraphicsScene *scene, QWidget *parent)
    : QGraphicsView(scene, parent) {
    setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing |
                   QPainter::SmoothPixmapTransform);
    setViewportUpdateMode(QGraphicsView::MinimalViewportUpdate);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setResizeAnchor(QGraphicsView::AnchorViewCenter);
    setDragMode(QGraphicsView::NoDrag);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::StrongFocus);
    setFrameShape(QFrame::NoFrame);
}

void BoardView::updateDragMode() {
    if (m_panMode || m_spacePan) {
        setDragMode(QGraphicsView::ScrollHandDrag);
    } else if (m_selectionMode) {
        setDragMode(QGraphicsView::RubberBandDrag);
    } else {
        setDragMode(QGraphicsView::NoDrag);
    }
}

void BoardView::setPanMode(bool active) {
    m_panMode = active;
    updateDragMode();
}

void BoardView::setSelectionMode(bool active) {
    m_selectionMode = active;
    updateDragMode();
}

void BoardView::resetZoom() {
    resetTransform();
    m_zoom = 1.0;
    emit zoomChanged(zoomPercent());
}

void BoardView::zoomBy(qreal factor) {
    const qreal next = m_zoom * factor;
    if (next < 0.08 || next > 12.0) return;
    scale(factor, factor);
    m_zoom = next;
    emit zoomChanged(zoomPercent());
}

int BoardView::zoomPercent() const {
    return qRound(m_zoom * 100.0);
}

void BoardView::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() == 0) {
        QGraphicsView::wheelEvent(event);
        return;
    }

    const qreal factor = event->angleDelta().y() > 0 ? 1.15 : 1.0 / 1.15;
    zoomBy(factor);
    event->accept();
}

void BoardView::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePan = true;
        updateDragMode();
        event->accept();
        return;
    }
    QGraphicsView::keyPressEvent(event);
}

void BoardView::keyReleaseEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_spacePan = false;
        updateDragMode();
        event->accept();
        return;
    }
    QGraphicsView::keyReleaseEvent(event);
}
