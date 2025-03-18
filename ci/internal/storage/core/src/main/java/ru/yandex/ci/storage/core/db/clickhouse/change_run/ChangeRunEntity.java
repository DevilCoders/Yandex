package ru.yandex.ci.storage.core.db.clickhouse.change_run;

import java.time.LocalDate;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.clickhouse.sp.Result;

@Value
@AllArgsConstructor
@Builder
public class ChangeRunEntity {
    LocalDate date;
    long testId;
    int revision;
    int branchId;
    Long runId;
    Result.Status status;
    Result.ErrorType errorType;
    boolean affected;
    long counter;
    boolean important;
    boolean muted;
}
