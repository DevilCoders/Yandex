#pragma once

enum class EProcessState {
    RUNNING /* "R" */,
    INTERRUPTIBLE_SLEEPING /* "S" */,
    UNINTERRUPTIBLE_SLEEPING /* "D" */,
    ZOMBIE /* "Z" */,
    STOPPED /* "T" */,
    TRACING /* "t" */,
    DEAD /* "X" */,
    UNKNOWN,
};
