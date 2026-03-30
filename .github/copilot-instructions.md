# PartVault – Copilot Instructions

## Tech Stack
Qt6 · SQLite · CMake 3.19+ · C++17 · GitHub Actions

## Naming Conventions

### Private member variables (camelCase, `m` prefix, NO underscore)
```cpp
// CORRECT
QString           mConnectionName;
CategoryNode*     mRoot = nullptr;
QLineEdit*        mNameEdit;
QDialogButtonBox* mButtons;

// WRONG
QString           m_connectionName;
CategoryNode*     m_root;
```

### Other names
| Kind | Style | Example |
|------|-------|---------|
| Classes | PascalCase | `CategoryTreeModel` |
| Methods / locals | camelCase | `buildPath()`, `categoryId` |
| Enum values / constants | PascalCase | `ColName`, `IdRole` |
| Source files | lowercasenospaces | `categorytreemodel.cpp` |

## Architecture Patterns

### Database access
- Store the **connection name** as `QString mConnectionName`, never a `QSqlDatabase` member.
- Look up per-method: `QSqlDatabase db = QSqlDatabase::database(mConnectionName);`

### Dialogs
- Constructor receives `connectionName` + pre-selected IDs (e.g. `parentId`).
- `AddCategoryDialog(connectionName, parentId, parent)` — parent from tree selection, shown as read-only label.
- OK enabled only when name is non-empty.

### Session persistence
- `MainWindow::restoreSession()` / `saveSession()` via `QSettings`.
- Keys: `geometry`, `state`, `categories/expanded` (QList\<int\>), `categories/selected` (int).
