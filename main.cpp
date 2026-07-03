// ============================================================================
// Phoenix-Filemanager - VERSION 1.14.0 (PRODUCTION UNIFIED BUILD - UI HARMONIZED)
// ============================================================================
// v1.14.0 - Command Matrix Integration, Dual-Engine Drop Stack, Polished UI
// ============================================================================

#include <QApplication>
#include <QMainWindow>
#include <QSplitter>
#include <QTreeView>
#include <QListView>
#include <QFileSystemModel>
#include <QStandardItemModel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QHeaderView>
#include <QDir>
#include <QMenuBar>
#include <QMenu>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QMessageBox>
#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QListWidget>
#include <QSortFilterProxyModel>
#include <QDateTime>
#include <QTimer>
#include <QProcess>
#include <QFileInfo>
#include <QFocusEvent>
#include <QMap>
#include <QSettings>
#include <QFile>
#include <QTableWidget>
#include <QItemSelectionModel>
#include <QPixmap>
#include <QTextStream>
#include <QClipboard>
#include <QMimeData>
#include <QDesktopServices>
#include <QUrl>
#include <QTabWidget>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QAudioOutput>
#include <QStandardPaths>
#include <QCryptographicHash>

// ============================================================================
// FEATURE: GLOBAL PERSISTENCE REPOSITORY
// ============================================================================
static QMap<QString, QStringList> g_virtualCollections;
static QMap<QString, QString> g_bookmarks;

void loadStorage() {
    g_virtualCollections.clear();
    QSettings collSettings("Phoenix-Filemanager", "CollectionsEngine");
    collSettings.beginGroup("VirtualCollections");
    QStringList collKeys = collSettings.childKeys();
    if (collKeys.isEmpty()) {
        g_virtualCollections["Project-Homework"] = QStringList();
        g_virtualCollections["Media-Assets"] = QStringList();
    } else {
        for (const QString &key : collKeys) g_virtualCollections[key] = collSettings.value(key).toStringList();
    }
    collSettings.endGroup();

    g_bookmarks.clear();
    QSettings bmSettings("Phoenix-Filemanager", "BookmarksEngine");
    bmSettings.beginGroup("Bookmarks");
    QStringList bmKeys = bmSettings.childKeys();
    if (bmKeys.isEmpty()) {
        g_bookmarks["🏠 Home"] = QDir::homePath();
        g_bookmarks["📥 Downloads"] = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
        g_bookmarks["📄 Documents"] = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    } else {
        for (const QString &key : bmKeys) g_bookmarks[key] = bmSettings.value(key).toString();
    }
    bmSettings.endGroup();
}

void saveStorage() {
    QSettings collSettings("Phoenix-Filemanager", "CollectionsEngine");
    collSettings.beginGroup("VirtualCollections");
    collSettings.remove("");
    for (auto it = g_virtualCollections.begin(); it != g_virtualCollections.end(); ++it) {
        collSettings.setValue(it.key(), it.value());
    }
    collSettings.endGroup();

    QSettings bmSettings("Phoenix-Filemanager", "BookmarksEngine");
    bmSettings.beginGroup("Bookmarks");
    bmSettings.remove("");
    for (auto it = g_bookmarks.begin(); it != g_bookmarks.end(); ++it) {
        bmSettings.setValue(it.key(), it.value());
    }
    bmSettings.endGroup();
}

// ============================================================================
// FEATURE: THE CORE PROXY ENGINE
// ============================================================================
class FileFilterAndColorProxyModel : public QSortFilterProxyModel {
    Q_OBJECT
public:
    FileFilterAndColorProxyModel(QObject *parent = nullptr)
    : QSortFilterProxyModel(parent), m_colorsEnabled(true), m_flatViewEnabled(false),
    m_recentsOnlyEnabled(false), m_activeCategory("All") {
        setFilterCaseSensitivity(Qt::CaseInsensitive);
        setFilterKeyColumn(0);
        setRecursiveFilteringEnabled(true);
    }

    void setColorsEnabled(bool enabled) { if (m_colorsEnabled != enabled) { m_colorsEnabled = enabled; invalidate(); } }
    void setFlatViewEnabled(bool enabled) { if (m_flatViewEnabled != enabled) { m_flatViewEnabled = enabled; QMetaObject::invokeMethod(this, [this]() { invalidate(); }, Qt::QueuedConnection); } }
    void setRecentsOnlyEnabled(bool enabled) { if (m_recentsOnlyEnabled != enabled) { m_recentsOnlyEnabled = enabled; invalidate(); } }
    void setActiveCategory(const QString &category) { m_activeCategory = category; invalidate(); }

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override {
        QString filePath;
        bool isDirectory = false;
        QDateTime lastMod = QDateTime::currentDateTime();

        QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(sourceModel());
        if (fsModel) {
            QModelIndex index = fsModel->index(source_row, 0, source_parent);
            if (fsModel->fileName(index) == "..") return true;
            if (m_flatViewEnabled && fsModel->isDir(index)) return false;
            filePath = fsModel->filePath(index);
            isDirectory = fsModel->isDir(index);
            lastMod = fsModel->fileInfo(index).lastModified();
        } else {
            QModelIndex pathIndex = sourceModel()->index(source_row, 1, source_parent);
            filePath = sourceModel()->data(pathIndex, Qt::DisplayRole).toString();
            QFileInfo info(filePath);
            isDirectory = info.isDir();
            lastMod = info.lastModified();
        }

        if (m_recentsOnlyEnabled && !isDirectory && lastMod.secsTo(QDateTime::currentDateTime()) > 86400) {
            return false;
        }

        if (m_activeCategory != "All" && !isDirectory) {
            QString ext = QFileInfo(filePath).suffix().toLower();
            if (m_activeCategory == "Images" && !QStringList({"png", "jpg", "jpeg", "gif", "webp", "bmp", "ico", "svg", "tga", "tiff"}).contains(ext)) return false;
            if (m_activeCategory == "Videos" && !QStringList({"mp4", "mkv", "avi", "mov", "wmv", "flv", "webm"}).contains(ext)) return false;
            if (m_activeCategory == "Docs" && !QStringList({"pdf", "txt", "doc", "docx", "xls", "xlsx", "odt", "md", "csv"}).contains(ext)) return false;
            if (m_activeCategory == "Scripts" && !QStringList({"py", "sh", "cpp", "h", "js", "html", "css", "json", "yaml", "conf"}).contains(ext)) return false;
        }

        return QSortFilterProxyModel::filterAcceptsRow(source_row, source_parent);
    }

    QVariant data(const QModelIndex &index, int role) const override {
        QModelIndex sourceIndex = mapToSource(index);
        QFileSystemModel *fsModel = qobject_cast<QFileSystemModel*>(sourceModel());

        // Resolve absolute target filepath location safely
        QFileInfo fileInfo;
        if (fsModel) fileInfo = fsModel->fileInfo(sourceIndex);
        else fileInfo = QFileInfo(sourceModel()->data(sourceModel()->index(sourceIndex.row(), 1), Qt::DisplayRole).toString());

        // ENGINE INJECTION: Dynamic Theater Media Cover Artwork Loader Loop (Column 0 Only)
        if (role == Qt::DecorationRole && index.column() == 0 && fileInfo.isDir()) {
            QString dirPath = fileInfo.absoluteFilePath();
QStringList artCoverTemplates = {"folder.jpg", "cover.jpg", "cover.png", "album.png", "folder.png", "cd_case.png"};
            for (const QString &artName : artCoverTemplates) {
                QString targetArtPath = dirPath + "/" + artName;
                if (QFile::exists(targetArtPath)) {
                    QPixmap customJacket(targetArtPath);
                    if (!customJacket.isNull()) {
                        // Scales cleanly into list icon views or tree grid displays down to square dimensions
                        return customJacket.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    }
                }
            }
        }

        // Fallback to original font coloring behavior properties
        if (role == Qt::ForegroundRole && m_colorsEnabled && fileInfo.exists()) {
            QDateTime createdTime = fileInfo.birthTime().isValid() ? fileInfo.birthTime() : fileInfo.lastModified();
            qint64 hoursOld = createdTime.secsTo(QDateTime::currentDateTime()) / 3600;
            if (hoursOld >= 0 && hoursOld <= 24) return QColor(Qt::red);
            else if (hoursOld > 24 && hoursOld <= 168) return QColor(Qt::blue);
        }

        return QSortFilterProxyModel::data(index, role);
    }
private:
    bool m_colorsEnabled, m_flatViewEnabled, m_recentsOnlyEnabled;
    QString m_activeCategory;
};

