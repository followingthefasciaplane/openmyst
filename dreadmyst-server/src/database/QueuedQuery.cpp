#include "database/QueuedQuery.h"

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

QueuedQuery::QueuedQuery() = default;

QueuedQuery::~QueuedQuery() {
    stop();
}

// ---------------------------------------------------------------------------
// Start / stop
// ---------------------------------------------------------------------------

void QueuedQuery::start(MySQLConnector* connector) {
    if (m_running.load()) return;

    m_connector = connector;
    m_running.store(true);
    m_workerThread = std::thread(&QueuedQuery::workerLoop, this);
    LOG_INFO("QueuedQuery: Worker thread started");
}

void QueuedQuery::stop() {
    if (!m_running.load()) return;

    m_running.store(false);
    m_queueCV.notify_all();

    if (m_workerThread.joinable()) {
        m_workerThread.join();
    }
    LOG_INFO("QueuedQuery: Worker thread stopped");
}

// ---------------------------------------------------------------------------
// Enqueue
// ---------------------------------------------------------------------------

void QueuedQuery::push(const std::string& sql, QueryCallback callback) {
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_queue.push(PendingQuery{sql, std::move(callback)});
    }
    m_queueCV.notify_one();
}

void QueuedQuery::pushExecute(const std::string& sql) {
    push(sql, nullptr);
}

size_t QueuedQuery::pendingCount() const {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    return m_queue.size();
}

// ---------------------------------------------------------------------------
// Callback delivery (main thread)
// ---------------------------------------------------------------------------

void QueuedQuery::processCallbacks() {
    std::queue<CompletedCallback> batch;
    {
        std::lock_guard<std::mutex> lock(m_callbackMutex);
        batch.swap(m_completedCallbacks);
    }

    while (!batch.empty()) {
        auto& cb = batch.front();
        if (cb.callback) {
            try {
                cb.callback(std::move(cb.results));
            } catch (const std::exception& ex) {
                LOG_ERROR("QueuedQuery: Exception in callback: %s", ex.what());
            }
        }
        batch.pop();
    }
}

// ---------------------------------------------------------------------------
// Worker thread
// ---------------------------------------------------------------------------

void QueuedQuery::workerLoop() {
    while (m_running.load()) {
        PendingQuery pq;

        // Wait for work
        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueCV.wait(lock, [this] {
                return !m_queue.empty() || !m_running.load();
            });

            if (!m_running.load() && m_queue.empty())
                break;

            if (m_queue.empty())
                continue;

            pq = std::move(m_queue.front());
            m_queue.pop();
        }

        // Execute
        if (!m_connector) {
            LOG_ERROR("QueuedQuery: No connector available, dropping query");
            continue;
        }

        if (pq.callback) {
            // Query with result set
            ResultSet results;
            if (!m_connector->query(pq.sql, results)) {
                LOG_ERROR("QueuedQuery: Query failed: %s", pq.sql.c_str());
            }

            // Enqueue callback for main-thread delivery
            {
                std::lock_guard<std::mutex> lock(m_callbackMutex);
                m_completedCallbacks.push(
                    CompletedCallback{std::move(pq.callback), std::move(results)});
            }
        } else {
            // Fire-and-forget execute
            if (!m_connector->execute(pq.sql)) {
                LOG_ERROR("QueuedQuery: Execute failed: %s", pq.sql.c_str());
            }
        }
    }

    // Drain remaining queries before exiting
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        while (!m_queue.empty()) {
            auto& pq = m_queue.front();
            if (m_connector) {
                if (pq.callback) {
                    ResultSet results;
                    m_connector->query(pq.sql, results);
                    std::lock_guard<std::mutex> cbLock(m_callbackMutex);
                    m_completedCallbacks.push(
                        CompletedCallback{std::move(pq.callback), std::move(results)});
                } else {
                    m_connector->execute(pq.sql);
                }
            }
            m_queue.pop();
        }
    }
}
