package ru.yandex.ci.storage.core.db.model.check_task;

import java.util.List;
import java.util.Map;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.JsonNode;
import com.google.common.primitives.UnsignedLong;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

// All fields except REQUIRED are used to report test results in case of task failure
@Persisted
@Value
public class LargeTestInfo {
    Map<String, String> owners = Map.of(); // TODO: fill if required?

    // REQUIRED for starting tests
    String toolchain;

    // REQUIRED for starting tests
    List<String> tags;

    @JsonProperty("suite_name")
    String suiteName;

    @JsonProperty("suite_id")
    String suiteId;

    @JsonProperty("suite_hid")
    UnsignedLong suiteHid;

    // REQUIRED for starting tests
    JsonNode requirements;

    String size = "large"; // TODO: fill if required?
}
