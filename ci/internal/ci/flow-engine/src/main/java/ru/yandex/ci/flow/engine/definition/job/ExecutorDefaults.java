package ru.yandex.ci.flow.engine.definition.job;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

/**
 * Аннотация, которая вешается на класс {@link JobExecutor}.
 */
@Target(ElementType.TYPE_PARAMETER)
@Retention(RetentionPolicy.RUNTIME)
public @interface ExecutorDefaults {
    int retries() default 0;
}