// ============================================================================
// FEATURE: BREADCRUMB NAVIGATION WIDGET
// ============================================================================
class BreadcrumbBar : public QWidget {
    Q_OBJECT
signals:
    void pathRequested(const QString &path);
public:
    BreadcrumbBar(QWidget *parent = nullptr) : QWidget(parent) {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        QHBoxLayout *mainLayout = new QHBoxLayout(this);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        stack = new QStackedWidget(this);
        stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        mainLayout->addWidget(stack);

        btnWidget = new QWidget(this);
        btnWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        btnLayout = new QHBoxLayout(btnWidget);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(2);

        editor = new QLineEdit(this);
        editor->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        connect(editor, &QLineEdit::returnPressed, this, [this]() {
            emit pathRequested(editor->text());
            stack->setCurrentIndex(0);
        });

        stack->addWidget(btnWidget);
        stack->addWidget(editor);
    }

    void setPath(const QString &path) {
        editor->setText(path);
        QLayoutItem *child;
        while ((child = btnLayout->takeAt(0)) != nullptr) {
            delete child->widget(); delete child;
        }

        if (path.startsWith("coll://")) {
            QPushButton *b = new QPushButton(path, this);
            btnLayout->addWidget(b);
        } else {
            QString current = "/";
            QPushButton *rootBtn = new QPushButton("💾 Root", this);
            connect(rootBtn, &QPushButton::clicked, this, [this]() { emit pathRequested("/"); });
            btnLayout->addWidget(rootBtn);

            QStringList parts = path.split("/", Qt::SkipEmptyParts);
            for (const QString &part : parts) {
                current += part + "/";
                QString target = current;
                QLabel *arrow = new QLabel("▶", this);
                QPushButton *btn = new QPushButton(part, this);
                connect(btn, &QPushButton::clicked, this, [this, target]() { emit pathRequested(target); });
                btnLayout->addWidget(arrow); btnLayout->addWidget(btn);
            }
        }

        QPushButton *editBtn = new QPushButton("📝 Edit Path", this);
        connect(editBtn, &QPushButton::clicked, this, [this]() {
            stack->setCurrentIndex(1);
            editor->setFocus(); editor->selectAll();
        });
        btnLayout->addWidget(editBtn); btnLayout->addStretch();
        stack->setCurrentIndex(0);
    }
    QString currentText() const { return editor->text(); }
private:
    QStackedWidget *stack; QWidget *btnWidget;
    QHBoxLayout *btnLayout; QLineEdit *editor;
};

// ============================================================================
// DIALOG WIDGETS
// ============================================================================
class BatchDirDialog : public QDialog {
    Q_OBJECT
public:
    BatchDirDialog(QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Opus Batch Directory Creator"); resize(380, 450);
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->addWidget(new QLabel("Enter directory names (one folder name per line):", this));
        textEdit = new QTextEdit(this); layout->addWidget(textEdit);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    QStringList getDirNames() const { return textEdit->toPlainText().split('\n', Qt::SkipEmptyParts); }
private: QTextEdit *textEdit;
};

class BatchRenameDialog : public QDialog {
    Q_OBJECT
public:
    BatchRenameDialog(const QStringList &files, QWidget *parent = nullptr) : QDialog(parent), targetFiles(files) {
        setWindowTitle("Live-Preview Batch Renamer"); resize(800, 500);
        QVBoxLayout *layout = new QVBoxLayout(this);
        QHBoxLayout *rulesLayout = new QHBoxLayout();
        QPushButton *btnTitleCase = new QPushButton("✨ Title Case Words", this);
        QPushButton *btnLowerCase = new QPushButton("🔽 Force Lowercase", this);
        rulesLayout->addWidget(btnTitleCase); rulesLayout->addWidget(btnLowerCase); rulesLayout->addStretch();
        layout->addLayout(rulesLayout);

        previewTable = new QTableWidget(targetFiles.size(), 2, this);
        previewTable->setHorizontalHeaderLabels({"Original Name", "Proposed New Name"});
        previewTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        layout->addWidget(previewTable);

        QDialogButtonBox *btnBox = new QDialogButtonBox(QDialogButtonBox::Apply | QDialogButtonBox::Cancel, this);
        layout->addWidget(btnBox);

        connect(btnTitleCase, &QPushButton::clicked, this, &BatchRenameDialog::applyTitleCaseRule);
        connect(btnLowerCase, &QPushButton::clicked, this, &BatchRenameDialog::applyLowerCaseRule);
        connect(btnBox->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &BatchRenameDialog::executeRename);
        connect(btnBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        populateTable();
    }
private slots:
    void applyTitleCaseRule() {
        for (int i = 0; i < previewTable->rowCount(); ++i) {
            QStringList words = previewTable->item(i, 0)->text().split(" ");
            for (QString &w : words) if (!w.isEmpty()) w[0] = w[0].toUpper();
            previewTable->item(i, 1)->setText(words.join(" "));
        }
    }
    void applyLowerCaseRule() { for (int i = 0; i < previewTable->rowCount(); ++i) previewTable->item(i, 1)->setText(previewTable->item(i, 0)->text().toLower()); }
    void executeRename() {
        int successCount = 0;
        for (int i = 0; i < previewTable->rowCount(); ++i) {
            QString fullPath = targetFiles[i], newName = previewTable->item(i, 1)->text();
            QFileInfo info(fullPath);
            if (info.fileName() != newName && info.absoluteDir().rename(info.fileName(), newName)) successCount++;
        }
        QMessageBox::information(this, "Renamer Finished", QString("Successfully renamed %1 files.").arg(successCount));
        accept();
    }
private:
    void populateTable() {
        for (int i = 0; i < targetFiles.size(); ++i) {
            QString name = QFileInfo(targetFiles[i]).fileName();
            previewTable->setItem(i, 0, new QTableWidgetItem(name)); previewTable->setItem(i, 1, new QTableWidgetItem(name));
        }
    }
    QStringList targetFiles; QTableWidget *previewTable;
};

class PreferencesDialog : public QDialog {
    Q_OBJECT
public:
    PreferencesDialog(const QStringList &currentLayout, QWidget *parent = nullptr) : QDialog(parent) {
        setWindowTitle("Toolbar Layout Preferences"); resize(600, 450);
        QHBoxLayout *mainLayout = new QHBoxLayout(this);

        QVBoxLayout *leftLayout = new QVBoxLayout();
        leftLayout->addWidget(new QLabel("Available Pool:", this));
        availableList = new QListWidget(this);
        availableList->addItems({"📄 New File", "📂 New Folder", "🎒 Link To Collection", "📥 Add to Stack",
                                 "✨ Batch Rename", "💻 Terminal Here", "🗑️ Delete",
                                 "❖ Toggle Grid/List", "⏳ Toggle Age Colors", "📅 Toggle Recents Only", "💥 Toggle Flat View",
                                 "🛠️ Run Custom Script"});
        leftLayout->addWidget(availableList); mainLayout->addLayout(leftLayout);

        QVBoxLayout *centerLayout = new QVBoxLayout();
        btnAdd = new QPushButton("Add ▶", this); btnRemove = new QPushButton("◀ Remove", this);
        centerLayout->addStretch(); centerLayout->addWidget(btnAdd); centerLayout->addWidget(btnRemove); centerLayout->addStretch();
        mainLayout->addLayout(centerLayout);

        QVBoxLayout *rightLayout = new QVBoxLayout();
        rightLayout->addWidget(new QLabel("Active Toolbar Layout Row:", this));
        activeList = new QListWidget(this);
        for (const QString &item : currentLayout) activeList->addItem(item);
        rightLayout->addWidget(activeList); mainLayout->addLayout(rightLayout);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        layout()->setContentsMargins(10, 10, 10, 10); ((QVBoxLayout*)layout())->addWidget(buttonBox);

        connect(btnAdd, &QPushButton::clicked, this, &PreferencesDialog::onAddClicked);
        connect(btnRemove, &QPushButton::clicked, this, &PreferencesDialog::onRemoveClicked);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }
    QStringList getFinalLayout() const { QStringList list; for (int i = 0; i < activeList->count(); ++i) list.append(activeList->item(i)->text()); return list; }
private slots:
    void onAddClicked() {
        QListWidgetItem *item = availableList->currentItem(); if (!item) return;
        if (item->text() == "🛠️ Run Custom Script") {
            bool ok; QString scriptPath = QInputDialog::getText(this, "Add Script", "Path:", QLineEdit::Normal, "/home/dave/myscript.sh", &ok);
            if (ok && !scriptPath.trimmed().isEmpty()) activeList->addItem("🛠️ Script: " + scriptPath.trimmed());
        } else activeList->addItem(item->text());
    }
    void onRemoveClicked() { delete activeList->currentItem(); }
private:
    QListWidget *availableList, *activeList; QPushButton *btnAdd, *btnRemove;
};

// ============================================================================
// FEATURE: INDEPENDENT FILE DISPLAY PANEL
// ============================================================================
class FilePanel : public QWidget {
    Q_OBJECT
signals:
    void panelActivated(FilePanel *panel);
    void pathNavigated(const QString &path, FilePanel *source);
    void fileHighlighted(const QString &filePath);

public:
    FilePanel(QWidget *parent = nullptr) : QWidget(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);
        layout->setContentsMargins(2, 2, 2, 2); layout->setSpacing(4);

