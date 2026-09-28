#include "MainWindow.h"
#include "BoardScene.h"
#include "BoardSerializer.h"
#include "BoardView.h"

#include <QAction>
#include <QApplication>
#include <functional>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsItem>
#include <QGraphicsOpacityEffect>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QImage>
#include <QKeySequence>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QUuid>
#include <QVBoxLayout>

namespace {
QString boardKey(const QString &path) {
    return QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex());
}

QString prettyModified(const QFileInfo &info) {
    const QDateTime modified = info.lastModified();
    if (!modified.isValid()) return {};
    if (modified.date() == QDate::currentDate())
        return QObject::tr("Editado hoy %1").arg(modified.time().toString("HH:mm"));
    return QObject::tr("Editado %1").arg(modified.toString("dd/MM/yyyy HH:mm"));
}

QIcon themedIcon(const QStringList &names, const QIcon &fallback = QIcon()) {
    for (const QString &name : names) {
        const QIcon icon = QIcon::fromTheme(name);
        if (!icon.isNull()) return icon;
    }
    return fallback;
}

void applyShadow(QWidget *widget, int blur = 28, int offsetY = 8, QColor color = QColor(18, 18, 23, 36)) {
    auto *shadow = new QGraphicsDropShadowEffect(widget);
    shadow->setBlurRadius(blur);
    shadow->setOffset(0, offsetY);
    shadow->setColor(color);
    widget->setGraphicsEffect(shadow);
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    recordHistory();

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setInterval(1100);
    m_autosaveTimer->setSingleShot(true);

    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::autosave);
    connect(m_scene, &BoardScene::contentChanged, this, [this] {
        m_dirty = true;
        m_saveState->setText(tr("Guardando…"));
        recordHistory();
        m_autosaveTimer->start();
        updateTitle();
    });

    refreshGallery();
}

void MainWindow::buildUi() {
    resize(1480, 920);
    setMinimumSize(980, 640);
    setWindowTitle("StudyBoard");
    menuBar()->hide();
    statusBar()->hide();

    setStyleSheet(R"(
        QMainWindow { background: #eeedec; }
        QWidget { font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif; color: #202124; }

        QFrame#HomeHeader {
            background: #4a4744;
            border: none;
        }
        QLabel#HomeTitle {
            color: #ffffff;
            font-weight: 700;
            font-size: 18px;
            letter-spacing: 0.2px;
        }
        QLabel#HomeSubtitle {
            color: rgba(255,255,255,0.72);
            font-size: 12px;
        }

        QScrollArea, QWidget#GalleryCanvas {
            background: #efefef;
            border: none;
        }

        QFrame#BoardCard {
            background: #ffffff;
            border: 1px solid #e5e2df;
            border-radius: 18px;
        }
        QPushButton#PreviewButton {
            background: #f7f6f5;
            border: none;
            border-top-left-radius: 18px;
            border-top-right-radius: 18px;
            text-align: center;
            padding: 0;
        }
        QPushButton#PreviewButton:hover {
            background: #f1f0ee;
        }
        QPushButton#NewBoardButton {
            background: qlineargradient(x1:0,y1:0,x2:1,y2:1, stop:0 #f1f5ff, stop:1 #e5ecff);
            color: #2e5ce6;
            border: none;
            border-radius: 18px;
            font-size: 42px;
            font-weight: 300;
        }
        QPushButton#NewBoardButton:hover { background: #dfe8ff; }
        QLabel#NewBoardLabel {
            background: transparent;
            color: #1f2430;
            font-size: 15px;
            font-weight: 600;
            padding: 0 0 14px 0;
        }
        QLabel#CardTitle {
            color: #191919;
            font-weight: 650;
            font-size: 14px;
        }
        QLabel#CardMeta {
            color: #77747b;
            font-size: 11px;
        }

        QFrame#TopBar {
            background: rgba(255,255,255,0.92);
            border-bottom: 1px solid #e7e4e2;
        }

        QFrame#FloatingBar, QFrame#ZoomBar, QFrame#PenPopup {
            background: rgba(255,255,255,0.96);
            border: 1px solid #e3dfdb;
            border-radius: 18px;
        }

        QToolButton {
            background: transparent;
            border: none;
            border-radius: 12px;
            color: #1f1f21;
            padding: 6px;
            font-size: 14px;
        }
        QToolButton:hover { background: #f0eeeb; }
        QToolButton[toolActive="true"] {
            background: #ece9e4;
            color: #131417;
        }
        QToolButton#PrimaryButton {
            background: #f6f7fb;
            color: #2e5ce6;
            border: 1px solid #dce3ff;
            padding: 7px 12px;
        }
        QToolButton#PrimaryButton:hover { background: #edf1ff; }

        QLineEdit#BoardTitle {
            background: transparent;
            border: none;
            color: #161618;
            font-weight: 600;
            font-size: 16px;
            padding: 5px 7px;
        }
        QLineEdit#BoardTitle:focus {
            background: #f7f4f1;
            border-radius: 10px;
        }
        QLabel#SaveState {
            color: #706d74;
            font-size: 12px;
            padding-right: 6px;
        }
        QLabel#PanelTitle {
            color: #212124;
            font-weight: 650;
            font-size: 13px;
        }
        QLabel#PanelValue {
            color: #706d74;
            font-size: 12px;
        }

        QSlider::groove:horizontal {
            height: 4px;
            background: #dedad6;
            border-radius: 2px;
        }
        QSlider::handle:horizontal {
            width: 14px;
            margin: -6px 0;
            background: #ffffff;
            border: 1px solid #78757d;
            border-radius: 7px;
        }
        QSlider::sub-page:horizontal {
            background: #26272b;
            border-radius: 2px;
        }

        QMenu {
            background: #ffffff;
            color: #1b1b1e;
            border: 1px solid #e1dfdc;
            padding: 6px;
            border-radius: 10px;
        }
        QMenu::item {
            padding: 8px 28px 8px 12px;
            border-radius: 8px;
        }
        QMenu::item:selected { background: #f1efec; }
        QToolTip {
            background: #2f3137;
            color: white;
            border: none;
            padding: 6px 8px;
            border-radius: 8px;
        }
    )");

    m_pages = new QStackedWidget(this);
    setCentralWidget(m_pages);

    buildHomePage();
    buildBoardPage();
    buildShortcuts();

    m_pages->setCurrentWidget(m_homePage);
}

