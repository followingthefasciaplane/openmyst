#pragma once
#include "core/Types.h"
#include "core/Log.h"
#include "database/MySQLConnector.h"

#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <functional>

// A single pending query: the SQL text and an optional callback that receives
// the result set.  When no callback is provided the query is fire-and-forget.
struct PendingQuery {
    std::string   sql;
    QueryCallback callback;  // may be nullptr for fire-and-forget
};

// Asynchronous database query queue.
//
// Queries are pushed from any thread and a dedicated worker thread drains
// the queue, executes each query against the provided MySQLConnector, and
// invokes the associated callback (if any) with the result set.
//
// This matches the original binary's queued query pattern used for
// non-blocking database operations (player saves, mail delivery, etc.).
class QueuedQuery {
public:
    QueuedQuery();
    ~QueuedQuery();

    // Non-copyable
    QueuedQuery(const QueuedQuery&) = delete;
    QueuedQuery& operator=(const QueuedQuery&) = delete;

    // Start the worker thread with the given database connector.
    // The connector must already be connected and must outlive the
    // QueuedQuery instance.
    void start(MySQLConnector* connector);

    // Signal the worker to stop and join the thread.
    void stop();

    // Push a query onto the queue.  Thread-safe.
    void push(const std::string& sql, QueryCallback callback = nullptr);

    // Push a fire-and-forget execute (no result set expected).
    void pushExecute(const std::string& sql);

    // Returns the number of queries currently waiting in the queue.
    size_t pendingCount() const;

    // Returns true while the worker thread is running.
    bool isRunning() const { return m_running.load(); }

    // Process any finished callbacks on the caller's thread.
    // The original binary dispatches callbacks back to the main thread;
    // call this from the game tick to deliver results safely.
    void processCallbacks();

private:
    void workerLoop();

    MySQLConnector*          m_connector = nullptr;
    std::thread              m_workerThread;
    std::atomic<bool>        m_running{false};

    // Inbound queue (any thread -> worker)
    mutable std::mutex       m_queueMutex;
    std::condition_variable  m_queueCV;
    std::queue<PendingQuery> m_queue;

    // Completed callbacks (worker -> main thread)
    struct CompletedCallback {
        QueryCallback callback;
        ResultSet     results;
    };
    std::mutex                       m_callbackMutex;
    std::queue<CompletedCallback>    m_completedCallbacks;
};
