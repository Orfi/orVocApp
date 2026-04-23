# Consolidate Import/Export Toolbar Buttons — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Replace 5 import/export toolbar buttons with 2 buttons (Export, Import), each opening a dropdown popup with format selection and inline options.

**Architecture:** Two new QML popup components (`ExportPopup.qml`, `ImportPopup.qml`) communicate with Main.qml via signals. Main.qml toolbar is simplified to 2 buttons + ExportOverlay. All existing file dialogs and C++ backend remain unchanged.

**Tech Stack:** Qt 6 Quick Controls (`Popup`, `RadioButton`, `ButtonGroup`), QML signals

**Spec:** `docs/superpowers/specs/2026-04-23-consolidate-import-export-buttons-design.md`

---

## File Structure

| File | Action | Responsibility |
|------|--------|----------------|
| `orVocab/ExportPopup.qml` | Create | Dropdown popup: format radios (JSON/Text/PDF), inline page size radios, emits `exportRequested` |
| `orVocab/ImportPopup.qml` | Create | Dropdown popup: format radios (JSON/Text), emits `importRequested` |
| `orVocab/Main.qml` | Modify | Remove 5 buttons + `pageSizeDialog`, add 2 buttons + popup instances + signal handlers |
| `orVocab/CMakeLists.txt` | Modify | Add `ExportPopup.qml` and `ImportPopup.qml` to `QML_FILES` |

---

### Task 1: Create ExportPopup.qml

**Files:**
- Create: `orVocab/ExportPopup.qml`

- [ ] **Step 1: Create ExportPopup.qml with full implementation**

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: exportPopup
    modal: true
    dim: true
    padding: 16
    width: 240

    property bool exporting: false

    signal exportRequested(string format, int pageSize)

    onExportingChanged: {
        if (exporting && formatGroup.checkedButton === pdfRadio) {
            jsonRadio.checked = true;
        }
    }

    background: Rectangle {
        color: "#162744"
        border.color: "#2d5a9e"
        border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Export Format"
            font.bold: true
        }

        ButtonGroup {
            id: formatGroup
        }

        RadioButton {
            id: jsonRadio
            text: "JSON"
            checked: true
            ButtonGroup.group: formatGroup
        }

        RadioButton {
            id: textRadio
            text: "Text"
            ButtonGroup.group: formatGroup
        }

        RadioButton {
            id: pdfRadio
            text: "PDF"
            enabled: !exportPopup.exporting
            opacity: enabled ? 1.0 : 0.4
            ButtonGroup.group: formatGroup
        }

        ColumnLayout {
            visible: pdfRadio.checked
            Layout.leftMargin: 28
            spacing: 4

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: pageSizeColumn.implicitHeight + 16
                color: "#101e36"
                radius: 4
                border.width: 0

                Rectangle {
                    width: 2
                    height: parent.height
                    color: "#2d5a9e"
                    anchors.left: parent.left
                }

                ColumnLayout {
                    id: pageSizeColumn
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.topMargin: 8
                    anchors.bottomMargin: 8
                    spacing: 4

                    Label {
                        text: "PAGE SIZE"
                        font.pixelSize: 11
                        color: "#7a8ba0"
                    }

                    ButtonGroup {
                        id: pageSizeGroup
                    }

                    RadioButton {
                        id: a4Radio
                        text: "A4"
                        checked: true
                        ButtonGroup.group: pageSizeGroup
                    }

                    RadioButton {
                        id: letterRadio
                        text: "Letter"
                        ButtonGroup.group: pageSizeGroup
                    }
                }
            }
        }

        RowLayout {
            Layout.topMargin: 8
            Layout.fillWidth: true

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1e3a5f"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                flat: true
                onClicked: exportPopup.close()
            }

            Button {
                text: "Export"
                onClicked: {
                    var format = "json";
                    if (textRadio.checked) format = "text";
                    else if (pdfRadio.checked) format = "pdf";

                    var pageSize = a4Radio.checked ? 0 : 1;
                    exportPopup.close();
                    exportPopup.exportRequested(format, pageSize);
                }
            }
        }
    }
}
```

- [ ] **Step 2: Verify file was created correctly**

Run: `head -5 orVocab/ExportPopup.qml`
Expected: The first 5 lines of the QML file (import statements and Popup declaration)

- [ ] **Step 3: Commit**

```bash
git add orVocab/ExportPopup.qml
git commit -m "refactor(RVC-33): ADDED: ExportPopup dropdown with format and page size selection"
```

---

### Task 2: Create ImportPopup.qml

**Files:**
- Create: `orVocab/ImportPopup.qml`

- [ ] **Step 1: Create ImportPopup.qml with full implementation**

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Popup {
    id: importPopup
    modal: true
    dim: true
    padding: 16
    width: 240

    signal importRequested(string format)

    background: Rectangle {
        color: "#162744"
        border.color: "#2d5a9e"
        border.width: 1
        radius: 6
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 8

        Label {
            text: "Import Format"
            font.bold: true
        }

        ButtonGroup {
            id: formatGroup
        }

        RadioButton {
            id: jsonRadio
            text: "JSON"
            checked: true
            ButtonGroup.group: formatGroup
        }

        RadioButton {
            id: textRadio
            text: "Text"
            ButtonGroup.group: formatGroup
        }

        RowLayout {
            Layout.topMargin: 8
            Layout.fillWidth: true

            Rectangle {
                Layout.fillWidth: true
                height: 1
                color: "#1e3a5f"
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Item { Layout.fillWidth: true }

            Button {
                text: "Cancel"
                flat: true
                onClicked: importPopup.close()
            }

            Button {
                text: "Import"
                onClicked: {
                    var format = jsonRadio.checked ? "json" : "text";
                    importPopup.close();
                    importPopup.importRequested(format);
                }
            }
        }
    }
}
```