void MainWindow::buildHomePage() {
    m_homePage = new QWidget(m_pages);
    auto *layout = new QVBoxLayout(m_homePage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *header = new QFrame(m_homePage);
    header->setObjectName("HomeHeader");
    header->setFixedHeight(60);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 18, 0);
    headerLayout->setSpacing(14);

    auto *titles = new QVBoxLayout();
    titles->setSpacing(1);
    auto *appTitle = new QLabel(tr("StudyBoard"), header);
    appTitle->setObjectName("HomeTitle");
    auto *subtitle = new QLabel(tr("Pizarras locales para estudiar, dibujar y organizar ideas"), header);
    subtitle->setObjectName("HomeSubtitle");
    titles->addWidget(appTitle);
    titles->addWidget(subtitle);
    headerLayout->addLayout(titles);
    headerLayout->addStretch();

    auto *open = new QToolButton(header);
    open->setObjectName("PrimaryButton");
    open->setText(tr("Abrir archivo"));
    open->setIcon(themedIcon({"document-open-symbolic", "document-open"}, style()->standardIcon(QStyle::SP_DialogOpenButton)));
    open->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    connect(open, &QToolButton::clicked, this, &MainWindow::openBoard);
    headerLayout->addWidget(open);
    layout->addWidget(header);

    auto *scroll = new QScrollArea(m_homePage);
    scroll->setWidgetResizable(true);
    auto *canvas = new QWidget(scroll);
    canvas->setObjectName("GalleryCanvas");
    m_galleryGrid = new QGridLayout(canvas);
    m_galleryGrid->setContentsMargins(36, 28, 36, 36);
    m_galleryGrid->setHorizontalSpacing(22);
    m_galleryGrid->setVerticalSpacing(22);
    m_galleryGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scroll->setWidget(canvas);
    layout->addWidget(scroll, 1);

    m_pages->addWidget(m_homePage);
}

QToolButton *MainWindow::createToolbarButton(const QString &fallbackText, const QString &tooltip, bool checkable) {
    auto *button = new QToolButton(m_bottomBar);
    button->setText(fallbackText);
    button->setToolTip(tooltip);
    button->setCheckable(checkable);
    button->setFixedSize(42, 42);
    button->setProperty("toolActive", false);
    return button;
}

