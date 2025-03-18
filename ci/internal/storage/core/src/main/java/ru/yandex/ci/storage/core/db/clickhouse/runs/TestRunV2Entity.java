package ru.yandex.ci.storage.core.db.clickhouse.runs;

import java.time.LocalDate;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

@Value
@AllArgsConstructor
@Builder
public class TestRunV2Entity {
    LocalDate date;
    Long testId;
    Integer revision;
    Integer branchId;
    Long runId;
}