- [ ] **Step 2: Verify file was created correctly**

Run: `head -5 orVocab/ImportPopup.qml`
Expected: The first 5 lines of the QML file (import statements and Popup declaration)

- [ ] **Step 3: Commit**

```bash
git add orVocab/ImportPopup.qml
git commit -m "refactor(RVC-33): ADDED: ImportPopup dropdown with format selection"
```

---

### Task 3: Register new QML files in CMakeLists.txt

**Files:**
- Modify: `orVocab/CMakeLists.txt:20-23` (QML_FILES list)

- [ ] **Step 1: Add ExportPopup.qml and ImportPopup.qml to QML_FILES**

In `orVocab/CMakeLists.txt`, find the `QML_FILES` list inside `qt_add_qml_module`:

```cmake
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
        ExportOverlay.qml
```

Change it to:

```cmake
    QML_FILES
        Main.qml
        Sidebar.qml
        TranslationView.qml
        ExportOverlay.qml
        ExportPopup.qml
        ImportPopup.qml
```

- [ ] **Step 2: Commit**

```bash
git add orVocab/CMakeLists.txt
git commit -m "refactor(RVC-33): ADDED: ExportPopup and ImportPopup to QML module"
```

---

### Task 4: Update Main.qml toolbar and wiring

**Files:**
- Modify: `orVocab/Main.qml`

- [ ] **Step 1: Replace 5 toolbar buttons with 2 buttons + ExportOverlay repositioned**

In Main.qml, replace the toolbar contents (lines 30-68). The current toolbar `RowLayout` contents are:

```qml
            Label {
                text: "orVocApp"
                font.bold: true
                Layout.fillWidth: true
            }

            Button {
                text: "Import JSON"
                onClicked: importJsonDialog.open()
            }
            Button {
                text: "Import Text"
                onClicked: importTextDialog.open()
            }
            Button {
                text: "Export JSON"
                onClicked: exportJsonDialog.open()
            }
            Button {
                text: "Export Text"
                onClicked: exportTextDialog.open()
            }
            ExportOverlay {
                id: exportOverlay
            }

            Button {
                text: "Export PDF"
                enabled: !exportOverlay.exporting
                opacity: enabled ? 1.0 : 0.4
                onClicked: pageSizeDialog.open()
            }
```