void MainWindow::buildBoardPage() {
    m_boardPage = new QWidget(m_pages);
    auto *layout = new QVBoxLayout(m_boardPage);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *topBar = new QFrame(m_boardPage);
    topBar->setObjectName("TopBar");
    topBar->setFixedHeight(54);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 0, 12, 0);
    topLayout->setSpacing(6);

    auto *homeButton = new QToolButton(topBar);
    homeButton->setIcon(themedIcon({"go-home-symbolic", "go-home"}, style()->standardIcon(QStyle::SP_DesktopIcon)));
    homeButton->setToolTip(tr("Volver a mis pizarras"));
    homeButton->setFixedSize(38, 38);
    connect(homeButton, &QToolButton::clicked, this, &MainWindow::showHome);
    topLayout->addWidget(homeButton);

    auto *separator = new QFrame(topBar);
    separator->setFrameShape(QFrame::VLine);
    separator->setStyleSheet("color:#dfdcda;");
    separator->setFixedHeight(22);
    topLayout->addWidget(separator);

    m_titleEdit = new QLineEdit(tr("Pizarra sin título"), topBar);
    m_titleEdit->setObjectName("BoardTitle");
    m_titleEdit->setMaximumWidth(360);
    connect(m_titleEdit, &QLineEdit::editingFinished, this, &MainWindow::renameCurrentBoard);
    topLayout->addWidget(m_titleEdit);
    topLayout->addStretch();

    m_saveState = new QLabel(tr("Guardado"), topBar);
    m_saveState->setObjectName("SaveState");
    topLayout->addWidget(m_saveState);

    auto *exportButton = new QToolButton(topBar);
    exportButton->setText(tr("Exportar"));
    exportButton->setObjectName("PrimaryButton");
    exportButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    exportButton->setIcon(themedIcon({"document-save-as-symbolic", "document-export"}, style()->standardIcon(QStyle::SP_DialogSaveButton)));
    exportButton->setToolTip(tr("Exportar la pizarra como imagen"));
    connect(exportButton, &QToolButton::clicked, this, &MainWindow::exportPng);
    topLayout->addWidget(exportButton);

    auto *settingsButton = new QToolButton(topBar);
    settingsButton->setIcon(themedIcon({"open-menu-symbolic", "preferences-system"}, style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    settingsButton->setToolTip(tr("Opciones"));
    settingsButton->setFixedSize(38, 38);
    connect(settingsButton, &QToolButton::clicked, this, &MainWindow::showMoreMenu);
    topLayout->addWidget(settingsButton);

    layout->addWidget(topBar);

    m_scene = new BoardScene(this);
    m_view = new BoardView(m_scene, m_boardPage);
    layout->addWidget(m_view, 1);
    connect(m_scene, &BoardScene::panRequested, m_view, &BoardView::setPanMode);

    m_bottomBar = new QFrame(m_boardPage);
    m_bottomBar->setObjectName("FloatingBar");
    auto *tools = new QHBoxLayout(m_bottomBar);
    tools->setContentsMargins(8, 7, 8, 7);
    tools->setSpacing(4);

    auto *undoButton = createToolbarButton(QStringLiteral("↶"), tr("Deshacer (Ctrl+Z)"));
    undoButton->setIcon(themedIcon({"edit-undo-symbolic", "edit-undo"}, style()->standardIcon(QStyle::SP_ArrowBack)));
    connect(undoButton, &QToolButton::clicked, this, &MainWindow::undo);
    tools->addWidget(undoButton);

    auto addTool = [this, tools](const QString &fallbackText, const QString &tip, BoardScene::Tool tool,
                                 const QStringList &iconNames, const QIcon &fallbackIcon = QIcon()) {
        auto *button = createToolbarButton(fallbackText, tip, true);
        button->setIcon(themedIcon(iconNames, fallbackIcon));
        button->setIconSize(QSize(18, 18));
        const int value = static_cast<int>(tool);
        m_toolButtons.insert(value, button);
        connect(button, &QToolButton::clicked, this, [this, value] { setTool(value); });
        tools->addWidget(button);
        return button;
    };

    addTool(QStringLiteral("➤"), tr("Seleccionar"), BoardScene::Tool::Select,
            {"transform-move", "cursor-arrow"}, style()->standardIcon(QStyle::SP_ArrowForward));
    addTool(QStringLiteral("✋"), tr("Mover lienzo (o mantén Espacio)"), BoardScene::Tool::Pan,
            {"input-touchpad-symbolic", "pan-up-symbolic"}, style()->standardIcon(QStyle::SP_DirOpenIcon));

    m_penButton = createToolbarButton(QStringLiteral("✎"), tr("Lápiz"), true);
    m_penButton->setIcon(themedIcon({"draw-freehand", "draw-freehand-symbolic"}));
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Pen), m_penButton);
    connect(m_penButton, &QToolButton::clicked, this, [this] {
        if (m_scene->tool() == BoardScene::Tool::Pen && m_penButton->property("toolActive").toBool())
            showPenPopup();
        else
            setTool(static_cast<int>(BoardScene::Tool::Pen));
    });
    tools->addWidget(m_penButton);

    m_highlighterButton = createToolbarButton(QStringLiteral("▰"), tr("Resaltador"), true);
    m_highlighterButton->setIcon(themedIcon({"draw-highlight", "format-text-highlight"}));
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Highlighter), m_highlighterButton);
    connect(m_highlighterButton, &QToolButton::clicked, this, [this] {
        if (m_scene->tool() == BoardScene::Tool::Highlighter && m_highlighterButton->property("toolActive").toBool())
            showPenPopup();
        else
            setTool(static_cast<int>(BoardScene::Tool::Highlighter));
    });
    tools->addWidget(m_highlighterButton);

    addTool(QStringLiteral("⌫"), tr("Borrador"), BoardScene::Tool::Eraser,
            {"draw-eraser", "edit-clear"}, style()->standardIcon(QStyle::SP_TrashIcon));
    addTool(QStringLiteral("▣"), tr("Nota adhesiva"), BoardScene::Tool::StickyNote,
            {"note", "mail-mark-important"});
    addTool(QStringLiteral("T"), tr("Texto"), BoardScene::Tool::Text,
            {"draw-text", "format-text-bold"});

    m_shapesButton = createToolbarButton(QStringLiteral("○□"), tr("Formas"));
    m_shapesButton->setIcon(themedIcon({"draw-rectangle", "applications-graphics"}, style()->standardIcon(QStyle::SP_FileDialogContentsView)));
    connect(m_shapesButton, &QToolButton::clicked, this, &MainWindow::showShapesMenu);
    tools->addWidget(m_shapesButton);

    auto *imageButton = createToolbarButton(QStringLiteral("▧"), tr("Imagen"), true);
    imageButton->setIcon(themedIcon({"insert-image", "image-x-generic"}, style()->standardIcon(QStyle::SP_FileIcon)));
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Image), imageButton);
    connect(imageButton, &QToolButton::clicked, this,
            [this] { setTool(static_cast<int>(BoardScene::Tool::Image)); });
    tools->addWidget(imageButton);

    auto *moreButton = createToolbarButton(QStringLiteral("⋯"), tr("Más opciones"));
    moreButton->setIcon(themedIcon({"open-menu-symbolic", "preferences-system"}, style()->standardIcon(QStyle::SP_FileDialogDetailedView)));
    connect(moreButton, &QToolButton::clicked, this, &MainWindow::showMoreMenu);
    tools->addWidget(moreButton);

    m_bottomBar->adjustSize();
    m_bottomBar->raise();
    applyShadow(m_bottomBar, 34, 10, QColor(32, 30, 30, 34));

    m_zoomBar = new QFrame(m_boardPage);
    m_zoomBar->setObjectName("ZoomBar");
    auto *zoomLayout = new QHBoxLayout(m_zoomBar);
    zoomLayout->setContentsMargins(8, 6, 8, 6);
    zoomLayout->setSpacing(4);

    auto *minus = new QToolButton(m_zoomBar);
    minus->setText(QStringLiteral("−"));
    minus->setFixedSize(34, 34);
    connect(minus, &QToolButton::clicked, this, [this] { m_view->zoomBy(1.0 / 1.15); });
    zoomLayout->addWidget(minus);

    m_zoomLabel = new QLabel("100%", m_zoomBar);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(50);
    zoomLayout->addWidget(m_zoomLabel);

    auto *plus = new QToolButton(m_zoomBar);
    plus->setText(QStringLiteral("+"));
    plus->setFixedSize(34, 34);
    connect(plus, &QToolButton::clicked, this, [this] { m_view->zoomBy(1.15); });
    zoomLayout->addWidget(plus);

    auto *reset = new QToolButton(m_zoomBar);
    reset->setText(QStringLiteral("⌗"));
    reset->setToolTip(tr("Restablecer zoom"));
    reset->setFixedSize(34, 34);
    connect(reset, &QToolButton::clicked, m_view, &BoardView::resetZoom);
    zoomLayout->addWidget(reset);

    connect(m_view, &BoardView::zoomChanged, this, [this](int percent) {
        m_zoomLabel->setText(QString::number(percent) + "%");
    });

    m_zoomBar->adjustSize();
    m_zoomBar->raise();
    applyShadow(m_zoomBar, 30, 8, QColor(32, 30, 30, 28));

    buildPenPopup();
    setTool(static_cast<int>(BoardScene::Tool::Pen));

    m_pages->addWidget(m_boardPage);
}

