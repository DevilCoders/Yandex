package ru.yandex.ci.storage.core.db.annotations;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;

import yandex.cloud.repository.db.Entity;

@Target({ElementType.FIELD})
@Retention(RetentionPolicy.RUNTIME)
public @interface IndirectReference {
    Class<? extends Entity.Id<?>> id();
}