Replace with:

```qml
            Label {
                text: "orVocApp"
                font.bold: true
                Layout.fillWidth: true
            }

            ExportOverlay {
                id: exportOverlay
            }

            Button {
                id: exportButton
                text: "Export"
                onClicked: exportPopup.open()
            }

            Button {
                id: importButton
                text: "Import"
                onClicked: importPopup.open()
            }
```

- [ ] **Step 2: Remove pageSizeDialog**

Delete the entire `pageSizeDialog` Dialog block (lines 147-175 in the original file):

```qml
    Dialog {
        id: pageSizeDialog
        title: "Select Page Size"
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Cancel

        RowLayout {
            spacing: 12

            Button {
                text: "A4"
                onClicked: {
                    pageSizeDialog.close();
                    root.selectedPageSize = 0;
                    exportPdfDialog.open();
                }
            }

            Button {
                text: "Letter"
                onClicked: {
                    pageSizeDialog.close();
                    root.selectedPageSize = 1;
                    exportPdfDialog.open();
                }
            }
        }
    }
```

- [ ] **Step 3: Add popup instances and signal handlers**

Add the following after the `confirmImportDialog` block (after line 145 in the original, which is the closing `}` of `confirmImportDialog`):

```qml
    ExportPopup {
        id: exportPopup
        parent: exportButton
        x: exportButton.width - width
        y: exportButton.height
        exporting: exportOverlay.exporting

        onExportRequested: function(format, pageSize) {
            if (format === "json") {
                exportJsonDialog.open();
            } else if (format === "text") {
                exportTextDialog.open();
            } else if (format === "pdf") {
                root.selectedPageSize = pageSize;
                exportPdfDialog.open();
            }
        }
    }

    ImportPopup {
        id: importPopup
        parent: importButton
        x: importButton.width - width
        y: importButton.height

        onImportRequested: function(format) {
            if (format === "json") {
                importJsonDialog.open();
            } else if (format === "text") {
                importTextDialog.open();
            }
        }
    }
```

- [ ] **Step 4: Build and verify compilation**

Run: `cmake --build build/Desktop_Qt_6_10_2-Debug 2>&1 | tail -5`
Expected: Build succeeds with no errors

- [ ] **Step 5: Run the application and verify**

Run: `./build/Desktop_Qt_6_10_2-Debug/orVocab/orVocApp`

Verify manually:
1. Toolbar shows: `[orVocApp] ... [Export] [Import]` (ExportOverlay hidden)
2. Clicking Export opens dropdown popup below button with JSON/Text/PDF radios
3. Selecting PDF shows inline A4/Letter page size picker
4. Clicking Export in popup with JSON selected opens JSON file save dialog
5. Clicking Export in popup with Text selected opens Text file save dialog
6. Clicking Export in popup with PDF + A4 selected opens PDF file save dialog
7. Clicking Import opens dropdown popup with JSON/Text radios
8. Clicking Import in popup with JSON selected opens JSON file open dialog, then shows confirmation dialog
9. Clicking Import in popup with Text selected opens Text file open dialog
10. During PDF export: ExportOverlay appears left of Export button with progress bar, PDF radio in popup is disabled

- [ ] **Step 6: Commit**

```bash
git add orVocab/Main.qml
git commit -m "refactor(RVC-33): CHANGED: toolbar from 5 buttons to 2 dropdown popups with format selection"
```

---

### Task 5: Run existing tests

**Files:**
- None (verification only)

- [ ] **Step 1: Run full test suite**

Run: `cd build/Desktop_Qt_6_10_2-Debug && ctest --output-on-failure`
Expected: All 11 tests pass (no regressions — tests cover C++ backend which is unchanged)

- [ ] **Step 2: Commit if any test fixes were needed**

If all tests pass without changes, no commit needed. If adjustments were required, commit them.
