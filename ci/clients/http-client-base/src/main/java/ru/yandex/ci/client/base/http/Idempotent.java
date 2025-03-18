package ru.yandex.ci.client.base.http;

import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.Target;

import static java.lang.annotation.ElementType.METHOD;
import static java.lang.annotation.RetentionPolicy.RUNTIME;

/**
 * Работает только со стандартными политиками retry из {@link RetryPolicies}
  */
@Documented
@Target(METHOD)
@Retention(RUNTIME)
public @interface Idempotent {
    boolean value() default true;
}
