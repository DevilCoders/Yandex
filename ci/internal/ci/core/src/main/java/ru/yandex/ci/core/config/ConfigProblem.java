package ru.yandex.ci.core.config;

import java.util.List;

import lombok.Value;

import ru.yandex.ci.util.ExceptionUtils;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
public class ConfigProblem {

    Level level;
    String title;
    String description;

    public static ConfigProblem crit(String title, String description) {
        return new ConfigProblem(Level.CRIT, title, description);
    }

    public static ConfigProblem crit(String title) {
        return crit(title, "");
    }

    public static ConfigProblem crit(Exception e) {
        return ConfigProblem.crit("Exception while processing config: " + e.getMessage(),
                ExceptionUtils.getStackTrace(e));
    }

    public static ConfigProblem warn(String title, String description) {
        return new ConfigProblem(Level.WARN, title, description);
    }

    public static ConfigProblem warn(String title) {
        return warn(title, "");
    }


    @Persisted
    public enum Level {
        CRIT,
        WARN
    }

    public static boolean isValid(List<ConfigProblem> problems) {
        return problems.stream().noneMatch(p -> p.getLevel() == ConfigProblem.Level.CRIT);
    }
}
