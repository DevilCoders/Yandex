package ru.yandex.ci.core.launch;

import javax.annotation.Nullable;

import ru.yandex.ci.ydb.Persisted;

@Persisted
public enum Activity {
    /**
     * Релиз/экшен активен
     */
    ACTIVE,
    /**
     * Активен, но есть упавшие кубики
     */
    ACTIVE_WITH_PROBLEMS,
    /**
     * Завершен успешно, в том числе отменен
     */
    FINISHED,
    /**
     * Упал. Он может перейти в состояние завершен. А может и не перейти, если пользователь забил на упавший экшен.
     */
    FAILED;

    @Nullable
    public static Activity fromStatus(@Nullable LaunchState.Status status) {
        if (status == null) {
            return null;
        }
        return switch (status) {
            case SUCCESS, CANCELED -> FINISHED;
            case FAILURE -> FAILED;
            case RUNNING_WITH_ERRORS -> ACTIVE_WITH_PROBLEMS;
            default -> ACTIVE;
        };
    }
}
