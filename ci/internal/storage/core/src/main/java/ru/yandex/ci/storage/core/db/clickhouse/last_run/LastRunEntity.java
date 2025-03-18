package ru.yandex.ci.storage.core.db.clickhouse.last_run;

import java.time.LocalDate;
import java.util.List;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.clickhouse.sp.Result;

@Value
@AllArgsConstructor
@Builder
public class LastRunEntity {
    LocalDate date;
    boolean active;
    long testId;
    int branchId;
    int revision;
    long runId;
    long counter;

    String strId;
    String toolchain;
    Result.TestType type;
    String path;
    String name;
    String subtestName;
    boolean suite;
    String suiteId;

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
    List<String> linkNames;
    List<String> links;
    List<String> metricKeys;
    List<Double> metricValues;
    String results;
}