void MainWindow::buildPenPopup() {
    m_penPopup = new QFrame(m_boardPage);
    m_penPopup->setObjectName("PenPopup");
    m_penPopup->setFixedWidth(300);
    auto *layout = new QVBoxLayout(m_penPopup);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setSpacing(10);

    auto *panelTitle = new QLabel(tr("Ajustes del lápiz"), m_penPopup);
    panelTitle->setObjectName("PanelTitle");
    layout->addWidget(panelTitle);

    auto addSliderRow = [this, layout](const QString &name, int min, int max, int value,
                                       QSlider **sliderOut, QLabel **valueOut) {
        auto *titleRow = new QHBoxLayout();
        auto *nameLabel = new QLabel(name, m_penPopup);
        nameLabel->setObjectName("PanelTitle");
        auto *valueLabel = new QLabel(QString::number(value), m_penPopup);
        valueLabel->setObjectName("PanelValue");
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        titleRow->addWidget(nameLabel);
        titleRow->addStretch();
        titleRow->addWidget(valueLabel);
        layout->addLayout(titleRow);

        auto *slider = new QSlider(Qt::Horizontal, m_penPopup);
        slider->setRange(min, max);
        slider->setValue(value);
        layout->addWidget(slider);
        *sliderOut = slider;
        *valueOut = valueLabel;
    };

    QSettings settings;
    const int width = settings.value("pen/width", 3).toInt();
    const int opacity = settings.value("pen/opacity", 100).toInt();
    const int stabilization = settings.value("pen/stabilization", 35).toInt();

    addSliderRow(tr("Grosor"), 1, 30, width, &m_widthSlider, &m_widthValue);
    addSliderRow(tr("Opacidad"), 5, 100, opacity, &m_opacitySlider, &m_opacityValue);
    addSliderRow(tr("Estabilización"), 0, 100, stabilization,
                 &m_stabilizationSlider, &m_stabilizationValue);

    auto *paletteLabel = new QLabel(tr("Colores"), m_penPopup);
    paletteLabel->setObjectName("PanelTitle");
    layout->addWidget(paletteLabel);

    auto *colors = new QHBoxLayout();
    colors->setSpacing(8);
    const QStringList palette = {
        "#202124", "#e53935", "#fb8c00", "#16a34a",
        "#36bbed", "#2563eb", "#d81b60", "#7c3aed"
    };
    for (const QString &hex : palette) {
        auto *color = new QToolButton(m_penPopup);
        color->setFixedSize(28, 28);
        color->setToolTip(hex);
        color->setStyleSheet(QString(
            "QToolButton { background:%1; border:2px solid white; border-radius:14px; }"
            "QToolButton:hover { border:2px solid #8a8b92; }").arg(hex));
        connect(color, &QToolButton::clicked, this, [this, hex] {
            m_scene->setColor(QColor(hex));
            QSettings().setValue("pen/color", hex);
        });
        colors->addWidget(color);
    }
    layout->addLayout(colors);

    connect(m_widthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_widthValue->setText(QString::number(value) + " px");
        m_scene->setStrokeWidth(value);
        QSettings().setValue("pen/width", value);
    });
    connect(m_opacitySlider, &QSlider::valueChanged, this, [this](int value) {
        m_opacityValue->setText(QString::number(value) + "%");
        m_scene->setStrokeOpacity(static_cast<qreal>(value) / 100.0);
        QSettings().setValue("pen/opacity", value);
    });
    connect(m_stabilizationSlider, &QSlider::valueChanged, this, [this](int value) {
        m_stabilizationValue->setText(QString::number(value) + "%");
        m_scene->setStabilization(value);
        QSettings().setValue("pen/stabilization", value);
    });

    m_scene->setStrokeWidth(width);
    m_scene->setStrokeOpacity(static_cast<qreal>(opacity) / 100.0);
    m_scene->setStabilization(stabilization);
    m_scene->setColor(QColor(settings.value("pen/color", "#202124").toString()));
    m_widthValue->setText(QString::number(width) + " px");
    m_opacityValue->setText(QString::number(opacity) + "%");
    m_stabilizationValue->setText(QString::number(stabilization) + "%");

    m_penPopupEffect = new QGraphicsOpacityEffect(m_penPopup);
    m_penPopupEffect->setOpacity(0.0);
    m_penPopup->setGraphicsEffect(m_penPopupEffect);

    m_penPopup->adjustSize();
    m_penPopup->hide();
    m_penPopup->raise();
}