        QHBoxLayout *controlsLayout = new QHBoxLayout();
        btnBack = new QPushButton("◀", this);
        btnForward = new QPushButton("▶", this);
        btnUp = new QPushButton("▲", this);
        btnBack->setEnabled(false);
        btnForward->setEnabled(false);

        controlsLayout->addWidget(btnBack);
        controlsLayout->addWidget(btnForward);
        controlsLayout->addWidget(btnUp);
        controlsLayout->addStretch();
        layout->addLayout(controlsLayout);

        breadcrumbBar = new BreadcrumbBar(this);
        layout->addWidget(breadcrumbBar);

        fsModel = new QFileSystemModel(this); fsModel->setRootPath(QDir::rootPath());
        collectionModel = new QStandardItemModel(this); collectionModel->setColumnCount(2);
        filterProxy = new FileFilterAndColorProxyModel(this); filterProxy->setSourceModel(fsModel);

        viewStack = new QStackedWidget(this);
        viewStack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        treeView = new QTreeView(this); treeView->setModel(filterProxy); treeView->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        listView = new QListView(this); listView->setModel(filterProxy); listView->setViewMode(QListView::IconMode);

        viewStack->addWidget(treeView); viewStack->addWidget(listView);
        layout->addWidget(viewStack);

        QHBoxLayout *categoryLayout = new QHBoxLayout();
        categoryLayout->setContentsMargins(2, 0, 2, 0);
        categoryLayout->setSpacing(1);

        QStringList categories = {"All", "Images", "Videos", "Docs", "Scripts"};
        for (const QString &cat : categories) {
            QPushButton *catBtn = new QPushButton(cat, this);
            catBtn->setCheckable(true);
            catBtn->setAutoExclusive(true);
            if (cat == "All") catBtn->setChecked(true);
            catBtn->setStyleSheet("QPushButton { padding: 2px 6px; font-size: 11px; }");
            connect(catBtn, &QPushButton::clicked, this, [this, cat]() { filterProxy->setActiveCategory(cat); });
            categoryLayout->addWidget(catBtn);
        }
        categoryLayout->addStretch();
        layout->addLayout(categoryLayout);

        filterBar = new QLineEdit(this); filterBar->setPlaceholderText("🔍 Filter active view...");
        layout->addWidget(filterBar);

        treeView->setContextMenuPolicy(Qt::CustomContextMenu);
        listView->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(treeView, &QWidget::customContextMenuRequested, this, &FilePanel::showContextMenu);
        connect(listView, &QWidget::customContextMenuRequested, this, &FilePanel::showContextMenu);

        navigateTo(QDir::homePath());

        treeView->installEventFilter(this); listView->installEventFilter(this); filterBar->installEventFilter(this);

        connect(btnUp, &QPushButton::clicked, this, &FilePanel::onUpClicked);
        connect(breadcrumbBar, &BreadcrumbBar::pathRequested, this, [this](const QString &p) { navigateTo(p); });
        connect(treeView, &QTreeView::doubleClicked, this, &FilePanel::onItemDoubleClicked);
        connect(listView, &QListView::doubleClicked, this, &FilePanel::onItemDoubleClicked);
        connect(filterBar, &QLineEdit::textChanged, this, [this](const QString &t) { filterProxy->setDynamicSortFilter(false); filterProxy->setFilterFixedString(t); filterProxy->setDynamicSortFilter(true); });

        connect(treeView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FilePanel::onSelectionChanged);
        connect(listView->selectionModel(), &QItemSelectionModel::selectionChanged, this, &FilePanel::onSelectionChanged);

        m_colorsChecked = true;
        m_recentsChecked = false;
        m_flatChecked = false;

        setActiveStyle(false);
    }

    void toggleGridView() { viewStack->setCurrentIndex(viewStack->currentIndex() == 0 ? 1 : 0); }
    void setAgeColors(bool checked) { m_colorsChecked = checked; filterProxy->setColorsEnabled(checked); }
    void setRecentsOnly(bool checked) { m_recentsChecked = checked; filterProxy->setRecentsOnlyEnabled(checked); }
    void setFlatView(bool checked) {
        m_flatChecked = checked;
        filterProxy->setFlatViewEnabled(checked);
        if(checked) treeView->expandAll();
        else navigateTo(breadcrumbBar->currentText());
    }

    bool isGridView() const { return viewStack->currentIndex() == 1; }
    bool isAgeColorsChecked() const { return m_colorsChecked; }
    bool isRecentsChecked() const { return m_recentsChecked; }
    bool isFlatViewChecked() const { return m_flatChecked; }
    QString currentPath() const { return breadcrumbBar->currentText(); }

    QStringList getSelectedFilePaths() const {
        QStringList paths;
        QAbstractItemView *currentView = (viewStack->currentIndex() == 0) ? static_cast<QAbstractItemView*>(treeView) : static_cast<QAbstractItemView*>(listView);
        QModelIndexList selected = currentView->selectionModel()->selectedRows();
        for (const QModelIndex &proxyIndex : selected) {
            QModelIndex sourceIndex = filterProxy->mapToSource(proxyIndex);
            if (filterProxy->sourceModel() == fsModel) paths << fsModel->filePath(sourceIndex);
            else paths << collectionModel->data(collectionModel->index(sourceIndex.row(), 1), Qt::DisplayRole).toString();
        }
        return paths;
    }

    void navigateTo(const QString &path) {
        if (path.startsWith("coll://")) {
            QString collName = path.mid(7);
            collectionModel->removeRows(0, collectionModel->rowCount());
            for (const QString &p : g_virtualCollections.value(collName)) {
                QList<QStandardItem*> rowItems; rowItems << new QStandardItem(QFileInfo(p).fileName()) << new QStandardItem(p);
                collectionModel->appendRow(rowItems);
            }
            filterProxy->setSourceModel(collectionModel);
            treeView->setRootIndex(QModelIndex()); listView->setRootIndex(QModelIndex());
        } else {
            filterProxy->setSourceModel(fsModel);
            QModelIndex targetProxyIndex = filterProxy->mapFromSource(fsModel->index(path));
            treeView->setRootIndex(targetProxyIndex); listView->setRootIndex(targetProxyIndex);
        }
        breadcrumbBar->setPath(path);
        emit pathNavigated(path, this);
    }


        void setActiveStyle(bool active) {
            if (active) {
                // SOURCE / ACTIVE SIDE: Bright electric blue frame around the active view grid
                treeView->setStyleSheet("QTreeView { border: 2px solid #3daee9; background-color: #1c1f22; }");
                listView->setStyleSheet("QListView { border: 2px solid #3daee9; background-color: #1c1f22; }");
            } else {
                // DESTINATION / INACTIVE SIDE: Muted charcoal gray frame for the background view grid
                treeView->setStyleSheet("QTreeView { border: 2px solid #343a40; background-color: #212529; }");
                listView->setStyleSheet("QListView { border: 2px solid #343a40; background-color: #212529; }");
            }
        }

        void executeCreateNewFolder() {
            if (currentPath().startsWith("coll://")) { QMessageBox::warning(this, "Action Blocked", "Cannot write into virtual collections."); return; }
            bool ok; QString name = QInputDialog::getText(this, "Create Folder", "Folder Name:", QLineEdit::Normal, "New Folder", &ok);
            if (ok && !name.trimmed().isEmpty()) {
                QDir dir(currentPath());
                if (dir.mkdir(name.trimmed())) QMessageBox::information(this, "Success", "Folder created successfully.");
                else QMessageBox::critical(this, "Error", "Failed to create folder. Check permissions or folder status.");
            }
        }

