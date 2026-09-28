#include "MainWindow.h"
#include "BoardScene.h"
#include "BoardSerializer.h"
#include "BoardView.h"

#include <QAction>
#include <functional>
#include <QApplication>
#include <QCloseEvent>
#include <QCryptographicHash>
#include <QCursor>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QFrame>
#include <QGraphicsItem>
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
#include <QPushButton>
#include <QStyle>
#include <QResizeEvent>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QStatusBar>
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
    resize(1440, 900);
    setMinimumSize(900, 600);
    setWindowTitle("StudyBoard");
    menuBar()->hide();
    statusBar()->hide();

    setStyleSheet(R"(
        QMainWindow { background: #f3f3f4; }
        QWidget { font-family: "Inter", "Segoe UI", "Noto Sans", sans-serif; }
        QFrame#TopBar { background: #ffffff; border-bottom: 1px solid #e7e7e9; }
        QFrame#FloatingBar, QFrame#ZoomBar, QFrame#PenPopup {
            background: #ffffff;
            border: 1px solid #e3e3e6;
            border-radius: 12px;
        }
        QToolButton {
            background: transparent;
            border: none;
            border-radius: 8px;
            color: #171719;
            padding: 7px;
            font-size: 15px;
        }
        QToolButton:hover { background: #f0f0f2; }
        QToolButton[toolActive="true"] { background: #e8e8eb; }
        QToolButton#PrimaryButton { background: #1f5eff; color: white; }
        QToolButton#PrimaryButton:hover { background: #154fdf; }
        QLineEdit#BoardTitle {
            background: transparent;
            border: none;
            color: #161618;
            font-weight: 600;
            font-size: 15px;
            padding: 5px 7px;
        }
        QLineEdit#BoardTitle:focus { background: #f5f5f7; border-radius: 7px; }
        QLabel#SaveState { color: #77777d; font-size: 12px; }
        QLabel#HomeTitle { color: #ffffff; font-weight: 700; font-size: 18px; }
        QFrame#HomeHeader { background: #484746; border: none; }
        QScrollArea { background: #eeeeef; border: none; }
        QWidget#GalleryCanvas { background: #eeeeef; }
        QFrame#BoardCard {
            background: #ffffff;
            border: 1px solid #dcdce0;
            border-radius: 10px;
        }
        QPushButton#PreviewButton {
            background: #fafafa;
            border: none;
            border-top-left-radius: 9px;
            border-top-right-radius: 9px;
            text-align: center;
        }
        QPushButton#NewBoardButton {
            background: #1d43ff;
            color: #ffffff;
            border: none;
            border-radius: 10px;
            font-size: 54px;
        }
        QPushButton#NewBoardButton:hover { background: #1738d8; }
        QLabel#CardTitle { color: #19191b; font-weight: 650; font-size: 14px; }
        QLabel#CardMeta { color: #7a7a80; font-size: 11px; }
        QSlider::groove:horizontal { height: 4px; background: #d8d8dc; border-radius: 2px; }
        QSlider::handle:horizontal { width: 14px; margin: -5px 0; background: #ffffff; border: 1px solid #777; border-radius: 7px; }
        QSlider::sub-page:horizontal { background: #2c2c31; border-radius: 2px; }
        QMenu { background: #ffffff; color: #1b1b1e; border: 1px solid #dedee2; padding: 6px; }
        QMenu::item { padding: 8px 24px 8px 12px; border-radius: 6px; }
        QMenu::item:selected { background: #f0f0f3; }
        QToolTip { background: #343438; color: white; border: none; padding: 5px; }
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
    header->setFixedHeight(48);
    auto *headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 18, 0);

    auto *appTitle = new QLabel(tr("StudyBoard"), header);
    appTitle->setObjectName("HomeTitle");
    headerLayout->addWidget(appTitle);
    headerLayout->addStretch();

    auto *open = new QToolButton(header);
    open->setText(tr("Abrir archivo"));
    open->setStyleSheet("color:white; padding:6px 12px;");
    connect(open, &QToolButton::clicked, this, &MainWindow::openBoard);
    headerLayout->addWidget(open);
    layout->addWidget(header);

    auto *scroll = new QScrollArea(m_homePage);
    scroll->setWidgetResizable(true);
    auto *canvas = new QWidget(scroll);
    canvas->setObjectName("GalleryCanvas");
    m_galleryGrid = new QGridLayout(canvas);
    m_galleryGrid->setContentsMargins(36, 28, 36, 36);
    m_galleryGrid->setHorizontalSpacing(20);
    m_galleryGrid->setVerticalSpacing(20);
    m_galleryGrid->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    scroll->setWidget(canvas);
    layout->addWidget(scroll, 1);

    m_pages->addWidget(m_homePage);
}

QToolButton *MainWindow::createToolbarButton(const QString &text, const QString &tooltip, bool checkable) {
    auto *button = new QToolButton(m_bottomBar);
    button->setText(text);
    button->setToolTip(tooltip);
    button->setCheckable(checkable);
    button->setFixedSize(38, 38);
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
    topBar->setFixedHeight(48);
    auto *topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(10, 0, 12, 0);
    topLayout->setSpacing(6);

    auto *homeButton = new QToolButton(topBar);
    homeButton->setText(QStringLiteral("⌂"));
    homeButton->setToolTip(tr("Volver a mis pizarras"));
    homeButton->setFixedSize(36, 36);
    connect(homeButton, &QToolButton::clicked, this, &MainWindow::showHome);
    topLayout->addWidget(homeButton);

    auto *separator = new QFrame(topBar);
    separator->setFrameShape(QFrame::VLine);
    separator->setStyleSheet("color:#dddddf;");
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
    exportButton->setToolTip(tr("Exportar la pizarra como imagen"));
    connect(exportButton, &QToolButton::clicked, this, &MainWindow::exportPng);
    topLayout->addWidget(exportButton);

    auto *settingsButton = new QToolButton(topBar);
    settingsButton->setText(QStringLiteral("⚙"));
    settingsButton->setToolTip(tr("Opciones"));
    settingsButton->setFixedSize(36, 36);
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
    tools->setContentsMargins(7, 6, 7, 6);
    tools->setSpacing(3);

    auto *undoButton = createToolbarButton(QStringLiteral("↶"), tr("Deshacer (Ctrl+Z)"));
    connect(undoButton, &QToolButton::clicked, this, &MainWindow::undo);
    tools->addWidget(undoButton);

    auto makeTool = [this, tools](const QString &text, const QString &tip, BoardScene::Tool tool) {
        auto *button = createToolbarButton(text, tip, true);
        const int value = static_cast<int>(tool);
        m_toolButtons.insert(value, button);
        connect(button, &QToolButton::clicked, this, [this, value] { setTool(value); });
        tools->addWidget(button);
        return button;
    };

    makeTool(QStringLiteral("➤"), tr("Seleccionar"), BoardScene::Tool::Select);
    makeTool(QStringLiteral("✋"), tr("Mover lienzo (o mantén Espacio)"), BoardScene::Tool::Pan);

    m_penButton = createToolbarButton(QStringLiteral("✎"), tr("Lápiz"), true);
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Pen), m_penButton);
    connect(m_penButton, &QToolButton::clicked, this, [this] {
        if (m_scene->tool() == BoardScene::Tool::Pen && m_penButton->property("toolActive").toBool())
            showPenPopup();
        else
            setTool(static_cast<int>(BoardScene::Tool::Pen));
    });
    tools->addWidget(m_penButton);

    m_highlighterButton = createToolbarButton(QStringLiteral("▰"), tr("Resaltador"), true);
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Highlighter), m_highlighterButton);
    connect(m_highlighterButton, &QToolButton::clicked, this, [this] {
        if (m_scene->tool() == BoardScene::Tool::Highlighter && m_highlighterButton->property("toolActive").toBool())
            showPenPopup();
        else
            setTool(static_cast<int>(BoardScene::Tool::Highlighter));
    });
    tools->addWidget(m_highlighterButton);

    makeTool(QStringLiteral("⌫"), tr("Borrador"), BoardScene::Tool::Eraser);
    makeTool(QStringLiteral("▣"), tr("Nota adhesiva"), BoardScene::Tool::StickyNote);
    makeTool(QStringLiteral("T"), tr("Texto"), BoardScene::Tool::Text);

    m_shapesButton = createToolbarButton(QStringLiteral("○□"), tr("Formas"));
    connect(m_shapesButton, &QToolButton::clicked, this, &MainWindow::showShapesMenu);
    tools->addWidget(m_shapesButton);

    auto *imageButton = createToolbarButton(QStringLiteral("▧"), tr("Imagen"), true);
    m_toolButtons.insert(static_cast<int>(BoardScene::Tool::Image), imageButton);
    connect(imageButton, &QToolButton::clicked, this,
            [this] { setTool(static_cast<int>(BoardScene::Tool::Image)); });
    tools->addWidget(imageButton);

    auto *moreButton = createToolbarButton(QStringLiteral("⋯"), tr("Más opciones"));
    connect(moreButton, &QToolButton::clicked, this, &MainWindow::showMoreMenu);
    tools->addWidget(moreButton);

    m_bottomBar->adjustSize();
    m_bottomBar->raise();

    m_zoomBar = new QFrame(m_boardPage);
    m_zoomBar->setObjectName("ZoomBar");
    auto *zoomLayout = new QHBoxLayout(m_zoomBar);
    zoomLayout->setContentsMargins(7, 5, 7, 5);
    zoomLayout->setSpacing(2);

    auto *minus = new QToolButton(m_zoomBar);
    minus->setText(QStringLiteral("−"));
    minus->setFixedSize(32, 32);
    connect(minus, &QToolButton::clicked, this, [this] { m_view->zoomBy(1.0 / 1.15); });
    zoomLayout->addWidget(minus);

    m_zoomLabel = new QLabel("100%", m_zoomBar);
    m_zoomLabel->setAlignment(Qt::AlignCenter);
    m_zoomLabel->setMinimumWidth(50);
    zoomLayout->addWidget(m_zoomLabel);

    auto *plus = new QToolButton(m_zoomBar);
    plus->setText(QStringLiteral("+"));
    plus->setFixedSize(32, 32);
    connect(plus, &QToolButton::clicked, this, [this] { m_view->zoomBy(1.15); });
    zoomLayout->addWidget(plus);

    auto *reset = new QToolButton(m_zoomBar);
    reset->setText(QStringLiteral("⌗"));
    reset->setToolTip(tr("Restablecer zoom"));
    reset->setFixedSize(32, 32);
    connect(reset, &QToolButton::clicked, m_view, &BoardView::resetZoom);
    zoomLayout->addWidget(reset);

    connect(m_view, &BoardView::zoomChanged, this, [this](int percent) {
        m_zoomLabel->setText(QString::number(percent) + "%");
    });
    m_zoomBar->adjustSize();
    m_zoomBar->raise();

    buildPenPopup();
    setTool(static_cast<int>(BoardScene::Tool::Pen));

    m_pages->addWidget(m_boardPage);
}

