#pragma once

#include <QByteArray>
#include <QHash>
#include <QMainWindow>
#include <QVector>

class BoardScene;
class BoardView;
class QCloseEvent;
class QFrame;
class QGraphicsOpacityEffect;
class QGridLayout;
class QLabel;
class QLineEdit;
class QResizeEvent;
class QSlider;
class QStackedWidget;
class QTimer;
class QToolButton;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    void setTool(int tool);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newBoard();
    void openBoard();
    bool saveBoard();
    bool saveBoardAs();
    void exportPng();
    void deleteSelection();
    void undo();
    void redo();
    void recordHistory();
    void autosave();
    void showHome();
    void showPenPopup();
    void showShapesMenu();
    void showMoreMenu();
    void pasteFromClipboard();
    void renameCurrentBoard();

private:
    void buildUi();
    void buildHomePage();
    void buildBoardPage();
    void buildShortcuts();
    void buildPenPopup();
    void refreshGallery();
    void openBoardPath(const QString &path);
    void showBoardPage();
    void updateTitle();
    void restoreSnapshot(const QByteArray &snapshot);
    void repositionOverlays();
    void saveThumbnail();
    void addRecentBoard(const QString &path);
    void styleButtonActive(QToolButton *button, bool active);
    void animatePopup(QFrame *popup, QGraphicsOpacityEffect *effect, bool show, const QRect &finalGeometry);

    QString boardsDirectory() const;
    QString thumbnailPath(const QString &path) const;
    QString boardTitle(const QString &path) const;
    void setBoardTitle(const QString &path, const QString &title);

    QToolButton *createToolbarButton(const QString &fallbackText, const QString &tooltip,
                                     bool checkable = false);

    BoardScene *m_scene = nullptr;
    BoardView *m_view = nullptr;

    QStackedWidget *m_pages = nullptr;
    QWidget *m_homePage = nullptr;
    QWidget *m_boardPage = nullptr;
    QGridLayout *m_galleryGrid = nullptr;

    QFrame *m_bottomBar = nullptr;
    QFrame *m_zoomBar = nullptr;
    QFrame *m_penPopup = nullptr;
    QGraphicsOpacityEffect *m_penPopupEffect = nullptr;
    QLineEdit *m_titleEdit = nullptr;
    QLabel *m_saveState = nullptr;
    QLabel *m_zoomLabel = nullptr;

    QSlider *m_widthSlider = nullptr;
    QSlider *m_opacitySlider = nullptr;
    QSlider *m_stabilizationSlider = nullptr;
    QLabel *m_widthValue = nullptr;
    QLabel *m_opacityValue = nullptr;
    QLabel *m_stabilizationValue = nullptr;

    QToolButton *m_penButton = nullptr;
    QToolButton *m_highlighterButton = nullptr;
    QToolButton *m_shapesButton = nullptr;
    QHash<int, QToolButton *> m_toolButtons;

    QTimer *m_autosaveTimer = nullptr;
    QString m_filePath;
    bool m_dirty = false;
    bool m_restoring = false;
    QVector<QByteArray> m_history;
    int m_historyIndex = -1;
};
