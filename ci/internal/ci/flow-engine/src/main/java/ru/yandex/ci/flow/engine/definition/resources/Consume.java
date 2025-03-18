package ru.yandex.ci.flow.engine.definition.resources;

import java.lang.annotation.ElementType;
import java.lang.annotation.Repeatable;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import com.google.protobuf.GeneratedMessageV3;

import ru.yandex.ci.flow.engine.definition.job.JobExecutor;

/**
 * Аннотация, которая вешается на класс {@link JobExecutor}, тем самым определяя,
 * какие ресурсы он потребляет.
 */
@Repeatable(Consumes.class)
@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface Consume {

    /**
     * Name of proto field
     *
     * @return proto field name (in snake_case)
     */
    String name();

    /**
     * Прото-объект, который описывает значение
     *
     * @return ресурс в виде protobuf объекта
     */
    Class<? extends GeneratedMessageV3> proto();

    /**
     * @return true if expect multiple values (false if single)
     */
    boolean list() default false;
}
