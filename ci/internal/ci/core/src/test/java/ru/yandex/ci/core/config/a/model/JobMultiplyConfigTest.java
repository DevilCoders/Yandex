package ru.yandex.ci.core.config.a.model;


import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.util.CiJson;

import static org.assertj.core.api.Assertions.assertThat;

class JobMultiplyConfigTest {

    @Test
    void testCompatibility() throws JsonProcessingException {
        var json = """
                {
                "by": "${tasks.task}",
                "maxJobs": 1,
                "field": "ci.tasklet"
                }""";

        var config = CiJson.mapper().readValue(json, JobMultiplyConfig.class);
        assertThat(config)
                .isEqualTo(JobMultiplyConfig.of("${tasks.task}", null, null, 1, "ci.tasklet"));
    }

    @Test
    void testGetter0() {
        assertThat(JobMultiplyConfig.of("", null, null, 0, null).getMaxJobs())
                .isEqualTo(JobMultiplyConfig.DEFAULT_JOBS);
    }

    @Test
    void testGetter100() {
        assertThat(JobMultiplyConfig.of("", null, null, 100, null).getMaxJobs())
                .isEqualTo(JobMultiplyConfig.MAX_JOBS);
    }

}
