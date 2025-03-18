package ru.yandex.ci.storage.core.db.clickhouse.old_test;

import java.time.LocalDate;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.storage.core.clickhouse.sp.Result;

@Value
@AllArgsConstructor
@Builder
public class OldTestEntity {
    LocalDate date;
    long id;
    String strId;
    String toolchain;
    Result.TestType type;
    String path;
    String name;
    String subtestName;
    boolean suite;
    String suiteId;
}
