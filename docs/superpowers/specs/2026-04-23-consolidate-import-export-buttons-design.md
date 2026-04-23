# RVC-33: Consolidate Import/Export Toolbar Buttons

## Summary

Replace the 5 individual import/export toolbar buttons (Import JSON, Import Text, Export JSON, Export Text, Export PDF) with 2 buttons — **Export** and **Import** — each opening a dropdown popup with format selection.

## Toolbar Layout

```
[orVocApp label] ──── spacer ──── [ExportOverlay] [Export] [Import]
```

- ExportOverlay sits to the left of the Export button (unchanged component, repositioned)
- ExportOverlay remains hidden by default, appears only during PDF export (existing behavior preserved)
- The 5 old buttons are removed

## ExportPopup.qml (New File)

A `Popup` dropdown anchored below the Export toolbar button.

### Contents

- **Format radio group:** JSON / Text / PDF (JSON selected by default)
- **Page size radio group:** A4 / Letter — appears inline below the PDF option only when PDF is selected (A4 selected by default)
- **Buttons:** Cancel (closes popup) and Export (triggers export)

### Signal

```qml
signal exportRequested(string format, int pageSize)
```

Emitted when the user clicks the Export button inside the popup. Main.qml receives this signal and opens the appropriate `Platform.FileDialog`.

### Properties

- `exporting` (bool) — bound to `ExportOverlay.exporting` from Main.qml. Controls whether the PDF radio option is disabled.

### PDF Export In Progress

When `exporting` is true:
- The PDF radio option is disabled (grayed out)
- If PDF was selected, selection resets to JSON
- JSON and Text radio options remain enabled
- The Export toolbar button itself remains enabled

### Signal Details

`pageSize` is only meaningful when `format` is `"pdf"`. When format is `"json"` or `"text"`, `pageSize` is ignored by Main.qml.

## ImportPopup.qml (New File)

A `Popup` dropdown anchored below the Import toolbar button, same visual style as ExportPopup.

### Contents

- **Format radio group:** JSON / Text (JSON selected by default)
- **Buttons:** Cancel (closes popup) and Import (triggers import)

### Signal

```qml
signal importRequested(string format)
```

Emitted when the user clicks the Import button inside the popup. Main.qml receives this signal and opens the appropriate `Platform.FileDialog`.

## Main.qml Changes

### Removed

- 5 toolbar buttons: Import JSON, Import Text, Export JSON, Export Text, Export PDF
- `pageSizeDialog` Dialog — page size selection moves into ExportPopup

### Added

- 2 toolbar buttons: Export and Import
- `ExportPopup` instance anchored below Export button
- `ImportPopup` instance anchored below Import button
- Signal handlers connecting popup signals to existing file dialogs

### Unchanged

- All 4 `Platform.FileDialog` instances (importJson, importText, exportJson, exportText)
- `exportPdfDialog` Platform.FileDialog
- `confirmImportDialog` (JSON import replace confirmation)
- `ExportOverlay` component (repositioned, not modified)
- `root.selectedPageSize` property (set from ExportPopup signal)

## Signal Flow

### Export

1. User clicks Export toolbar button
2. ExportPopup opens as dropdown below button
3. User selects format (+ page size if PDF)
4. User clicks Export inside popup
5. Popup emits `exportRequested(format, pageSize)` and closes
6. Main.qml handler sets `root.selectedPageSize` (if PDF) and opens matching file dialog
7. File dialog `onAccepted` handlers execute (unchanged)

### Import

1. User clicks Import toolbar button
2. ImportPopup opens as dropdown below button
3. User selects format
4. User clicks Import inside popup
5. Popup emits `importRequested(format)` and closes
6. Main.qml handler opens matching file dialog
7. For JSON: file dialog `onAccepted` opens `confirmImportDialog` (unchanged)
8. For Text: file dialog `onAccepted` calls `VocabManager.importText()` directly (unchanged)

## Preserved Behavior

- `defaultSuffix` on all file save dialogs (json, txt, pdf)
- JSON import confirmation dialog ("This will replace your current word list of N words. Continue?")
- ExportOverlay progress bar and fade notification animations
- PDF radio disabled during active PDF export

## Files Changed

| File | Action |
|------|--------|
| `ExportPopup.qml` | New |
| `ImportPopup.qml` | New |
| `Main.qml` | Modified |
| `CMakeLists.txt` | Modified (add QML_FILES) |
| `ExportOverlay.qml` | Unchanged |
| All C++ files | Unchanged |

## Dependencies

No new dependencies. Uses existing Qt Quick Controls components: `Popup`, `RadioButton`, `ButtonGroup`.
