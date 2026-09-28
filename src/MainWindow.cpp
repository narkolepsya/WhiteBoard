#include "MainWindow.h"
#include "BoardScene.h"
#include "BoardSerializer.h"
#include "BoardView.h"

#include <QAction>
#include <QApplication>
#include <QColorDialog>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QGraphicsItem>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QTimer>
#include <QToolBar>
#include <QToolButton>

namespace {
QAction *toolAction(QToolBar *bar, const QString &text, BoardScene::Tool tool, MainWindow *owner) {
    auto *a = bar->addAction(text);
    a->setCheckable(true);
    a->setData(static_cast<int>(tool));
    QObject::connect(a, &QAction::triggered, owner, [owner, a] { owner->setTool(a->data().toInt()); });
    return a;
}
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    buildUi();
    recordHistory();

    m_autosaveTimer = new QTimer(this);
    m_autosaveTimer->setInterval(1500);
    m_autosaveTimer->setSingleShot(true);
    connect(m_autosaveTimer, &QTimer::timeout, this, &MainWindow::autosave);
    connect(m_scene, &BoardScene::contentChanged, this, [this] {
        m_dirty = true;
        recordHistory();
        m_autosaveTimer->start();
        updateTitle();
    });
}

void MainWindow::buildUi() {
    resize(1400, 900);
    setWindowTitle("StudyBoard");

    m_scene = new BoardScene(this);
    m_view = new BoardView(m_scene, this);
    setCentralWidget(m_view);
    connect(m_scene, &BoardScene::panRequested, m_view, &BoardView::setPanMode);

    auto *fileMenu = menuBar()->addMenu(tr("Archivo"));
    auto *newAct = fileMenu->addAction(tr("Nuevo"));
    newAct->setShortcut(QKeySequence::New);
    connect(newAct, &QAction::triggered, this, &MainWindow::newBoard);
    auto *openAct = fileMenu->addAction(tr("Abrir…"));
    openAct->setShortcut(QKeySequence::Open);
    connect(openAct, &QAction::triggered, this, &MainWindow::openBoard);
    auto *saveAct = fileMenu->addAction(tr("Guardar"));
    saveAct->setShortcut(QKeySequence::Save);
    connect(saveAct, &QAction::triggered, this, &MainWindow::saveBoard);
    auto *saveAsAct = fileMenu->addAction(tr("Guardar como…"));
    saveAsAct->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAct, &QAction::triggered, this, &MainWindow::saveBoardAs);
    fileMenu->addSeparator();
    auto *exportAct = fileMenu->addAction(tr("Exportar PNG…"));
    connect(exportAct, &QAction::triggered, this, &MainWindow::exportPng);

    auto *editMenu = menuBar()->addMenu(tr("Editar"));
    auto *undoAct = editMenu->addAction(tr("Deshacer"));
    undoAct->setShortcut(QKeySequence::Undo);
    connect(undoAct, &QAction::triggered, this, &MainWindow::undo);
    auto *redoAct = editMenu->addAction(tr("Rehacer"));
    redoAct->setShortcut(QKeySequence::Redo);
    connect(redoAct, &QAction::triggered, this, &MainWindow::redo);
    auto *deleteAct = editMenu->addAction(tr("Eliminar selección"));
    deleteAct->setShortcut(QKeySequence::Delete);
    connect(deleteAct, &QAction::triggered, this, &MainWindow::deleteSelection);

    auto *viewMenu = menuBar()->addMenu(tr("Vista"));
    auto *gridAct = viewMenu->addAction(tr("Cuadrícula"));
    gridAct->setCheckable(true);
    connect(gridAct, &QAction::toggled, m_scene, &BoardScene::setGridVisible);
    auto *resetZoomAct = viewMenu->addAction(tr("Restablecer zoom"));
    connect(resetZoomAct, &QAction::triggered, m_view, &BoardView::resetZoom);

    auto *toolbar = addToolBar(tr("Herramientas"));
    toolbar->setMovable(false);
    toolbar->setToolButtonStyle(Qt::ToolButtonTextOnly);

    auto *select = toolAction(toolbar, tr("Seleccionar"), BoardScene::Tool::Select, this);
    auto *pan = toolAction(toolbar, tr("Mover"), BoardScene::Tool::Pan, this);
    auto *pen = toolAction(toolbar, tr("Lápiz"), BoardScene::Tool::Pen, this);
    auto *high = toolAction(toolbar, tr("Resaltador"), BoardScene::Tool::Highlighter, this);
    auto *erase = toolAction(toolbar, tr("Borrador"), BoardScene::Tool::Eraser, this);
    auto *text = toolAction(toolbar, tr("Texto"), BoardScene::Tool::Text, this);
    auto *sticky = toolAction(toolbar, tr("Nota"), BoardScene::Tool::StickyNote, this);
    auto *rect = toolAction(toolbar, tr("Rectángulo"), BoardScene::Tool::Rectangle, this);
    auto *ellipse = toolAction(toolbar, tr("Elipse"), BoardScene::Tool::Ellipse, this);
    auto *line = toolAction(toolbar, tr("Línea"), BoardScene::Tool::Line, this);
    auto *image = toolAction(toolbar, tr("Imagen"), BoardScene::Tool::Image, this);
    Q_UNUSED(select); Q_UNUSED(pan); Q_UNUSED(high); Q_UNUSED(erase); Q_UNUSED(text);
    Q_UNUSED(sticky); Q_UNUSED(rect); Q_UNUSED(ellipse); Q_UNUSED(line); Q_UNUSED(image);
    pen->setChecked(true);

    toolbar->addSeparator();
    auto *colorButton = new QToolButton(toolbar);
    colorButton->setText(tr("Color"));
    connect(colorButton, &QToolButton::clicked, this, &MainWindow::chooseColor);
    toolbar->addWidget(colorButton);

    m_colorSwatch = new QLabel(toolbar);
    m_colorSwatch->setFixedSize(22, 22);
    m_colorSwatch->setStyleSheet("background:#111827; border:1px solid #999; border-radius:4px;");
    toolbar->addWidget(m_colorSwatch);

    toolbar->addWidget(new QLabel(tr("  Grosor: "), toolbar));
    m_widthSpin = new QDoubleSpinBox(toolbar);
    m_widthSpin->setRange(1.0, 30.0);
    m_widthSpin->setValue(3.0);
    m_widthSpin->setSingleStep(1.0);
    connect(m_widthSpin, &QDoubleSpinBox::valueChanged, m_scene, &BoardScene::setStrokeWidth);
    toolbar->addWidget(m_widthSpin);

    statusBar()->showMessage(tr("Rueda: zoom · Espacio: mover lienzo · Ctrl+S: guardar"));
}