void MainWindow::buildShortcuts() {
    auto addShortcut = [this](const QKeySequence &shortcut, const std::function<void()> &callback) {
        auto *action = new QAction(this);
        action->setShortcut(shortcut);
        action->setShortcutContext(Qt::WindowShortcut);
        connect(action, &QAction::triggered, this, callback);
        addAction(action);
        return action;
    };

    addShortcut(QKeySequence::New, [this] { newBoard(); });
    addShortcut(QKeySequence::Open, [this] { openBoard(); });
    addShortcut(QKeySequence::Save, [this] { saveBoard(); });
    addShortcut(QKeySequence::Undo, [this] { undo(); });

    auto *redoAction = new QAction(this);
    redoAction->setShortcuts({QKeySequence::Redo, QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Z)});
    connect(redoAction, &QAction::triggered, this, &MainWindow::redo);
    addAction(redoAction);

    addShortcut(QKeySequence::Delete, [this] { deleteSelection(); });
    addShortcut(QKeySequence::Paste, [this] { pasteFromClipboard(); });
}

QString MainWindow::boardsDirectory() const {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString path = QDir(root).filePath("boards");
    QDir().mkpath(path);
    return path;
}

QString MainWindow::thumbnailPath(const QString &path) const {
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    const QString dir = QDir(root).filePath("thumbnails");
    QDir().mkpath(dir);
    return QDir(dir).filePath(boardKey(path) + ".png");
}

QString MainWindow::boardTitle(const QString &path) const {
    if (path.isEmpty()) return tr("Pizarra sin título");
    QSettings settings;
    const QString fallback = QFileInfo(path).completeBaseName();
    return settings.value("titles/" + boardKey(path), fallback).toString();
}

void MainWindow::setBoardTitle(const QString &path, const QString &title) {
    if (path.isEmpty()) return;
    QSettings().setValue("titles/" + boardKey(path), title.trimmed().isEmpty() ? tr("Pizarra sin título") : title.trimmed());
}

void MainWindow::addRecentBoard(const QString &path) {
    if (path.isEmpty()) return;
    QSettings settings;
    QStringList recent = settings.value("recentBoards").toStringList();
    recent.removeAll(path);
    recent.prepend(path);
    while (recent.size() > 20) recent.removeLast();
    settings.setValue("recentBoards", recent);
}

