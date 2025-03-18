package ru.yandex.ci.common.application;

import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.context.annotation.PropertySource;

@Configuration
@Import(PropertiesConfig.Default.class)
public class PropertiesConfig {

    // Secrets in classpath, for all apps
    @PropertySource("classpath:temporal/temporal-default.properties")
    @PropertySource("classpath:ci-core/ci-common.properties")
    @PropertySource("classpath:ci-core/ci-common-${spring.profiles.active}.properties")
    @PropertySource(
            value = "classpath:ci-storage-core/ci-storage.properties",
            ignoreResourceNotFound = true)
    @PropertySource(
            value = "classpath:ci-storage-core/ci-storage-${spring.profiles.active}.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource(
            value = "classpath:ci-observer-core/ci-observer.properties",
            ignoreResourceNotFound = true)
    @PropertySource(
            value = "classpath:ci-observer-core/ci-observer-${spring.profiles.active}.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource("classpath:${app.name}.properties")
    @PropertySource("classpath:${app.name}-${spring.profiles.active}.properties")

    // Secrets in Deploy container
    @PropertySource(
            value = "file:/secrets/ci-common.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource(
            value = "file:/secrets/${app.type}.properties",
            ignoreResourceNotFound = true
    )

    // Properties and secrets in local env
    @PropertySource(
            value = "file:${user.home}/.ci/ci-common-${app.secrets.profile}.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource(
            value = "file:${user.home}/.ci/${app.type}-${app.secrets.profile}.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource(
            value = "file:${user.home}/.ci/secrets/ci-common-${app.secrets.profile}.properties",
            ignoreResourceNotFound = true
    )
    @PropertySource(
            value = "file:${user.home}/.ci/secrets/${app.type}-${app.secrets.profile}.properties",
            ignoreResourceNotFound = true
    )
    static class Default {
    }

}