void MainWindow::setTool(int tool) {
    const auto value = static_cast<BoardScene::Tool>(tool);
    m_scene->setTool(value);
    for (auto *bar : findChildren<QToolBar *>()) {
        for (auto *action : bar->actions()) {
            if (action->isCheckable() && action->data().isValid())
                action->setChecked(action->data().toInt() == tool);
        }
    }
}

void MainWindow::newBoard() {
    m_scene->clear();
    m_filePath.clear();
    m_history.clear();
    m_historyIndex = -1;
    m_dirty = false;
    recordHistory();
    updateTitle();
}

void MainWindow::openBoard() {
    const QString path = QFileDialog::getOpenFileName(this, tr("Abrir pizarra"), {},
                                                      tr("StudyBoard (*.studyboard *.json)"));
    if (path.isEmpty()) return;
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
    updateTitle();
}

bool MainWindow::saveBoard() {
    if (m_filePath.isEmpty()) return saveBoardAs();
    QString error;
    if (!BoardSerializer::saveFile(*m_scene, m_filePath, &error)) {
        QMessageBox::critical(this, tr("No se pudo guardar"), error);
        return false;
    }
    m_dirty = false;
    updateTitle();
    return true;
}

bool MainWindow::saveBoardAs() {
    QString path = QFileDialog::getSaveFileName(this, tr("Guardar pizarra"), {}, tr("StudyBoard (*.studyboard)"));
    if (path.isEmpty()) return false;
    if (!path.endsWith(".studyboard", Qt::CaseInsensitive)) path += ".studyboard";
    m_filePath = path;
    return saveBoard();
}

void MainWindow::exportPng() {
    QRectF bounds = m_scene->itemsBoundingRect().adjusted(-40, -40, 40, 40);
    if (bounds.isEmpty()) bounds = QRectF(-800, -500, 1600, 1000);
    const qreal maxDim = 10000.0;
    const qreal scale = qMin(1.0, maxDim / qMax(bounds.width(), bounds.height()));
    QSize size = (bounds.size() * scale).toSize().expandedTo(QSize(1, 1));
    QImage image(size, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::white);
    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    m_scene->render(&painter, QRectF(QPointF(0, 0), QSizeF(size)), bounds);
    painter.end();

    QString path = QFileDialog::getSaveFileName(this, tr("Exportar PNG"), {}, tr("PNG (*.png)"));
    if (path.isEmpty()) return;
    if (!path.endsWith(".png", Qt::CaseInsensitive)) path += ".png";
    image.save(path, "PNG");
}

void MainWindow::chooseColor() {
    const QColor color = QColorDialog::getColor(m_scene->color(), this, tr("Color del lápiz"));
    if (!color.isValid()) return;
    m_scene->setColor(color);
    m_colorSwatch->setStyleSheet(QString("background:%1; border:1px solid #999; border-radius:4px;").arg(color.name()));
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
    updateTitle();
}

void MainWindow::recordHistory() {
    if (m_restoring) return;
    const QByteArray snapshot = BoardSerializer::toJson(*m_scene);
    if (m_historyIndex >= 0 && m_history.value(m_historyIndex) == snapshot) return;
    while (m_history.size() > m_historyIndex + 1) m_history.removeLast();
    m_history.append(snapshot);
    if (m_history.size() > 80) m_history.removeFirst();
    m_historyIndex = m_history.size() - 1;
}

void MainWindow::restoreSnapshot(const QByteArray &snapshot) {
    QString error;
    m_restoring = true;
    BoardSerializer::fromJson(*m_scene, snapshot, &error);
    m_restoring = false;
    m_dirty = true;
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
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dir);
    BoardSerializer::saveFile(*m_scene, QDir(dir).filePath("autosave.studyboard"));
    if (!m_filePath.isEmpty()) saveBoard();
}

void MainWindow::updateTitle() {
    const QString name = m_filePath.isEmpty() ? tr("Sin título") : QFileInfo(m_filePath).completeBaseName();
    setWindowTitle(QString("%1%2 — StudyBoard").arg(name, m_dirty ? " *" : ""));
}
