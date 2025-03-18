package ru.yandex.ci.core.config;

public interface ConfigIdEntry<T extends ConfigIdEntry<T>> {
    String getId();

    T withId(String id);

}
