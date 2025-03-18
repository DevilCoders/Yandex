package ru.yandex.ci.storage.core.db.clickhouse.runs;

import java.time.LocalDate;
import java.util.List;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.clickhouse.sp.Result;

@Value
@AllArgsConstructor
@Builder
public class TestRunEntity {
    LocalDate date;
    long id;
    long taskId;
    long testId;
    Result.Status status;
    Result.ErrorType errorType;
    List<String> ownerGroups;
    List<String> ownerLogins;
    String uid;
    String snippet;
    Double duration;
    Result.TestSize testSize;
    List<String> tags;
    String requirements;
    List<String> metricKeys;
    List<Double> metricValues;
    String results;
}