void MainWindow::buildPenPopup() {
    m_penPopup = new QFrame(m_boardPage);
    m_penPopup->setObjectName("PenPopup");
    m_penPopup->setFixedWidth(290);
    auto *layout = new QVBoxLayout(m_penPopup);
    layout->setContentsMargins(15, 14, 15, 14);
    layout->setSpacing(10);

    auto addSliderRow = [this, layout](const QString &name, int min, int max, int value,
                                       QSlider **sliderOut, QLabel **valueOut) {
        auto *titleRow = new QHBoxLayout();
        auto *nameLabel = new QLabel(name, m_penPopup);
        auto *valueLabel = new QLabel(QString::number(value), m_penPopup);
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

    auto *colors = new QHBoxLayout();
    colors->setSpacing(7);
    const QStringList palette = {
        "#1f1f22", "#e51c2a", "#ff9f1a", "#10a65a",
        "#36bbed", "#0a6fc2", "#ca1460", "#7142a1"
    };
    for (const QString &hex : palette) {
        auto *color = new QToolButton(m_penPopup);
        color->setFixedSize(26, 26);
        color->setToolTip(hex);
        color->setStyleSheet(QString(
            "QToolButton { background:%1; border:2px solid white; border-radius:13px; }"
            "QToolButton:hover { border:2px solid #777; }").arg(hex));
        connect(color, &QToolButton::clicked, this, [this, hex] {
            m_scene->setColor(QColor(hex));
            QSettings().setValue("pen/color", hex);
        });
        colors->addWidget(color);
    }
    layout->addLayout(colors);

    connect(m_widthSlider, &QSlider::valueChanged, this, [this](int value) {
        m_widthValue->setText(QString::number(value));
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
    m_scene->setColor(QColor(settings.value("pen/color", "#1f1f22").toString()));

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
    newCard->setFixedSize(250, 175);
    auto *newLayout = new QVBoxLayout(newCard);
    newLayout->setContentsMargins(0, 0, 0, 0);
    auto *newButton = new QPushButton(QStringLiteral("+"), newCard);
    newButton->setObjectName("NewBoardButton");
    newButton->setToolTip(tr("Crear una pizarra nueva"));
    connect(newButton, &QPushButton::clicked, this, &MainWindow::newBoard);
    newLayout->addWidget(newButton, 1);
    auto *newLabel = new QLabel(tr("Nueva pizarra"), newCard);
    newLabel->setAlignment(Qt::AlignCenter);
    newLabel->setStyleSheet("background:#1d43ff; color:white; font-size:14px; padding:0 0 12px 0;");
    newLayout->addWidget(newLabel);
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
        card->setFixedSize(250, 175);
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(0, 0, 0, 8);
        cardLayout->setSpacing(4);

        auto *preview = new QPushButton(card);
        preview->setObjectName("PreviewButton");
        preview->setFixedHeight(118);
        const QString thumb = thumbnailPath(path);
        if (QFileInfo::exists(thumb)) {
            preview->setIcon(QIcon(thumb));
            preview->setIconSize(QSize(238, 112));
        } else {
            preview->setText(QStringLiteral("✎"));
            preview->setStyleSheet("font-size:32px; color:#999;");
        }
        connect(preview, &QPushButton::clicked, this, [this, path] { openBoardPath(path); });
        cardLayout->addWidget(preview);

        auto *title = new QLabel(boardTitle(path), card);
        title->setObjectName("CardTitle");
        title->setContentsMargins(12, 0, 10, 0);
        title->setTextInteractionFlags(Qt::NoTextInteraction);
        cardLayout->addWidget(title);

        auto *meta = new QLabel(prettyModified(info), card);
        meta->setObjectName("CardMeta");
        meta->setContentsMargins(12, 0, 10, 0);
        cardLayout->addWidget(meta);

        m_galleryGrid->addWidget(card, row, column);
        advance();
    }
}

void MainWindow::newBoard() {
    m_scene->clear();
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
    image.fill(QColor("#f5f5f6"));
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
    image.fill(QColor("#f5f5f6"));
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

void MainWindow::setTool(int tool) {
    if (!m_scene || !m_view) return;

    const auto value = static_cast<BoardScene::Tool>(tool);
    m_scene->setTool(value);
    m_view->setSelectionMode(value == BoardScene::Tool::Select);

    for (auto it = m_toolButtons.begin(); it != m_toolButtons.end(); ++it) {
        const bool active = it.key() == tool;
        it.value()->setChecked(active);
        it.value()->setProperty("toolActive", active);
        it.value()->style()->unpolish(it.value());
        it.value()->style()->polish(it.value());
    }

    if (value != BoardScene::Tool::Pen && value != BoardScene::Tool::Highlighter)
        m_penPopup->hide();
}

void MainWindow::showPenPopup() {
    if (!m_penPopup) return;
    m_penPopup->setVisible(!m_penPopup->isVisible());
    if (m_penPopup->isVisible()) {
        repositionOverlays();
        m_penPopup->raise();
    }
}

void MainWindow::showShapesMenu() {
    QMenu menu(this);
    QAction *rectangle = menu.addAction(tr("▭  Rectángulo"));
    QAction *ellipse = menu.addAction(tr("○  Elipse"));
    QAction *line = menu.addAction(tr("╱  Línea"));

    const QPoint pos = m_shapesButton->mapToGlobal(QPoint(0, -menu.sizeHint().height() - 8));
    QAction *chosen = menu.exec(pos);
    if (chosen == rectangle) setTool(static_cast<int>(BoardScene::Tool::Rectangle));
    if (chosen == ellipse) setTool(static_cast<int>(BoardScene::Tool::Ellipse));
    if (chosen == line) setTool(static_cast<int>(BoardScene::Tool::Line));
}

void MainWindow::showMoreMenu() {
    QMenu menu(this);
    QAction *save = menu.addAction(tr("Guardar ahora\tCtrl+S"));
    QAction *saveAs = menu.addAction(tr("Guardar como…"));
    QAction *open = menu.addAction(tr("Abrir…\tCtrl+O"));
    menu.addSeparator();
    QAction *grid = menu.addAction(tr("Cuadrícula"));
    grid->setCheckable(true);
    grid->setChecked(m_scene->gridVisible());
    QAction *exportAction = menu.addAction(tr("Exportar PNG…"));
    menu.addSeparator();
    QAction *home = menu.addAction(tr("Mis pizarras"));

    QAction *chosen = menu.exec(QCursor::pos());
    if (chosen == save) saveBoard();
    else if (chosen == saveAs) saveBoardAs();
    else if (chosen == open) openBoard();
    else if (chosen == grid) m_scene->setGridVisible(grid->isChecked());
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
    m_penPopup->hide();
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
