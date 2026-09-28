#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>

class QGraphicsPathItem;
class QGraphicsItem;

class BoardScene final : public QGraphicsScene {
    Q_OBJECT
public:
    enum class Tool {
        Select,
        Pan,
        Pen,
        Highlighter,
        Eraser,
        Text,
        Rectangle,
        Ellipse,
        Line,
        StickyNote,
        Image
    };

    enum DataRole {
        TypeRole = Qt::UserRole + 1,
        PayloadRole
    };

    explicit BoardScene(QObject *parent = nullptr);

    void setTool(Tool tool);
    Tool tool() const { return m_tool; }
    void setColor(const QColor &color) { m_color = color; }
    QColor color() const { return m_color; }
    void setStrokeWidth(qreal width) { m_strokeWidth = width; }
    qreal strokeWidth() const { return m_strokeWidth; }
    void setGridVisible(bool visible);
    bool gridVisible() const { return m_gridVisible; }

signals:
    void contentChanged();
    void panRequested(bool active);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void drawBackground(QPainter *painter, const QRectF &rect) override;

private:
    void beginStroke(const QPointF &pos, bool highlighter);
    void updateStroke(const QPointF &pos);
    void eraseAt(const QPointF &pos);
    void addTextAt(const QPointF &pos, bool sticky);
    void addImageAt(const QPointF &pos);
    void finishShape(const QPointF &pos);
    void makeInteractive(QGraphicsItem *item);

    Tool m_tool = Tool::Pen;
    QColor m_color = QColor("#111827");
    qreal m_strokeWidth = 3.0;
    bool m_gridVisible = false;

    bool m_drawing = false;
    QPointF m_startPos;
    QPainterPath m_path;
    QGraphicsPathItem *m_currentStroke = nullptr;
    QGraphicsItem *m_previewShape = nullptr;
};
