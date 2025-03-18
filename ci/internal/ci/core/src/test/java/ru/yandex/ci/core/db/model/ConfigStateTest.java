package ru.yandex.ci.core.db.model;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class ConfigStateTest {
    @Test
    void testConfigState_whenReleasesFieldIsNotSetInBuilder() {
        assertThat(ConfigState.builder().build().getReleases())
                .isNotNull();
    }

    @Test
    void testConfigState_whenFlowFieldIsNotSetInBuilder() {
        assertThat(ConfigState.builder().build().getActions())
                .isNotNull();
    }
}
