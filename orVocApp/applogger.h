// orVocApp/applogger.h
#ifndef APPLOGGER_H
#define APPLOGGER_H

#include <QString>

/**
 * @brief Minimal file-backed failure log for diagnosing network/parse failures.
 *
 * Writes timestamped lines to @c app.log under QStandardPaths::AppDataLocation.
 * The log is truncated once at application startup (see @ref clear, called
 * from main.cpp) so each run starts fresh — it is meant for diagnosing the
 * *current* session's failures, not as a persistent audit trail.
 *
 * Used by NetworkClient (interactive lookup failures) and BaseExporter
 * (per-word fetch failures during PDF/JSON export) to record enough context
 * (word, phase, error, retry attempt) to diagnose a failed export after the
 * fact without attaching a debugger.
 */
class AppLogger
{
public:
    /// @brief Truncates the log file. Call once at application startup.
    static void clear();

    /**
     * @brief Appends a timestamped line to the log file.
     * @param message The message to record; a trailing newline is added.
     */
    static void log(const QString &message);

    /// @brief Returns the absolute path to the log file.
    static QString filePath();
};

#endif // APPLOGGER_H
