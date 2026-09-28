#pragma once

#include <QGraphicsView>

class BoardView final : public QGraphicsView {
    Q_OBJECT
public:
    explicit BoardView(QGraphicsScene *scene, QWidget *parent = nullptr);
    void setPanMode(bool active);
    void resetZoom();

protected:
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    bool m_panMode = false;
    bool m_spacePan = false;
    qreal m_zoom = 1.0;
};
