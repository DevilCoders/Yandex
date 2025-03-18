package ru.yandex.ci.engine.launch;

import ru.yandex.ci.engine.config.ConfigBundle;

public class ConfigHasSecurityProblemsException extends RuntimeException {
    public ConfigHasSecurityProblemsException(ConfigBundle configBundle) {
        super("Config [%s] has invalid status: %s".formatted(configBundle.getConfigPath(), configBundle.getStatus()));
    }
}
