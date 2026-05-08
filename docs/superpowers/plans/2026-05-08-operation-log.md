# Operation Log Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement a Master-Detail Operation Log audit viewer with pagination and JSON-based diff rendering for PetManager.

**Architecture:** 
1. Database: Create a `sys_operation_logs` table (if not exists) and a `LogDataManager` for executing paginated queries using `LIMIT` and `OFFSET`.
2. UI: Build `OperationLogDialog` triggered from `PersonalModule`. It uses a Master-Detail layout.
3. Rendering: The right-side detail panel parses `details` (JSON) and dynamically generates an HTML table to render Diff visual effects via `QTextBrowser`.

**Tech Stack:** Qt C++, QSqlQuery, QJsonDocument, QTextBrowser (HTML/CSS rendering), SQLite/MySQL.

---

### Task 1: Update Data Model and LogDataManager

**Files:**
- Modify: `common_types.h`
- Create: `modules/logdatamanager.h`
- Create: `modules/logdatamanager.cpp`

- [ ] **Step 1: Define `SysOperationLog` struct in `common_types.h`**
Add `SysOperationLog` at the end of `common_types.h`:
```cpp
struct SysOperationLog {
    QString id;
    QString timestamp;
    QString operatorName;
    QString module;       // e.g., "订单", "库存", "员工"
    QString action;       // e.g., "UPDATE", "DELETE", "CREATE"
    QString details;      // JSON string
};
Q_DECLARE_METATYPE(SysOperationLog)
```

- [ ] **Step 2: Create `LogDataManager` header**
Create `modules/logdatamanager.h`:
```cpp
#ifndef LOGDATAMANAGER_H
#define LOGDATAMANAGER_H

#include <QObject>
#include <QList>
#include "../common_types.h"

class LogDataManager : public QObject {
    Q_OBJECT
public:
    explicit LogDataManager(QObject *parent = nullptr);
    bool initTable();
    QList<SysOperationLog> fetchLogs(int limit, int offset, const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "");
    int getTotalCount(const QString &startDate = "", const QString &endDate = "", const QString &operatorName = "");
    
    // For testing
    bool insertMockLog(const SysOperationLog &log);
};

#endif // LOGDATAMANAGER_H
```

- [ ] **Step 3: Implement `LogDataManager` methods**
Create `modules/logdatamanager.cpp`. Implement `initTable` (CREATE TABLE IF NOT EXISTS sys_operation_logs), `fetchLogs` (SELECT ... LIMIT ... OFFSET), and `getTotalCount`. Include `<QSqlQuery>`, `<QSqlError>`, `<QVariant>`.

### Task 2: Create Operation Log Dialog UI (Master Panel)

**Files:**
- Create: `modules/operationlogdialog.h`
- Create: `modules/operationlogdialog.cpp`

- [ ] **Step 1: Define `OperationLogDialog` header**
Create `modules/operationlogdialog.h`:
```cpp
#ifndef OPERATIONLOGDIALOG_H
#define OPERATIONLOGDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTextBrowser>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QDateEdit>
#include "logdatamanager.h"

class OperationLogDialog : public QDialog {
    Q_OBJECT
public:
    explicit OperationLogDialog(QWidget *parent = nullptr);

private slots:
    void loadPage(int page);
    void onLogSelected(QListWidgetItem *item);
    void onSearchClicked();

private:
    void setupUi();
    QString renderDiffHtml(const QString &jsonStr);

    LogDataManager *m_dataManager;
    int m_currentPage;
    int m_itemsPerPage;
    int m_totalItems;

    QDateEdit *m_startDateEdit;
    QDateEdit *m_endDateEdit;
    QLineEdit *m_operatorEdit;
    QListWidget *m_listWidget;
    QTextBrowser *m_detailBrowser;
    QLabel *m_pageLabel;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
};

#endif // OPERATIONLOGDIALOG_H
```

- [ ] **Step 2: Implement Layout and Initialization**
In `modules/operationlogdialog.cpp`, implement `setupUi` using `QHBoxLayout` and `QVBoxLayout` to create the Master-Detail layout. Set dialog size to e.g., 900x600. Initialize `LogDataManager`, call `initTable()`, and connect buttons to slots.

- [ ] **Step 3: Implement Pagination Logic**
Implement `loadPage(int page)`: Calculate offset, fetch data from `m_dataManager`, populate `m_listWidget` with items representing logs (store the `details` JSON in the item's `Qt::UserRole`). Update `m_pageLabel` and button states.

### Task 3: Implement JSON Diff Rendering

**Files:**
- Modify: `modules/operationlogdialog.cpp`

- [ ] **Step 1: Implement `renderDiffHtml`**
In `modules/operationlogdialog.cpp`, include `<QJsonDocument>`, `<QJsonObject>`, `<QJsonValue>`.
Implement `renderDiffHtml` to parse the JSON (e.g., `{"field": "Status", "old": "Paid", "new": "Refunded"}`) and return HTML string:
```cpp
QString OperationLogDialog::renderDiffHtml(const QString &jsonStr) {
    QJsonDocument doc = QJsonDocument::fromJson(jsonStr.toUtf8());
    if (doc.isNull() || !doc.isObject()) {
        return QString("<p style='color:#475569;'>%1</p>").arg(jsonStr.toHtmlEscaped());
    }
    
    QJsonObject obj = doc.object();
    QString field = obj.value("field").toString();
    QString oldVal = obj.value("old").toString();
    QString newVal = obj.value("new").toString();
    
    return QString(
        "<h3>数据变更对比</h3>"
        "<table width='100%' border='1' cellspacing='0' cellpadding='8' style='border-collapse:collapse; border-color:#e2e8f0;'>"
        "<tr style='background-color:#f1f5f9;'><th>字段名</th><th>修改前 (Old)</th><th>修改后 (New)</th></tr>"
        "<tr>"
        "<td>%1</td>"
        "<td style='background-color:#fee2e2; text-decoration:line-through; color:#991b1b;'>%2</td>"
        "<td style='background-color:#dcfce7; color:#166534;'>%3</td>"
        "</tr>"
        "</table>"
    ).arg(field.toHtmlEscaped(), oldVal.toHtmlEscaped(), newVal.toHtmlEscaped());
}
```

- [ ] **Step 2: Connect Selection Event**
Implement `onLogSelected`: Extract the JSON from the selected item's `Qt::UserRole`, call `renderDiffHtml`, and set it to `m_detailBrowser->setHtml(html)`.

### Task 4: Connect to Personal Module & Inject Mock Data

**Files:**
- Modify: `modules/personalmodule.cpp`

- [ ] **Step 1: Update action handler**
In `modules/personalmodule.cpp`, include `"operationlogdialog.h"`.
Find the `if (actionName == "操作日志")` block. Remove the `QMessageBox::information` code and replace it with:
```cpp
OperationLogDialog dialog(this);
dialog.exec();
```

- [ ] **Step 2: Add Mock Data on Init**
In `OperationLogDialog` constructor, add a temporary check: if `m_dataManager->getTotalCount() == 0`, insert 5-10 mock `SysOperationLog` entries with different JSON payload formats to verify the list and diff rendering works correctly.

### Task 5: Self-Review & Verification

**Files:**
- Run: Build and verify manually

- [ ] **Step 1: Build the project**
Run qmake and make (or ninja) to compile the project. Fix any missing includes (`<QDebug>`, etc).
- [ ] **Step 2: Run Application**
Open the application, navigate to Personal Center, click "操作日志" and verify the pagination and diff highlights are correctly displayed.
