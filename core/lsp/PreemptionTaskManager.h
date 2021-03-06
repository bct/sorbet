#ifndef SORBET_LSP_PREEMPTIONTASKMANAGER_H
#define SORBET_LSP_PREEMPTIONTASKMANAGER_H

#include "core/core.h"
#include <memory>

namespace sorbet::core::lsp {
class Task;
class TypecheckEpochManager;
class PreemptionTaskManager final {
private:
    // Used to pre-empt typechecking (post-resolver).
    // - Worker threads grab as a reader lock, and routinely gives up and re-acquire the lock to allow other requests to
    // pre-empt.
    // - Typechecking coordinator thread grabs as a writer lock when there's a preemption function, which halts all
    // worker threads.
    mutable absl::Mutex typecheckMutex;
    std::shared_ptr<Task> preemptTask;
    std::shared_ptr<TypecheckEpochManager> epochManager;
    // Thread ID of the typechecking thread. Lazily set.
    std::optional<std::thread::id> typecheckingThreadId;
    // Thread ID of the message processing thread. Lazily set.
    std::optional<std::thread::id> messageProcessingThreadId;

public:
    PreemptionTaskManager(std::shared_ptr<TypecheckEpochManager> epochManager);
    // Run only from message processing thread.
    // Attempts to preempt a running slow path to run the provided task. If it returns true, the task is guaranteed
    // to run.
    bool trySchedulePreemptionTask(std::shared_ptr<Task> task);
    // Run only from the typechecking thread.
    // Runs the scheduled preemption task, if any.
    bool tryRunScheduledPreemptionTask();
    // Run only from typechecker worker threads. Prevents preemption from occurring while the ReaderMutexLock is alive.
    std::unique_ptr<absl::ReaderMutexLock> lockPreemption() const;
    // (For testing only) Assert that typecheckMutex is held.
    void assertTypecheckMutexHeld();
};

} // namespace sorbet::core::lsp
#endif