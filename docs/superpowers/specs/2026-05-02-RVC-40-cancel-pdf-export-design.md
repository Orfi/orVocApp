# RVC-40 — Cancellable PDF Export

Ticket: https://welorfi.atlassian.net/browse/RVC-40

## Problem

PDF export runs in two phases — a sequential network fetch phase on the main thread (≈150 ms × word count) and a `QThread`-based render phase using `QPrinter`/`QPainter`. Neither phase observes a cancellation signal. Once the user clicks **Export**, they are locked out of the sidebar and Import/PDF-export controls until the pipeline fully completes, which for a word bank of a few hundred entries can take a minute or more.

## Goals

- User can cancel an in-progress export at any point between clicking **Export** and the final `finished` signal.
- Cancellation aborts the pipeline as fast as the underlying APIs allow — no waiting for the current network reply to time out, no waiting for the remaining pages to render.
- No partial PDF is left on disk after cancel.
- UI controls that were disabled during export re-enable only after the pipeline has fully unwound, so a second export cannot race the first one's teardown.

## Non-goals

- Pausing / resuming an export. Cancel is one-shot.
- Cancelling other exports (JSON / plain text) — these complete synchronously and instantly.
- Showing cancellation progress (spinner, percent, etc.). The overlay simply hides and a green "Export cancelled" notification fades.

## UX

### Cancel control

A round red button lives inline in the toolbar `NotificationOverlay`, immediately after the progress bar. It matches the existing pronunciation (`▶`) button's sizing and padding:

- Square/round shape, width = height = toolbar button height.
- No text, no icon — just a red fill.
- Visible only while `exporting === true`.
- Disables on click (self-guard against double-click) until `exporting` flips back to `false`.

### Notification on cancel

After cancel completes, the overlay shows a **green** (success-color) fading notification:

```
Export cancelled
```

3-second hold, reusing the existing `showNotification(text, isError=false)` path. Failure still shows red "Export failed" via the existing `finished(false, ...)` path — the two outcomes stay visually distinct.

### Re-enable timing

Sidebar, Import button, and the PDF radio button re-enable only when `NotificationOverlay.exporting` becomes `false`. The overlay sets that flag to `false` exclusively in its `onCancelled` / `onFinished` handlers, which are emitted by the exporter only after:

1. the outstanding network reply (if any) has been aborted,
2. the render worker thread has joined,
3. any partial PDF file has been removed.

The Cancel button itself disables the instant it is clicked so the user can't double-fire `requestCancel()`.

## Architecture

### `BaseExporter` changes

New members:

```cpp
std::atomic<bool> m_cancelled{false};
QPointer<QNetworkReply> m_currentReply;   // tracks the in-flight fetch
```

New public slot:

```cpp
Q_INVOKABLE void requestCancel();
```

New signal:

```cpp
void cancelled();
```

`requestCancel()` does three things, in order:

1. `m_cancelled.store(true, std::memory_order_release)`
2. If `m_currentReply` is still valid, call `m_currentReply->abort()` (idempotent — fires `finished` with an error, which the reply callbacks then observe as cancellation).
3. No thread join here — render worker observes the flag itself.

### Fetch phase

`BaseExporter::fetchNextWord`, `onDefinitionReply`, and `onTranslationReply` each gain an early bail:

```cpp
if (m_cancelled.load(std::memory_order_acquire)) {
    emit cancelled();
    return;
}
```

The reply callbacks check the flag **before** re-entering `fetchNextWord` or scheduling the next `QTimer::singleShot`. `m_currentReply` is set to the fresh `QNetworkReply*` at each step so `requestCancel` always has the right handle.

### Render phase

The worker lambda in `BaseExporter::startRender` already calls `renderToFile` synchronously on the worker thread. `renderToFile` is made cancellation-aware:

- At each page boundary inside the letter-group loop, check `m_cancelled`.
- On cancel: break out of the loop, call `painter.end()`, call `QFile::remove(outputPath)` to delete the partial PDF, return `false`.

After `renderToFile` returns, the worker lambda inspects `m_cancelled`:

```cpp
if (m_cancelled.load(std::memory_order_acquire)) {
    emit cancelled();
} else {
    emit finished(success, m_outputPath);
}
```

### Signals summary

| Signal | Meaning | Overlay color |
| --- | --- | --- |
| `finished(true, path)` | Success | green "PDF exported successfully" |
| `finished(false, path)` | System failure (render error, all-entries-invalid) | red "Export failed" |
| `cancelled()` | User-initiated cancellation | green "Export cancelled" |

`finished` semantics are unchanged. `cancelled` is new.

### QML wiring

- `Main.qml` — in the `exportPdfDialog.onAccepted` handler where the exporter is wired up, after connecting `progress` and `finished`, also connect `exporter.cancelled` → `notificationOverlay.showCancelled()` and call `notificationOverlay.setActiveExporter(exporter)` so the Cancel button has a handle to call `requestCancel()` on.
- `NotificationOverlay.qml` — gains:
  - `property var activeExporter: null` and `function setActiveExporter(e) { activeExporter = e; }`.
  - A round red `Button` visible while `exporting`, sized like the pronunciation button, `onClicked: { enabled = false; if (activeExporter) activeExporter.requestCancel(); }`.
  - `function showCancelled() { exporting = false; activeExporter = null; showNotification("Export cancelled", false); }`.
  - `onExportingChanged` resets the Cancel button's `enabled` to `true` when a fresh export begins.

## Error handling

- If `requestCancel` is called *after* the pipeline has already emitted `finished`, it is a no-op (the flag flip is harmless and there is no reply to abort).
- If `QFile::remove` fails on the partial PDF (permission, already-removed race), `renderToFile` still returns and the `cancelled` signal still fires — the user already chose to cancel; we do not surface remove failures.
- Network abort is idempotent. `QNetworkReply::abort()` on a reply that has already emitted `finished` is a no-op.

## Testing

Catch2 tests in `tests/test_pdfexporter.cpp`:

1. `PdfExporter cancel during render returns false and removes partial file` — construct a PdfExporter with a synthetic entry set, start render in a worker thread, call `requestCancel()` shortly after, wait for `cancelled` via `QSignalSpy`, assert `QFile::exists(path) == false`.
2. `BaseExporter cancel before first fetch emits cancelled without emitting finished` — construct a trivial subclass with a stub renderer, call `exportToFile`, immediately call `requestCancel`, assert `cancelled` spy count 1, `finished` spy count 0.

Manual test plan:

- Export 300-word PDF → click Cancel during fetch phase → no partial PDF on disk, green notification, UI re-enabled within ~200 ms.
- Export PDF → click Cancel during render → same outcome.
- Export PDF → let it complete → no regression in the success path.
- Double-click Cancel → only one `cancelled` emission, no crash.
- Cancel button disappears after `exporting` flips false.

## Implementation order

1. Extend `BaseExporter` (flag, reply tracking, `requestCancel`, `cancelled` signal). Confirm existing tests still pass.
2. Wire fetch-phase cancel checks and reply abort.
3. Wire render-phase cancel checks in `PdfExporter::renderToFile`. Delete partial file.
4. Emit `cancelled` vs `finished` from the worker lambda.
5. Add the Cancel button and `showCancelled` helper in `NotificationOverlay.qml`.
6. Connect `exporter.cancelled` in `Main.qml`.
7. Add Catch2 tests.