void MainWindow::refreshGallery() {
    if (!m_galleryGrid) return;

    while (QLayoutItem *item = m_galleryGrid->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    int row = 0;
    int column = 0;
    constexpr int columns = 5;

    auto advance = [&] {
        ++column;
        if (column >= columns) { column = 0; ++row; }
    };

    auto *newCard = new QFrame(m_homePage);
    newCard->setObjectName("BoardCard");
    newCard->setFixedSize(260, 184);
    auto *newLayout = new QVBoxLayout(newCard);
    newLayout->setContentsMargins(0, 0, 0, 12);
    newLayout->setSpacing(4);
    auto *newButton = new QPushButton(QStringLiteral("＋"), newCard);
    newButton->setObjectName("NewBoardButton");
    newButton->setToolTip(tr("Crear una pizarra nueva"));
    connect(newButton, &QPushButton::clicked, this, &MainWindow::newBoard);
    newLayout->addWidget(newButton, 1);
    auto *newLabel = new QLabel(tr("Nueva pizarra"), newCard);
    newLabel->setObjectName("NewBoardLabel");
    newLabel->setAlignment(Qt::AlignCenter);
    newLayout->addWidget(newLabel);
    applyShadow(newCard, 24, 7, QColor(32, 30, 30, 20));
    m_galleryGrid->addWidget(newCard, row, column);
    advance();

    QStringList paths;
    QSettings settings;
    const QStringList recent = settings.value("recentBoards").toStringList();
    for (const QString &path : recent) {
        if (QFileInfo::exists(path) && !paths.contains(path)) paths.append(path);
    }

    QDir local(boardsDirectory());
    const QFileInfoList localFiles = local.entryInfoList({"*.studyboard"}, QDir::Files, QDir::Time);
    for (const QFileInfo &info : localFiles) {
        if (!paths.contains(info.absoluteFilePath())) paths.append(info.absoluteFilePath());
    }

    for (const QString &path : paths) {
        const QFileInfo info(path);
        if (!info.exists()) continue;

        auto *card = new QFrame(m_homePage);
        card->setObjectName("BoardCard");
        card->setFixedSize(260, 184);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 10);
        cardLayout->setSpacing(4);

        auto *preview = new QPushButton(card);
        preview->setObjectName("PreviewButton");
        preview->setFixedHeight(126);
        const QString thumb = thumbnailPath(path);
        if (QFileInfo::exists(thumb)) {
            preview->setIcon(QIcon(thumb));
            preview->setIconSize(QSize(248, 120));
        } else {
            preview->setText(QStringLiteral("✎"));
            preview->setStyleSheet("font-size:32px; color:#999;");
        }
        connect(preview, &QPushButton::clicked, this, [this, path] { openBoardPath(path); });
        cardLayout->addWidget(preview);

        auto *title = new QLabel(boardTitle(path), card);
        title->setObjectName("CardTitle");
        title->setContentsMargins(14, 0, 10, 0);
        title->setTextInteractionFlags(Qt::NoTextInteraction);
        cardLayout->addWidget(title);

        auto *meta = new QLabel(prettyModified(info), card);
        meta->setObjectName("CardMeta");
        meta->setContentsMargins(14, 0, 10, 0);
        cardLayout->addWidget(meta);

        applyShadow(card, 24, 7, QColor(32, 30, 30, 18));
        m_galleryGrid->addWidget(card, row, column);
        advance();
    }
}

void MainWindow::newBoard() {
    m_scene->clear();
    m_scene->setBackgroundColor(QColor("#f3f2f1"));
    m_scene->setBackgroundStyle(BoardScene::BackgroundStyle::Solid);
    m_history.clear();
    m_historyIndex = -1;
    m_dirty = false;

    const QString id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    m_filePath = QDir(boardsDirectory()).filePath(id + ".studyboard");
    setBoardTitle(m_filePath, tr("Pizarra sin título"));
    m_titleEdit->setText(tr("Pizarra sin título"));

    recordHistory();
    saveBoard();
    showBoardPage();
    setTool(static_cast<int>(BoardScene::Tool::Pen));
    m_view->resetZoom();
    updateTitle();
}

void MainWindow::openBoard() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Abrir pizarra"), {}, tr("StudyBoard (*.studyboard *.json)"));
    if (!path.isEmpty()) openBoardPath(path);
}

void MainWindow::openBoardPath(const QString &path) {
    QString error;
    m_restoring = true;
    const bool ok = BoardSerializer::loadFile(*m_scene, path, &error);
    m_restoring = false;
    if (!ok) {
        QMessageBox::critical(this, tr("No se pudo abrir"), error);
        return;
    }

    m_filePath = path;
    m_history.clear();
    m_historyIndex = -1;
    m_dirty = false;
    recordHistory();
    addRecentBoard(path);
    m_titleEdit->setText(boardTitle(path));
    m_saveState->setText(tr("Guardado"));
    showBoardPage();
    m_view->resetZoom();
    const QRectF bounds = m_scene->itemsBoundingRect();
    if (!bounds.isEmpty()) m_view->centerOn(bounds.center());
    updateTitle();
}

bool MainWindow::saveBoard() {
    if (m_filePath.isEmpty()) return saveBoardAs();

    QString error;
    if (!BoardSerializer::saveFile(*m_scene, m_filePath, &error)) {
        QMessageBox::critical(this, tr("No se pudo guardar"), error);
        m_saveState->setText(tr("Error al guardar"));
        return false;
    }

    m_dirty = false;
    m_saveState->setText(tr("Guardado"));
    addRecentBoard(m_filePath);
    saveThumbnail();
    updateTitle();
    return true;
}

bool MainWindow::saveBoardAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar pizarra"), {}, tr("StudyBoard (*.studyboard)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(".studyboard", Qt::CaseInsensitive)) path += ".studyboard";

    const QString previousTitle = m_titleEdit ? m_titleEdit->text().trimmed() : tr("Pizarra sin título");
    m_filePath = path;
    setBoardTitle(m_filePath, previousTitle);
    return saveBoard();
}

void MainWindow::exportPng() {
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    if (bounds.isEmpty()) bounds = QRectF(-800, -500, 1600, 1000);

    const qreal maxDim = 10000.0;
    const qreal scale = qMin(1.0, maxDim / qMax(bounds.width(), bounds.height()));
    const QSize size = (bounds.size() * scale).toSize().expandedTo(QSize(1, 1));

    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(m_scene->backgroundColor());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(size)), bounds);
    painter.end();

    QString path = QFileDialog::getSaveFileName(this, tr("Exportar PNG"), {}, tr("PNG (*.png)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
    image.save(path, "PNG");
}

void MainWindow::saveThumbnail() {
    if (m_filePath.isEmpty()) return;

    QImage image(QSize(480, 270), QImage::Format_ARGB32_Premultiplied);
    image.fill(m_scene->backgroundColor());
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);

    QRectF source = m_scene->itemsBoundingRect().adjusted(-35, -35, 35, 35);
    if (source.isEmpty()) source = QRectF(-800, -450, 1600, 900);
    m_scene->render(&painter, QRectF(0, 0, image.width(), image.height()), source, Qt::KeepAspectRatio);
    painter.end();
    image.save(thumbnailPath(m_filePath), "PNG");
}

