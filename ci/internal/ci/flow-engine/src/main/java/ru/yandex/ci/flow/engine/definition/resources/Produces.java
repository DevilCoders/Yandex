package ru.yandex.ci.flow.engine.definition.resources;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import com.google.protobuf.GeneratedMessageV3;

import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

/**
 * Аннотация, которая вешается на класс {@link JobExecutor}, тем самым определяя,
 * какие ресурсы он производит.
 */
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface Produces {
    /**
     * Джоба производит ровно один protobuf ресурс этого типа.
     *
     * @return ресурс в виде protobuf объекта
     */
    Class<? extends GeneratedMessageV3>[] single() default {};

    /**
     * Джоба производин множество protobuf ресурсов этого типа
     *
     * @return ресурс в виде protobuf объекта
     */
    Class<? extends GeneratedMessageV3>[] multiple() default {};
}
