package ru.yandex.ci.core.launch;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import lombok.Value;
import org.apache.logging.log4j.util.Strings;

/**
 * Версия используемого тасклета.
 *
 * @see <a href="https://docs.yandex-team.ru/ci/job-tasklet">релизный процесс тасклетов</a>
 */
@Value
public class TaskVersion {
    private static final String STABLE_NAME = "stable";
    public static final TaskVersion STABLE = new TaskVersion(STABLE_NAME);

    String value;

    public static TaskVersion of(@Nonnull String value) {
        if (STABLE_NAME.equalsIgnoreCase(value)) {
            return STABLE;
        }
        Preconditions.checkArgument(Strings.isNotBlank(value));
        return new TaskVersion(value.toLowerCase());
    }

    @Override
    public String toString() {
        return value;
    }

}
