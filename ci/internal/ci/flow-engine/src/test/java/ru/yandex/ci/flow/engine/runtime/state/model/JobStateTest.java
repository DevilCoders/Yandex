package ru.yandex.ci.flow.engine.runtime.state.model;

import java.util.List;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class JobStateTest {

    @Test
    void isInProgress() {
        var jobState = new JobState();
        jobState.addLaunch(new JobLaunch(1, "", List.of(), List.of(StatusChange.forcedExecutorSucceeded())));
        assertThat(jobState.isInProgress()).isTrue();
    }
}
