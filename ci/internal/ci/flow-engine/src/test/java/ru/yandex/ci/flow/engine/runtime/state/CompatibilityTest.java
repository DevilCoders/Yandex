package ru.yandex.ci.flow.engine.runtime.state;

import java.io.IOException;

import com.jayway.jsonpath.internal.JsonFormatter;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.util.CiJson;
import ru.yandex.ci.util.gson.CiGson;

import static org.assertj.core.api.Assertions.assertThat;

@Slf4j
public class CompatibilityTest {

    @Test
    void testConversion() throws IOException {
        var jacksonMapper = CiJson.mapper();
        var gsonMapper = CiGson.instance();

        var json = TestUtils.textResource("ydb-data/expect.json");
        var gsonObject = gsonMapper.fromJson(json, JobState.class);

        var gsonJson = jacksonMapper.writer().writeValueAsString(gsonObject);
        log.info("From GSON:\n{}", gsonJson);

        var jacksonObject = jacksonMapper.reader().readValue(json, JobState.class);
        var jacksonJson = jacksonMapper.writer().writeValueAsString(jacksonObject);
        log.info("From Jackson:\n{}", jacksonJson);

        var jsonFromJackson = gsonMapper.toJson(jacksonObject);

        log.info("Expected:\n{}", JsonFormatter.prettyPrint(json));
        log.info("Actual:\n{}", JsonFormatter.prettyPrint(jsonFromJackson));

        assertThat(jacksonObject)
                .isEqualTo(gsonObject);
    }
}
