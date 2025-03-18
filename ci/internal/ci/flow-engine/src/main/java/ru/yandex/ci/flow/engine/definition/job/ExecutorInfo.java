package ru.yandex.ci.flow.engine.definition.job;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

@Target(ElementType.TYPE)
@Retention(RetentionPolicy.RUNTIME)
public @interface ExecutorInfo {
    /**
     * Title
     *
     * @return Executor title.
     */
    String title() default "";

    /**
     * Description
     *
     * @return Executor description.
     */
    String description() default "";

    /**
     * This flag is set by infra team for approved common executors.
     *
     * @return True if recommended for use.
     */
    boolean recommended() default false;

    /**
     * Owner of the job. By default will extract from job's namespace.
     *
     * @return Owner of the job.
     */
    String owner() default "";

    /**
     * Whether job is deprecated.
     *
     * @return True if job is deprecated.
     */
    boolean deprecated() default false;
}