void MainWindow::deleteSelection() {
    const auto selected = m_scene->selectedItems();
    if (selected.isEmpty()) return;
    for (auto *item : selected) {
        m_scene->removeItem(item);
        delete item;
    }
    m_dirty = true;
    recordHistory();
    m_autosaveTimer->start();
    updateTitle();
}

void MainWindow::recordHistory() {
    if (m_restoring || !m_scene) return;
    const QByteArray snapshot = BoardSerializer::toJson(*m_scene);
    if (m_historyIndex >= 0 && m_history.value(m_historyIndex) == snapshot) return;

    while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
    m_history.append(snapshot);
    if (m_history.size() > 50) m_history.removeFirst();
    m_historyIndex = m_history.size() - 1;
}

void MainWindow::restoreSnapshot(const QByteArray &snapshot) {
    QString error;
    m_restoring = true;
    BoardSerializer::fromJson(*m_scene, snapshot, &error);
    m_restoring = false;
    m_dirty = true;
    m_saveState->setText(tr("Guardando…"));
    m_autosaveTimer->start();
    updateTitle();
}

void MainWindow::undo() {
    if (m_historyIndex <= 0) return;
    --m_historyIndex;
    restoreSnapshot(m_history[m_historyIndex]);
}

void MainWindow::redo() {
    if (m_historyIndex + 1 >= m_history.size()) return;
    ++m_historyIndex;
    restoreSnapshot(m_history[m_historyIndex]);
}

void MainWindow::autosave() {
    if (!m_filePath.isEmpty()) {
        saveBoard();
        return;
    }

    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    BoardSerializer::saveFile(*m_scene, QDir(dir).filePath("autosave.studyboard"));
    m_saveState->setText(tr("Guardado"));
}

void MainWindow::styleButtonActive(QToolButton *button, bool active) {
    if (!button) return;
    button->setChecked(active);
    button->setProperty("toolActive", active);
    button->style()->unpolish(button);
    button->style()->polish(button);
}

void MainWindow::setTool(int tool) {
    if (!m_scene || !m_view) return;

    const auto value = static_cast<BoardScene::Tool>(tool);
    m_scene->setTool(value);
    m_view->setSelectionMode(value == BoardScene::Tool::Select);

    for (auto it = m_toolButtons.begin(); it != m_toolButtons.end(); ++it)
        styleButtonActive(it.value(), it.key() == tool);

    if (value != BoardScene::Tool::Pen && value != BoardScene::Tool::Highlighter)
        animatePopup(m_penPopup, m_penPopupEffect, false, m_penPopup->geometry());
}

