#pragma once

#include <QByteArray>
#include <QMainWindow>
#include <QVector>

class BoardScene;
class BoardView;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QTimer;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setTool(int tool);

private slots:
    void newBoard();
    void openBoard();
    bool saveBoard();
    bool saveBoardAs();
    void exportPng();
    void chooseColor();
    void deleteSelection();
    void undo();
    void redo();
    void recordHistory();
    void autosave();

private:
    void buildUi();
    void updateTitle();
    void restoreSnapshot(const QByteArray &snapshot);

    BoardScene *m_scene = nullptr;
    BoardView *m_view = nullptr;
    QDoubleSpinBox *m_widthSpin = nullptr;
    QLabel *m_colorSwatch = nullptr;
    QTimer *m_autosaveTimer = nullptr;

    QString m_filePath;
    bool m_dirty = false;
    bool m_restoring = false;
    QVector<QByteArray> m_history;
    int m_historyIndex = -1;
};