        void executeCreateNewFileFromTemplate(const QString &extension) {
            if (currentPath().startsWith("coll://")) { QMessageBox::warning(this, "Action Blocked", "Cannot write into virtual collections."); return; }
            bool ok; QString name = QInputDialog::getText(this, "Generate Test File", "Enter file name:", QLineEdit::Normal, "test_document" + extension, &ok);
            if (ok && !name.trimmed().isEmpty()) {
                QString finalName = name.trimmed();
                if (!finalName.endsWith(extension)) finalName += extension;
                QFile file(QDir(currentPath()).absoluteFilePath(finalName));
                if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                    if (extension == ".py") file.write("#!/usr/bin/env python3\nprint('Opus test run success!')\n");
                    else if (extension == ".html") file.write("<!DOCTYPE html>\n<html><body><h1>Sandbox File</h1></body></html>\n");
                    else file.write("Generated file template.\n");
                    file.close();
                    QMessageBox::information(this, "Success", "File generated successfully.");
                } else QMessageBox::critical(this, "Write Fault", "Could not write file.");
            }
        }

        void executeDelete() {
            QStringList paths = getSelectedFilePaths();
            if (paths.isEmpty()) return;

            QMessageBox::StandardButton reply = QMessageBox::question(this, "Confirm Deletion",
                                                                      QString("Are you sure you want to PERMANENTLY delete %1 item(s)?\nThis cannot be undone.").arg(paths.size()),
                                                                      QMessageBox::Yes | QMessageBox::No);

            if (reply == QMessageBox::Yes) {
                int successCount = 0;
                for (const QString &p : paths) {
                    QFileInfo info(p);
                    if (info.isDir()) { if (QDir(p).removeRecursively()) successCount++; }
                    else { if (QFile::remove(p)) successCount++; }
                }
                QMessageBox::information(this, "Deletion Complete", QString("Successfully deleted %1 items.").arg(successCount));
            }
        }

        void executeUp() { onUpClicked(); }

    protected:
        bool eventFilter(QObject *watched, QEvent *event) override {
            if (event->type() == QEvent::FocusIn || event->type() == QEvent::MouseButtonPress) emit panelActivated(this);
            return QWidget::eventFilter(watched, event);
        }

    private slots:
        void showContextMenu(const QPoint &pos) {
            QAbstractItemView *view = qobject_cast<QAbstractItemView*>(sender());
            QModelIndex index = view->indexAt(pos);
            QMenu contextMenu(this);

            if (index.isValid()) {
                contextMenu.addAction("📂 Open / Enter", [this, index]() { onItemDoubleClicked(index); });
                contextMenu.addSeparator();

                contextMenu.addAction("📋 Copy File", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (!sel.isEmpty()) {
                        QMimeData *mimeData = new QMimeData;
                        QList<QUrl> urls; for (const QString &p : sel) urls << QUrl::fromLocalFile(p);
                        mimeData->setUrls(urls); QApplication::clipboard()->setMimeData(mimeData);
                    }
                });
                contextMenu.addAction("✂️ Cut File", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (!sel.isEmpty()) {
                        QMimeData *mimeData = new QMimeData;
                        QList<QUrl> urls; for (const QString &p : sel) urls << QUrl::fromLocalFile(p);
                        mimeData->setUrls(urls);
                        QApplication::clipboard()->setMimeData(mimeData);
                    }
                });
                contextMenu.addAction("📝 Rename Item", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (sel.isEmpty()) return;
                    QFileInfo info(sel.first());
                    bool ok; QString newName = QInputDialog::getText(this, "Rename Item", "New Name:", QLineEdit::Normal, info.fileName(), &ok);
                    if (ok && !newName.trimmed().isEmpty()) info.absoluteDir().rename(info.fileName(), newName.trimmed());
                });

                QMenu *copyMenu = contextMenu.addMenu("🔗 Copy Names/Paths");
                copyMenu->addAction("Filename Only", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (!sel.isEmpty()) QApplication::clipboard()->setText(QFileInfo(sel.first()).fileName());
                });
                copyMenu->addAction("Absolute Path", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (!sel.isEmpty()) QApplication::clipboard()->setText(sel.first());
                });

                QMenu *archiveMenu = contextMenu.addMenu("🗜️ Archive (tar.gz)");
                archiveMenu->addAction("Compress Selected Here", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (sel.isEmpty()) return;
                    QStringList args; args << "-czvf" << "archive.tar.gz";
                    for (const QString& p : sel) args << QFileInfo(p).fileName();
                    QProcess::startDetached("tar", args, currentPath());
                });
                archiveMenu->addAction("Extract Here", [this]() {
                    QStringList sel = getSelectedFilePaths();
                    if (sel.isEmpty()) return;
                    QProcess::startDetached("tar", QStringList() << "-xzvf" << QFileInfo(sel.first()).fileName(), currentPath());
                });

                contextMenu.addSeparator();
                contextMenu.addAction("🗑️ Delete permanently", this, &FilePanel::executeDelete);
                contextMenu.addSeparator();
            } else {
                contextMenu.addAction("📋 Paste File Here", [this]() {
                    const QMimeData *mimeData = QApplication::clipboard()->mimeData();
                    if (mimeData && mimeData->hasUrls()) {
                        for (const QUrl &url : mimeData->urls()) {
                            QString src = url.toLocalFile(); QFileInfo info(src);
                            QFile::copy(src, currentPath() + "/" + info.fileName());
                        }
                    }
                });
                contextMenu.addSeparator();
            }

            contextMenu.addAction("💻 Open Terminal Here", [this]() {
                QStringList terminalCommands = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "xterm"};
                for (const QString &cmd : terminalCommands) if (QProcess::startDetached(cmd, QStringList(), currentPath())) break;
            });

            contextMenu.exec(view->viewport()->mapToGlobal(pos));
        }

        void onSelectionChanged(const QItemSelection &, const QItemSelection &) {
            QStringList selected = getSelectedFilePaths();
            if (!selected.isEmpty()) emit fileHighlighted(selected.first());
        }
        void onUpClicked() { if (breadcrumbBar->currentText().startsWith("coll://")) navigateTo(QDir::homePath()); else { QDir dir(breadcrumbBar->currentText()); if (dir.cdUp()) navigateTo(dir.absolutePath()); } }
        void onItemDoubleClicked(const QModelIndex &proxyIndex) {
            if (qApp->mouseButtons() == Qt::RightButton) return;
            QModelIndex sourceIndex = filterProxy->mapToSource(proxyIndex);
            if (filterProxy->sourceModel() == fsModel) {
                if (fsModel->isDir(sourceIndex)) navigateTo(fsModel->filePath(sourceIndex));
                else QDesktopServices::openUrl(QUrl::fromLocalFile(fsModel->filePath(sourceIndex)));
            }
            else {
                // Virtual Hub Collection mapping routine
                QString targetPath = collectionModel->data(collectionModel->index(sourceIndex.row(), 1), Qt::DisplayRole).toString();
                QFileInfo info(targetPath);
                if (info.isDir()) navigateTo(targetPath);
                else QDesktopServices::openUrl(QUrl::fromLocalFile(targetPath));
            }
        }

    private:
        QTreeView *treeView; QListView *listView; QStackedWidget *viewStack;
        QFileSystemModel *fsModel; QStandardItemModel *collectionModel;
        FileFilterAndColorProxyModel *filterProxy;
        QPushButton *btnBack, *btnForward, *btnUp;
        BreadcrumbBar *breadcrumbBar; QLineEdit *filterBar;
        bool m_colorsChecked, m_recentsChecked, m_flatChecked;
    };

