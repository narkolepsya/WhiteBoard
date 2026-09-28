#pragma once

#include <QColor>
#include <QGraphicsScene>
#include <QPainterPath>
#include <QPen>

class QGraphicsPathItem;
class QGraphicsItem;
class QPixmap;

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

    enum class BackgroundStyle {
        Solid,
        Dots,
        Grid,
        Ruled
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

    void setStrokeOpacity(qreal opacity);
    qreal strokeOpacity() const { return m_strokeOpacity; }

    void setStabilization(int amount);
    int stabilization() const { return m_stabilization; }

    void setGridVisible(bool visible);
    bool gridVisible() const { return m_gridVisible; }

    void setBackgroundColor(const QColor &color);
    QColor backgroundColor() const { return m_backgroundColor; }

    void setBackgroundStyle(BackgroundStyle style);
    BackgroundStyle backgroundStyle() const { return m_backgroundStyle; }

    bool pasteImageAt(const QPointF &pos);

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
    void insertPixmapAt(const QPixmap &pixmap, const QPointF &pos, const QString &sourcePath = {});
    void finishShape(const QPointF &pos);
    void makeInteractive(QGraphicsItem *item);

    Tool m_tool = Tool::Pen;
    QColor m_color = QColor("#20242d");
    qreal m_strokeWidth = 3.0;
    qreal m_strokeOpacity = 1.0;
    int m_stabilization = 35;
    bool m_gridVisible = false;
    QColor m_backgroundColor = QColor("#f3f2f1");
    BackgroundStyle m_backgroundStyle = BackgroundStyle::Solid;

    bool m_drawing = false;
    QPointF m_startPos;
    QPointF m_smoothedPos;
    QPainterPath m_path;
    QGraphicsPathItem *m_currentStroke = nullptr;
    QGraphicsItem *m_previewShape = nullptr;
};
