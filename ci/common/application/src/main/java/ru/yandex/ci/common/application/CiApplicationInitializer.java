package ru.yandex.ci.common.application;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import org.springframework.context.ApplicationContextInitializer;
import org.springframework.context.ConfigurableApplicationContext;

/**
 * Используется для подготовки переменных окружения приложения,
 * в том числе в тестах.
 */
@RequiredArgsConstructor
public class CiApplicationInitializer implements ApplicationContextInitializer<ConfigurableApplicationContext> {
    private final String applicationType;
    private final String applicationName;

    @Override
    public void initialize(@Nonnull ConfigurableApplicationContext applicationContext) {
        CiApplication.init(applicationType, applicationName, applicationContext);
    }
}