// ============================================================================
// MAIN APPLICATION CORE FRAMEWORK
// ============================================================================
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow() : syncEnabled(false), activePanel(nullptr), dynamicCustomBar(nullptr) {
        loadStorage();
        QWidget *centralWidget = new QWidget(this);
        QVBoxLayout *mainVerticalLayout = new QVBoxLayout(centralWidget);
        mainVerticalLayout->setContentsMargins(4, 4, 4, 4);

        // STABILIZATION STEP: Build out the layout configuration metadata definitions pool completely
        toolbarLayoutState << "📄 New File" << "📂 New Folder" << "📥 Add to Stack" << "🎒 Link To Collection" << "✨ Batch Rename" << "🗑️ Delete"
                           << "❖ Toggle Grid/List" << "⏳ Toggle Age Colors" << "📅 Toggle Recents Only" << "💥 Toggle Flat View";

        // STABILIZATION STEP: Construct the bar interface asset explicitly BEFORE loading sub-tab views!
        rebuildDynamicToolbarDecks();

        QHBoxLayout *globalLayout = new QHBoxLayout();
        mainVerticalLayout->addLayout(globalLayout);

        // --- LEFT SIDEBAR ---
        QVBoxLayout *sidebarLayout = new QVBoxLayout();

        sidebarLayout->addWidget(new QLabel("⭐ Bookmarks", this));
        bookmarksListWidget = new QListWidget(this);
        rebuildBookmarksList();

        bookmarksListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(bookmarksListWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QListWidgetItem *item = bookmarksListWidget->itemAt(pos);
            if (!item) return;
            QMenu menu(this);
            menu.addAction("❌ Remove Bookmark", this, [this, item]() {
                g_bookmarks.remove(item->text());
                rebuildBookmarksList();
                saveStorage();
            });
            menu.exec(bookmarksListWidget->viewport()->mapToGlobal(pos));
        });

        sidebarLayout->addWidget(bookmarksListWidget);
        QPushButton *btnBookmarkCurrent = new QPushButton("➕ Bookmark Current", this);
        sidebarLayout->addWidget(btnBookmarkCurrent);

        sidebarLayout->addWidget(new QLabel("🎒 Virtual Hub", this));
        collListWidget = new QListWidget(this);
        for (auto it = g_virtualCollections.begin(); it != g_virtualCollections.end(); ++it) collListWidget->addItem(it.key());
        collListWidget->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(collListWidget, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QListWidgetItem *item = collListWidget->itemAt(pos);
            QMenu menu(this);
            menu.addAction("✨ Create New Collection", this, [this]() {
                bool ok;
                QString name = QInputDialog::getText(this, "New Collection", "Collection Name:", QLineEdit::Normal, "New Collection", &ok);
                if (ok && !name.trimmed().isEmpty() && !g_virtualCollections.contains(name.trimmed())) {
                    g_virtualCollections[name.trimmed()] = QStringList();
                    collListWidget->addItem(name.trimmed());
                    saveStorage();
                }
            });
            if (item) {
                menu.addAction("❌ Delete Collection", this, [this, item]() {
                    g_virtualCollections.remove(item->text());
                    delete item;
                    saveStorage();
                });
            }
            menu.exec(collListWidget->viewport()->mapToGlobal(pos));
        });
        sidebarLayout->addWidget(collListWidget);

        sidebarLayout->addWidget(new QLabel("📥 Drop Stack", this));
        dropStackWidget = new QListWidget(this);
        sidebarLayout->addWidget(dropStackWidget);

        QHBoxLayout *stackActionLayout = new QHBoxLayout();
        QPushButton *btnExecuteStack = new QPushButton("🚀 Move Stack", this);
        QPushButton *btnCopyStack = new QPushButton("📋 Copy Stack", this);
        stackActionLayout->addWidget(btnExecuteStack);
        stackActionLayout->addWidget(btnCopyStack);
        sidebarLayout->addLayout(stackActionLayout);

        globalLayout->addLayout(sidebarLayout, 1);

        // --- CENTER SPLITTER DECK ---
        splitter = new QSplitter(Qt::Horizontal, this);

        leftTabs = new QTabWidget(this); leftTabs->setTabsClosable(true);
        leftTabs->setMinimumWidth(300);

        rightTabs = new QTabWidget(this); rightTabs->setTabsClosable(true);
        rightTabs->setMinimumWidth(300);

        QToolButton *addLeft = new QToolButton(this); addLeft->setText("➕");
        connect(addLeft, &QToolButton::clicked, this, [this]() { addNewTab(leftTabs, QDir::homePath()); });
        leftTabs->setCornerWidget(addLeft, Qt::TopRightCorner);

        QToolButton *addRight = new QToolButton(this); addRight->setText("➕");
        connect(addRight, &QToolButton::clicked, this, [this]() { addNewTab(rightTabs, QDir::homePath()); });
        rightTabs->setCornerWidget(addRight, Qt::TopRightCorner);

        auto closeTab = [](QTabWidget *tabs, int idx) {
            if (tabs->count() > 1) { QWidget *w = tabs->widget(idx); tabs->removeTab(idx); w->deleteLater(); }
        };
        connect(leftTabs, &QTabWidget::tabCloseRequested, this, [this, closeTab](int idx){ closeTab(leftTabs, idx); });
        connect(rightTabs, &QTabWidget::tabCloseRequested, this, [this, closeTab](int idx){ closeTab(rightTabs, idx); });

        connect(leftTabs, &QTabWidget::currentChanged, this, [this](int idx){
            if(idx >= 0 && qobject_cast<FilePanel*>(leftTabs->widget(idx))) onPanelActivated(qobject_cast<FilePanel*>(leftTabs->widget(idx)));
        });
        connect(rightTabs, &QTabWidget::currentChanged, this, [this](int idx){
            if(idx >= 0 && qobject_cast<FilePanel*>(rightTabs->widget(idx))) onPanelActivated(qobject_cast<FilePanel*>(rightTabs->widget(idx)));
        });

        // Center Operations Strip
        centerStrip = new QWidget(this);
        QVBoxLayout *stripLayout = new QVBoxLayout(centerStrip);
        stripLayout->setContentsMargins(2, 2, 2, 2);
        stripLayout->setAlignment(Qt::AlignTop);

        centerStrip->setStyleSheet("QWidget { background: #212529; border: 1px solid #2d3238; border-radius: 6px; } "
                                   "QPushButton { padding: 6px; margin: 2px; font-weight: bold; font-size: 11px; background: #343a40; color: #ffffff; border: 1px solid #495057; border-radius: 4px; } "
                                   "QPushButton:hover { background: #495057; } "
                                   "QPushButton:disabled { background: #1a1d20; color: #6c757d; border: 1px solid #2d3238; }");

        QPushButton *btnCopy = new QPushButton("📋 Copy ▶", this);
        QPushButton *btnMove = new QPushButton("🚀 Move ▶", this);
        QPushButton *btnNewFolder = new QPushButton("📂 New Folder", this);
        QPushButton *btnSwap = new QPushButton("↕ Swap Panes", this);
        QPushButton *btnTrash = new QPushButton("🗑️ Delete Item", this);
        QPushButton *btnRefreshView = new QPushButton("🔄 Refresh View", this);
        QPushButton *btnCustom = new QPushButton("➕ Add Script", this);

        btnCopy->setMinimumWidth(100); btnMove->setMinimumWidth(100);
        btnNewFolder->setMinimumWidth(100); btnSwap->setMinimumWidth(100);
        btnTrash->setMinimumWidth(100); btnRefreshView->setMinimumWidth(100);
        btnCustom->setMinimumWidth(100);

        stripLayout->addWidget(btnCopy); stripLayout->addWidget(btnMove);
        stripLayout->addSpacing(8);
        stripLayout->addWidget(btnNewFolder); stripLayout->addWidget(btnRefreshView);
        stripLayout->addSpacing(8);
        stripLayout->addWidget(btnSwap); stripLayout->addWidget(btnTrash);
        stripLayout->addStretch();
        stripLayout->addWidget(btnCustom);

        connect(btnCopy, &QPushButton::clicked, this, [this]() { transferFiles(false); });
        connect(btnMove, &QPushButton::clicked, this, [this]() { transferFiles(true); });
        connect(btnNewFolder, &QPushButton::clicked, this, [this]() { if(activePanel) activePanel->executeCreateNewFolder(); });
        connect(btnSwap, &QPushButton::clicked, this, &MainWindow::swapPanes);
        connect(btnTrash, &QPushButton::clicked, this, [this]() { if(activePanel) activePanel->executeDelete(); });
        connect(btnRefreshView, &QPushButton::clicked, this, [this]() { if(activePanel) activePanel->navigateTo(activePanel->currentPath()); });

        // Media Inspector Panel Shell
        inspectorContainer = new QWidget(this);
        inspectorContainer->setMinimumWidth(250);
        QVBoxLayout *inspectorLayout = new QVBoxLayout(inspectorContainer);
        inspectorLayout->setContentsMargins(2, 2, 2, 2);

        inspectorStack = new QStackedWidget(this);
        inspectorPane = new QTextEdit(this);
        inspectorPane->setReadOnly(true);
        inspectorPane->setWordWrapMode(QTextOption::WrapAnywhere);
        inspectorPane->setPlaceholderText("Select a file to inspect...");
        inspectorStack->addWidget(inspectorPane);

        imagePreviewLabel = new QLabel(this);
        imagePreviewLabel->setAlignment(Qt::AlignCenter);
        inspectorStack->addWidget(imagePreviewLabel);

        mediaWidget = new QWidget(this);
        QVBoxLayout *mediaLayout = new QVBoxLayout(mediaWidget);
        mediaLayout->setContentsMargins(0, 0, 0, 0);

        mediaTitleLabel = new QLabel("No Track Loaded", this);
        mediaTitleLabel->setStyleSheet("font-weight: bold; color: #3daee9; padding: 4px;");
        mediaTitleLabel->setWordWrap(true);
        mediaLayout->addWidget(mediaTitleLabel);

        videoSurface = new QVideoWidget(this);
        mediaLayout->addWidget(videoSurface);

        QHBoxLayout *mediaControls = new QHBoxLayout();
        QPushButton *btnPlay = new QPushButton("▶ Play", this);
        QPushButton *btnPause = new QPushButton("⏸ Pause", this);
        mediaControls->addWidget(btnPlay);
        mediaControls->addWidget(btnPause);
        mediaLayout->addLayout(mediaControls);

        inspectorStack->addWidget(mediaWidget);
        inspectorLayout->addWidget(inspectorStack);

        splitter->addWidget(leftTabs);
        splitter->addWidget(centerStrip);
        splitter->addWidget(rightTabs);
        splitter->addWidget(inspectorContainer); // Aligns Inspector structural content properly on the right margin side!

        audioOutput = new QAudioOutput(this);
        mediaPlayer = new QMediaPlayer(this);
        mediaPlayer->setAudioOutput(audioOutput);
        mediaPlayer->setVideoOutput(videoSurface);

        connect(btnPlay, &QPushButton::clicked, this, [this]() {
            if (videoSurface->isVisible()) {
                mediaPlayer->play();
            } else {
                if (!mediaTitleLabel->text().isEmpty()) {
                    QString currentTrack = mediaTitleLabel->text().mid(2).trimmed();
                    if (activePanel) {
                        if (!activeAudioProcess) activeAudioProcess = new QProcess(this);
                        if (activeAudioProcess->state() != QProcess::Running) {
                            activeAudioProcess->start("ffplay", QStringList() << "-nodisp" << "-autoexit" << activePanel->currentPath() + "/" + currentTrack);
                        }
                    }
                }
            }
        });

        connect(btnPause, &QPushButton::clicked, this, [this]() {
            if (activeAudioProcess && activeAudioProcess->state() == QProcess::Running) {
                activeAudioProcess->terminate();
            }
            mediaPlayer->pause();
        });

        splitter->setSizes(QList<int>() << 520 << 110 << 520 << 250);
        globalLayout->addWidget(splitter, 5);

        setCentralWidget(centralWidget);
        resize(1400, 800);

        // Safe panel tab loading sequence triggers activation hooks on a fully realized backend toolbar instance
        addNewTab(leftTabs, QDir::homePath());
        addNewTab(rightTabs, QDir::homePath());

        if(leftTabs->widget(0)) onPanelActivated(qobject_cast<FilePanel*>(leftTabs->widget(0)));

        connect(collListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){ if(activePanel) activePanel->navigateTo("coll://" + item->text()); });
        connect(bookmarksListWidget, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item){ if(activePanel) activePanel->navigateTo(g_bookmarks[item->text()]); });
        connect(btnExecuteStack, &QPushButton::clicked, this, &MainWindow::executeDropStack);
        connect(btnBookmarkCurrent, &QPushButton::clicked, this, &MainWindow::addCurrentToBookmarks);

        // Copy Stack Staging Connector
        connect(btnCopyStack, &QPushButton::clicked, this, [this]() {
            if (dropStackWidget->count() == 0 || !activePanel) return;
            QString dest = activePanel->currentPath();
            if (dest.startsWith("coll://")) return;
            int success = 0;
            for (int i = 0; i < dropStackWidget->count(); ++i) {
                QString src = dropStackWidget->item(i)->text();
                if (QFile::copy(src, dest + "/" + QFileInfo(src).fileName())) success++;
            }
            dropStackWidget->clear();
            QMessageBox::information(this, "Stack Copied", QString("Successfully duplicated %1 files here.").arg(success));
        });

        QMenuBar *mb = menuBar();

        QMenu *fileMenu = mb->addMenu("&File");
        QMenu *newMenu = fileMenu->addMenu("Create New...");
        newMenu->addAction("📂 Folder", this, [this]() { if(activePanel) activePanel->executeCreateNewFolder(); });
        newMenu->addAction("📄 Plain Text (.txt)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".txt"); });
        newMenu->addAction("🐍 Python Script (.py)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".py"); });
        newMenu->addAction("🌐 HTML Page (.html)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".html"); });
        fileMenu->addSeparator();
        fileMenu->addAction("🚪 Quit", qApp, &QApplication::quit);

        QMenu *editMenu = mb->addMenu("&Edit");
        editMenu->addAction("📋 Copy Absolute Path", this, [this]() {
            if(activePanel && !activePanel->getSelectedFilePaths().isEmpty()) QApplication::clipboard()->setText(activePanel->getSelectedFilePaths().first());
        });
        editMenu->addAction("🗑️ Delete Selected", this, [this]() { if(activePanel) activePanel->executeDelete(); });

        QMenu *viewMenu = mb->addMenu("&View");
        QAction *actDualPane = viewMenu->addAction("🪟 Dual Pane Mode");
        actDualPane->setCheckable(true); actDualPane->setChecked(true);

        QAction *actCenterStrip = viewMenu->addAction("⏸️ Center Operations Strip");
        actCenterStrip->setCheckable(true); actCenterStrip->setChecked(true);

        QAction *actPreviewPane = viewMenu->addAction("🔍 Preview Pane (Inspector)");
        actPreviewPane->setCheckable(true); actPreviewPane->setChecked(true);

        auto refreshLayoutSizing = [this, actDualPane, actCenterStrip, actPreviewPane]() {
            bool dualActive = actDualPane->isChecked();
            bool stripActive = actCenterStrip->isChecked();
            bool previewActive = actPreviewPane->isChecked();

            rightTabs->setVisible(dualActive);
            centerStrip->setVisible(dualActive && stripActive);
            inspectorContainer->setVisible(previewActive);

            QList<int> currentSizes = splitter->sizes();
            int totalWidth = 0;
            for (int s : currentSizes) totalWidth += s;
            if (totalWidth <= 0) totalWidth = 1400;

            QList<int> newSizes;
            if (!dualActive && !previewActive) {
                newSizes << totalWidth << 0 << 0 << 0;
            } else {
                int allocLeft = dualActive ? (totalWidth / 2) - 100 : totalWidth - 300;
                int allocRight = dualActive ? (totalWidth / 2) - 100 : 0;
                int allocStrip = (dualActive && stripActive) ? 110 : 0;
                int allocPreview = previewActive ? 250 : 0;
                newSizes << allocLeft << allocStrip << allocRight << allocPreview;
            }
            splitter->setSizes(newSizes);
        };

        connect(actDualPane, &QAction::triggered, refreshLayoutSizing);
        connect(actCenterStrip, &QAction::triggered, refreshLayoutSizing);
        connect(actPreviewPane, &QAction::triggered, this, [this, refreshLayoutSizing](bool checked) {
            inspectorContainer->setVisible(checked);
            refreshLayoutSizing();
        });

        QAction *actSync = viewMenu->addAction("🔗 Linked Sync Navigation");
        actSync->setCheckable(true);
        connect(actSync, &QAction::triggered, this, [this](bool checked){ syncEnabled = checked; });

        QMenu *goMenu = mb->addMenu("&Go");
        goMenu->addAction("▲ Up", this, [this]() { if(activePanel) activePanel->executeUp(); });
        goMenu->addAction("🏠 Home", this, [this]() { if(activePanel) activePanel->navigateTo(QDir::homePath()); });

        QMenu *toolsMenu = mb->addMenu("&Tools");
        toolsMenu->addAction("💻 Open Terminal Here", this, [this]() {
            if(activePanel) {
                QStringList terminalCommands = {"x-terminal-emulator", "gnome-terminal", "konsole", "xfce4-terminal", "xterm"};
                for (const QString &cmd : terminalCommands) if (QProcess::startDetached(cmd, QStringList(), activePanel->currentPath())) break;
            }
        });
        toolsMenu->addAction("✨ Batch Rename", this, [this]() {
            if(activePanel && !activePanel->getSelectedFilePaths().isEmpty()) {
                BatchRenameDialog dlg(activePanel->getSelectedFilePaths(), this); dlg.exec();
            } else { QMessageBox::warning(this, "Warning", "Select files first!"); }
        });

        QMenu *settingsMenu = mb->addMenu("&Settings");
        settingsMenu->addAction("⚙️ Customize Toolbars...", this, &MainWindow::openPreferencesLayout);

        QMenu *helpMenu = mb->addMenu("&Help");
        helpMenu->addAction("📘 Documentation", this, [this]() {
            QMessageBox::information(this, "Documentation", "Phoenix-Filemanager Documentation is under construction.");
        });
        helpMenu->addAction("ℹ️ About Phoenix-Filemanager", this, [this]() {
            QMessageBox::about(this, "About Phoenix-Filemanager",
                               "<h2>Phoenix-Filemanager</h2>"
                               "<p><b>Version:</b> 1.14.0</p>"
                               "<p>A unified dual-pane workspace hub for all Linux distributions.</p>");
        });
    }

protected:
    void closeEvent(QCloseEvent *event) override { saveStorage(); QMainWindow::closeEvent(event); }

private slots:
    void transferFiles(bool isMove) {
        if (!activePanel) return;
        QTabWidget *srcTabs = nullptr; QTabWidget *dstTabs = nullptr;
        if (leftTabs->indexOf(activePanel) != -1) { srcTabs = leftTabs; dstTabs = rightTabs; }
        else if (rightTabs->indexOf(activePanel) != -1) { srcTabs = rightTabs; dstTabs = leftTabs; }
        else return;

        FilePanel *srcPanel = qobject_cast<FilePanel*>(srcTabs->currentWidget());
        FilePanel *dstPanel = qobject_cast<FilePanel*>(dstTabs->currentWidget());
        if (!srcPanel || !dstPanel) return;

        QString dstPath = dstPanel->currentPath();
        if (dstPath.startsWith("coll://")) { QMessageBox::warning(this, "Error", "Cannot write into virtual collections."); return; }
        QStringList selected = srcPanel->getSelectedFilePaths();
        if (selected.isEmpty()) return;

        bool overwriteAll = false;
        QStringList finalSelected;

        for (const QString &selPath : selected) {
            QFileInfo srcInfo(selPath); QFileInfo dstInfo(dstPath + "/" + srcInfo.fileName());
            if (dstInfo.exists() && !overwriteAll) {
                QString msg = QString("File conflict detected:\n\n'%1'\n\nOverwrite?").arg(srcInfo.fileName());
                QMessageBox::StandardButton reply = QMessageBox::question(this, "File Collision", msg,
                                                                          QMessageBox::Yes | QMessageBox::YesToAll | QMessageBox::No | QMessageBox::Cancel);
                if (reply == QMessageBox::Cancel) return;
                if (reply == QMessageBox::No) continue;
                if (reply == QMessageBox::YesToAll) overwriteAll = true;
            }
            finalSelected << selPath;
        }
        if (finalSelected.isEmpty()) return;

        QString cmd = isMove ? "mv" : "cp";
        QStringList args; if (!isMove) args << "-r";
        args << finalSelected << dstPath;

        if (QProcess::execute(cmd, args) == 0) {
            QMessageBox::information(this, "Transfer Complete", "Operation finished successfully.");
        }
    }

    void swapPanes() {
        FilePanel *lPanel = qobject_cast<FilePanel*>(leftTabs->currentWidget());
        FilePanel *rPanel = qobject_cast<FilePanel*>(rightTabs->currentWidget());
        if (lPanel && rPanel) {
            QString lPath = lPanel->currentPath(); QString rPath = rPanel->currentPath();
            lPanel->navigateTo(rPath); rPanel->navigateTo(lPath);
        }
    }

    void rebuildBookmarksList() {
        bookmarksListWidget->clear();
        for (auto it = g_bookmarks.begin(); it != g_bookmarks.end(); ++it) bookmarksListWidget->addItem(it.key());
    }

    void addCurrentToBookmarks() {
        if (!activePanel) return;
        QString current = activePanel->currentPath();
        if (current.startsWith("coll://")) return;
        QString defaultName = QFileInfo(current).fileName();
        if (defaultName.isEmpty()) defaultName = "Root";
        g_bookmarks[defaultName] = current;
        rebuildBookmarksList(); saveStorage();
    }

    void addNewTab(QTabWidget *tabs, const QString &path) {
        FilePanel *p = new FilePanel(this);
        connect(p, &FilePanel::panelActivated, this, &MainWindow::onPanelActivated);
        connect(p, &FilePanel::fileHighlighted, this, &MainWindow::updateInspectorPane);
        connect(p, &FilePanel::pathNavigated, this, [this, tabs, p](const QString &newPath) {
            int idx = tabs->indexOf(p);
            if(idx != -1) tabs->setTabText(idx, QFileInfo(newPath).fileName().isEmpty() ? "Root" : QFileInfo(newPath).fileName());
            onPanelNavigated(newPath, p);
        });
        p->navigateTo(path);
        int idx = tabs->addTab(p, QFileInfo(path).fileName().isEmpty() ? "Root" : QFileInfo(path).fileName());
        tabs->setCurrentIndex(idx);
    }

    void openPreferencesLayout() {
        PreferencesDialog dialog(toolbarLayoutState, this);
        if (dialog.exec() == QDialog::Accepted) { toolbarLayoutState = dialog.getFinalLayout(); rebuildDynamicToolbarDecks(); }
    }

    void onPanelActivated(FilePanel *panel) {
        if (activePanel != panel) {
            if(activePanel) activePanel->setActiveStyle(false);
            activePanel = panel;
            if(activePanel) activePanel->setActiveStyle(true);

            if (activePanel && dynamicCustomBar) {
                for (QAction *action : dynamicCustomBar->actions()) {
                    if (action->text() == "⏳ Age Colors") action->setChecked(activePanel->isAgeColorsChecked());
                    else if (action->text() == "📅 Recents") action->setChecked(activePanel->isRecentsChecked());
                    else if (action->text() == "💥 Flat View") action->setChecked(activePanel->isFlatViewChecked());
                }
            }
        }
    }

    void onPanelNavigated(const QString &path, FilePanel *source) {
        if (syncEnabled) {
            QTabWidget *targetTabs = (leftTabs->indexOf(source) != -1) ? rightTabs : leftTabs;
            if (targetTabs) {
                FilePanel *targetPanel = qobject_cast<FilePanel*>(targetTabs->currentWidget());
                if (targetPanel && targetPanel->currentPath() != path) {
                    targetPanel->blockSignals(true); targetPanel->navigateTo(path); targetPanel->blockSignals(false);
                    targetTabs->setTabText(targetTabs->currentIndex(), QFileInfo(path).fileName().isEmpty() ? "Root" : QFileInfo(path).fileName());
                }
            }
        }
    }

    void updateInspectorPane(const QString &filePath) {
        inspectorPane->disconnect();
        inspectorPane->setReadOnly(true);
        mediaPlayer->stop();

        if (activeAudioProcess && activeAudioProcess->state() == QProcess::Running) {
            activeAudioProcess->terminate();
            activeAudioProcess->waitForFinished(300);
        }

        QFileInfo info(filePath);
        if (info.isDir()) {
            inspectorStack->setCurrentIndex(0);
            inspectorPane->setPlainText(QString("[Directory Data]\nPath: %1").arg(filePath));
            return;
        }

        QString ext = info.suffix().toLower();

        if (QStringList({"png", "jpg", "jpeg", "gif", "webp", "bmp", "ico", "svg", "tga", "tiff"}).contains(ext)) {
            QPixmap pix(filePath);
            if (!pix.isNull()) {
                int targetWidth = inspectorContainer->width() - 15;
                int targetHeight = inspectorContainer->height() - 40;
                imagePreviewLabel->setPixmap(pix.scaled(targetWidth > 50 ? targetWidth : 200, targetHeight > 50 ? targetHeight : 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                inspectorStack->setCurrentIndex(1);
            }
            return;
        }

        if (QStringList({"mp3", "wav", "ogg", "mp4", "mkv", "avi", "flac", "m4a", "aac", "mov", "wmv", "webm"}).contains(ext)) {
            inspectorStack->setCurrentIndex(2);
            if (mediaTitleLabel) mediaTitleLabel->setText("🎵 " + info.fileName());
            if (QStringList({"mp3", "wav", "ogg", "flac", "m4a", "aac"}).contains(ext)) {
                videoSurface->setVisible(false);
                if (!activeAudioProcess) activeAudioProcess = new QProcess(this);
                activeAudioProcess->start("ffplay", QStringList() << "-nodisp" << "-autoexit" << filePath);
            } else {
                videoSurface->setVisible(true);
                mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
                QTimer::singleShot(100, this, [this]() { mediaPlayer->play(); });
            }
            return;
        }

        QFile file(filePath);
        if (file.open(QIODevice::ReadWrite | QIODevice::Text)) {
            QTextStream stream(&file);
            inspectorPane->setPlainText(stream.readAll());
            file.close();
            inspectorStack->setCurrentIndex(0);

            if (QStringList({"txt", "py", "sh", "cpp", "h", "json", "html", "css", "ini", "log", "md", "xml", "yaml", "yml", "conf", "cfg", "desktop", "service", "csv", "ts", "js"}).contains(ext)) {
                inspectorPane->setReadOnly(false);
                connect(inspectorPane, &QTextEdit::textChanged, this, [filePath, this]() {
                    QFile writeFile(filePath);
                    if (writeFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&writeFile); out << inspectorPane->toPlainText(); writeFile.close();
                    }
                });
            }
        } else {
            inspectorStack->setCurrentIndex(0);
            if (QStringList({"zip", "tar", "gz", "bz2", "xz", "7z", "rar"}).contains(ext)) {
                inspectorPane->setPlainText(QString("📦 Compressed Archive\n\nName: %1\nSize: %2 KB").arg(info.fileName()).arg(info.size() / 1024));
            } else {
                inspectorPane->setPlainText(QString("[Binary Data]\n\nName: %1\nSize: %2 Bytes").arg(info.fileName()).arg(info.size()));
            }
        }
    }

    void executeDropStack() {
        if (dropStackWidget->count() == 0 || !activePanel) return;
        QString destDir = activePanel->currentPath();
        if (destDir.startsWith("coll://")) return;
        int success = 0;
        for (int i = 0; i < dropStackWidget->count(); ++i) {
            QString srcPath = dropStackWidget->item(i)->text();
            if (QDir().rename(srcPath, destDir + "/" + QFileInfo(srcPath).fileName())) success++;
        }
        dropStackWidget->clear();
    }

    void rebuildDynamicToolbarDecks() {
        if (dynamicCustomBar) { removeToolBar(dynamicCustomBar); delete dynamicCustomBar; }
        dynamicCustomBar = addToolBar("Command Matrix");
        dynamicCustomBar->setMovable(false);
        dynamicCustomBar->setStyleSheet("QToolBar { spacing: 4px; padding: 4px; background: #212529; border-bottom: 1px solid #2d3238; } "
                                        "QToolButton { padding: 4px 8px; border-radius: 4px; border: 1px solid #495057; background: #343a40; color: #ffffff; font-weight: bold; } "
                                        "QToolButton:hover { background: #495057; } "
                                        "QToolButton:checked { background: #3daee9; color: #ffffff; border: 1px solid #2a85b3; }");

        for (const QString &token : toolbarLayoutState) {
            if (token == "📄 New File") {
                QToolButton *btn = new QToolButton(this); btn->setText("📄 New File"); btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                QMenu *m = new QMenu(btn);
                m->addAction("📄 Plain Text (.txt)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".txt"); });
                m->addAction("🐍 Python Script (.py)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".py"); });
                m->addAction("🌐 HTML Page (.html)", this, [this]() { if(activePanel) activePanel->executeCreateNewFileFromTemplate(".html"); });
                btn->setMenu(m); btn->setPopupMode(QToolButton::InstantPopup); dynamicCustomBar->addWidget(btn);
            }
            else if (token == "📂 New Folder") {
                QToolButton *btn = new QToolButton(this); btn->setText("📂 New Folder"); btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                QMenu *m = new QMenu(btn);
                m->addAction("✨ Batch Create", this, [this]() {
                    if(activePanel) {
                        BatchDirDialog d(this);
                        if (d.exec() == QDialog::Accepted) {
                            for (const QString &f : d.getDirNames()) QDir(activePanel->currentPath()).mkpath(f.trimmed());
                        }
                    }
                });
                btn->setMenu(m); btn->setPopupMode(QToolButton::MenuButtonPopup);
                connect(btn, &QToolButton::clicked, this, [this]() { if(activePanel) activePanel->executeCreateNewFolder(); });
                dynamicCustomBar->addWidget(btn);
            }
            else if (token == "✨ Batch Rename") {
                QAction *a = dynamicCustomBar->addAction("✨ Batch Rename");
                connect(a, &QAction::triggered, this, [this]() {
                    if(activePanel && !activePanel->getSelectedFilePaths().isEmpty()) { BatchRenameDialog dlg(activePanel->getSelectedFilePaths(), this); dlg.exec(); }
                });
            }
            else if (token == "📥 Add to Stack") {
                QAction *a = dynamicCustomBar->addAction("📥 Add to Stack");
                connect(a, &QAction::triggered, this, [this]() { if(activePanel) for(const QString &p : activePanel->getSelectedFilePaths()) dropStackWidget->addItem(p); });
            }
            else if (token == "🎒 Link To Collection") {
                QAction *a = dynamicCustomBar->addAction("🎒 Link To Collection");
                connect(a, &QAction::triggered, this, [this]() {
                    if(!activePanel || activePanel->getSelectedFilePaths().isEmpty() || g_virtualCollections.keys().isEmpty()) return;
                    bool ok; QString target = QInputDialog::getItem(this, "Add Link", "Select Collection:", g_virtualCollections.keys(), 0, false, &ok);
                    if (ok && !target.isEmpty()) {
                        for(const QString &p : activePanel->getSelectedFilePaths()) if (!g_virtualCollections[target].contains(p)) g_virtualCollections[target].append(p);
                        saveStorage();
                    }
                });
            }
            else if (token == "🗑️ Delete") {
                QAction *a = dynamicCustomBar->addAction("🗑️ Delete");
                connect(a, &QAction::triggered, this, [this]() { if(activePanel) activePanel->executeDelete(); });
            }
            else if (token == "❖ Toggle Grid/List") {
                QAction *a = dynamicCustomBar->addAction("❖ Toggle Grid/List");
                connect(a, &QAction::triggered, this, [this]() { if (activePanel) activePanel->toggleGridView(); });
            }
            else if (token == "⏳ Toggle Age Colors") {
                QAction *a = dynamicCustomBar->addAction("⏳ Age Colors"); a->setCheckable(true); a->setChecked(true);
                connect(a, &QAction::triggered, this, [this](bool checked) { if (activePanel) activePanel->setAgeColors(checked); });
            }
            else if (token == "📅 Toggle Recents Only") {
                QAction *a = dynamicCustomBar->addAction("📅 Recents"); a->setCheckable(true);
                connect(a, &QAction::triggered, this, [this](bool checked) { if (activePanel) activePanel->setRecentsOnly(checked); });
            }
            else if (token == "💥 Toggle Flat View") {
                QAction *a = dynamicCustomBar->addAction("💥 Flat View"); a->setCheckable(true);
                connect(a, &QAction::triggered, this, [this](bool checked) { if (activePanel) activePanel->setFlatView(checked); });
            }
        }
    }

private:
    QSplitter *splitter;
    QTabWidget *leftTabs, *rightTabs;
    QWidget *centerStrip;
    FilePanel *activePanel;
    QListWidget *collListWidget, *dropStackWidget, *bookmarksListWidget;
    QTextEdit *inspectorPane;
    QWidget *inspectorContainer;
    QStackedWidget *inspectorStack;
    QLabel *imagePreviewLabel;
    QWidget *mediaWidget;
    QVideoWidget *videoSurface;
    QMediaPlayer *mediaPlayer;
    QProcess *activeAudioProcess = nullptr;
    QAudioOutput *audioOutput;
    QLabel *mediaTitleLabel;
    QToolBar *dynamicCustomBar;
    QStringList toolbarLayoutState;
    bool syncEnabled;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Global Theme Harmonization Sweeper for Phoenix-Filemanager
    app.setStyleSheet(
        "QWidget { background-color: #212529; color: #f8f9fa; font-family: 'Segoe UI', 'Inter', sans-serif; font-size: 13px; } "
        "QTreeView, QListView { background-color: #1a1d20; border: 1px solid #2d3238; border-radius: 6px; padding: 4px; font-size: 13px; } "
        "QTreeView::item, QListView::item { padding: 6px; border-radius: 4px; } "
        "QTreeView::item:hover, QListView::item:hover { background-color: #2d3238; } "
        "QTreeView::item:selected, QListView::item:selected { background-color: #0d6efd; color: #ffffff; font-weight: bold; } "
        "QComboBox, QLineEdit { background-color: #2d3238; border: 1px solid #495057; border-radius: 4px; padding: 4px 8px; color: #ffffff; } "
        "QComboBox:hover, QLineEdit:focus { border: 1px solid #3daee9; } "
        "QMenuBar { background-color: #212529; border-bottom: 1px solid #2d3238; } "
        "QMenuBar::item:selected { background-color: #343a40; } "
        "QMenu { background-color: #1a1d20; border: 1px solid #2d3238; padding: 4px; border-radius: 6px; } "
        "QMenu::item:selected { background-color: #0d6efd; color: #ffffff; } "
        "QStatusBar { background-color: #1a1d20; border-top: 1px solid #2d3238; color: #adb5bd; font-size: 11px; }"
        );

    MainWindow w; w.show();
    return app.exec();
}
#include "main.moc"