void MainWindow::animatePopup(QFrame *popup, QGraphicsOpacityEffect *effect, bool show, const QRect &finalGeometry) {
    if (!popup || !effect) return;

    auto *group = new QParallelAnimationGroup(popup);

    QRect startGeom = finalGeometry;
    startGeom.translate(0, show ? 12 : 0);
    QRect endGeom = finalGeometry;
    endGeom.translate(0, show ? 0 : 12);

    auto *geoAnim = new QPropertyAnimation(popup, "geometry", group);
    geoAnim->setDuration(180);
    geoAnim->setStartValue(show ? startGeom : finalGeometry);
    geoAnim->setEndValue(show ? finalGeometry : endGeom);
    geoAnim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(geoAnim);

    auto *opacityAnim = new QPropertyAnimation(effect, "opacity", group);
    opacityAnim->setDuration(160);
    opacityAnim->setStartValue(show ? 0.0 : 1.0);
    opacityAnim->setEndValue(show ? 1.0 : 0.0);
    opacityAnim->setEasingCurve(QEasingCurve::OutCubic);
    group->addAnimation(opacityAnim);

    if (show) {
        popup->setGeometry(startGeom);
        popup->show();
        popup->raise();
    } else {
        connect(group, &QParallelAnimationGroup::finished, popup, [popup] { popup->hide(); });
    }

    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::showPenPopup() {
    if (!m_penPopup || !m_penPopupEffect) return;
    repositionOverlays();
    const bool show = !m_penPopup->isVisible();
    animatePopup(m_penPopup, m_penPopupEffect, show, m_penPopup->geometry());
}

void MainWindow::showShapesMenu() {
    QMenu menu(this);
    menu.addAction(themedIcon({"draw-rectangle"}), tr("Rectángulo"), [this] { setTool(static_cast<int>(BoardScene::Tool::Rectangle)); });
    menu.addAction(themedIcon({"draw-ellipse"}), tr("Elipse"), [this] { setTool(static_cast<int>(BoardScene::Tool::Ellipse)); });
    menu.addAction(themedIcon({"draw-line"}), tr("Línea"), [this] { setTool(static_cast<int>(BoardScene::Tool::Line)); });

    const QPoint pos = m_shapesButton->mapToGlobal(QPoint(0, -menu.sizeHint().height() - 8));
    menu.exec(pos);
}

void MainWindow::showMoreMenu() {
    QMenu menu(this);
    QAction *save = menu.addAction(themedIcon({"document-save-symbolic", "document-save"}, style()->standardIcon(QStyle::SP_DialogSaveButton)), tr("Guardar ahora\tCtrl+S"));
    QAction *saveAs = menu.addAction(tr("Guardar como…"));
    QAction *open = menu.addAction(themedIcon({"document-open-symbolic", "document-open"}, style()->standardIcon(QStyle::SP_DialogOpenButton)), tr("Abrir…\tCtrl+O"));
    menu.addSeparator();

    QMenu *backgroundColorMenu = menu.addMenu(tr("Color de fondo"));
    struct ColorOption { const char *name; const char *hex; };
    const QList<ColorOption> colors = {
        {"Blanco cálido", "#f3f2f1"},
        {"Blanco", "#ffffff"},
        {"Azul suave", "#eef4ff"},
        {"Rosa suave", "#fff3f6"},
        {"Menta", "#eefaf5"},
        {"Lavanda", "#f5f1ff"}
    };
    for (const auto &opt : colors) {
        backgroundColorMenu->addAction(opt.name, [this, opt] { m_scene->setBackgroundColor(QColor(opt.hex)); });
    }

    QMenu *backgroundStyleMenu = menu.addMenu(tr("Patrón de fondo"));
    backgroundStyleMenu->addAction(tr("Sólido"), [this] { m_scene->setBackgroundStyle(BoardScene::BackgroundStyle::Solid); });
    backgroundStyleMenu->addAction(tr("Puntos"), [this] { m_scene->setBackgroundStyle(BoardScene::BackgroundStyle::Dots); });
    backgroundStyleMenu->addAction(tr("Cuadrícula"), [this] { m_scene->setBackgroundStyle(BoardScene::BackgroundStyle::Grid); });
    backgroundStyleMenu->addAction(tr("Regla"), [this] { m_scene->setBackgroundStyle(BoardScene::BackgroundStyle::Ruled); });

    QAction *exportAction = menu.addAction(tr("Exportar PNG…"));
    menu.addSeparator();
    QAction *home = menu.addAction(tr("Mis pizarras"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == save) saveBoard();
    else if (chosen == saveAs) saveBoardAs();
    else if (chosen == open) openBoard();
    else if (chosen == exportAction) exportPng();
    else if (chosen == home) showHome();
}

void MainWindow::pasteFromClipboard() {
    if (m_pages->currentWidget() != m_boardPage) return;
    const QPoint viewportCenter = m_view->viewport()->rect().center();
    const QPointF scenePos = m_view->mapToScene(viewportCenter);
    m_scene->pasteImageAt(scenePos);
}

void MainWindow::renameCurrentBoard() {
    if (m_filePath.isEmpty()) return;
    QString title = m_titleEdit->text().trimmed();
    if (title.isEmpty()) title = tr("Pizarra sin título");
    m_titleEdit->setText(title);
    setBoardTitle(m_filePath, title);
    updateTitle();
}

void MainWindow::showHome() {
    if (m_dirty) saveBoard();
    if (m_penPopup && m_penPopup->isVisible())
        animatePopup(m_penPopup, m_penPopupEffect, false, m_penPopup->geometry());
    refreshGallery();
    m_pages->setCurrentWidget(m_homePage);
    setWindowTitle("StudyBoard");
}

void MainWindow::showBoardPage() {
    m_pages->setCurrentWidget(m_boardPage);
    repositionOverlays();
    m_bottomBar->raise();
    m_zoomBar->raise();
    updateTitle();
}

void MainWindow::updateTitle() {
    if (m_pages && m_pages->currentWidget() == m_homePage) {
        setWindowTitle("StudyBoard");
        return;
    }
    const QString name = m_filePath.isEmpty() ? tr("Pizarra sin título") : boardTitle(m_filePath);
    setWindowTitle(QString("%1%2 — StudyBoard").arg(name, m_dirty ? " *" : ""));
}

void MainWindow::repositionOverlays() {
    if (!m_boardPage || !m_bottomBar || !m_zoomBar) return;

    m_bottomBar->adjustSize();
    const QSize bottomSize = m_bottomBar->sizeHint();
    const int bottomX = qMax(12, (m_boardPage->width() - bottomSize.width()) / 2);
    const int bottomY = qMax(60, m_boardPage->height() - bottomSize.height() - 18);
    m_bottomBar->setGeometry(bottomX, bottomY, bottomSize.width(), bottomSize.height());

    m_zoomBar->adjustSize();
    const QSize zoomSize = m_zoomBar->sizeHint();
    m_zoomBar->setGeometry(qMax(12, m_boardPage->width() - zoomSize.width() - 18),
                           qMax(60, m_boardPage->height() - zoomSize.height() - 18),
                           zoomSize.width(), zoomSize.height());

    if (m_penPopup) {
        m_penPopup->adjustSize();
        const QSize popupSize = m_penPopup->sizeHint();
        const int popupX = qMax(12, bottomX + (bottomSize.width() - popupSize.width()) / 2);
        const int popupY = qMax(58, bottomY - popupSize.height() - 10);
        m_penPopup->setGeometry(popupX, popupY, popupSize.width(), popupSize.height());
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    repositionOverlays();
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (m_dirty) saveBoard();
    QMainWindow::closeEvent(event);
}
