#pragma once

#include <QGraphicsView>

class BoardView final : public QGraphicsView {
    Q_OBJECT
public:
    explicit BoardView(QGraphicsScene *scene, QWidget *parent = nullptr);

    void setPanMode(bool active);
    void setSelectionMode(bool active);
    void resetZoom();
    void zoomBy(qreal factor);
    int zoomPercent() const;

signals:
    void zoomChanged(int percent);

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    void updateDragMode();

    bool m_panMode = false;
    bool m_spacePan = false;
    bool m_selectionMode = false;
    qreal m_zoom = 1.0;
};
