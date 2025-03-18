package ru.yandex.ci.storage.core.db.clickhouse.run_link;

import java.time.LocalDate;
import java.util.List;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

@Value
@AllArgsConstructor
@Builder
public class RunLinkEntity {
    LocalDate date;
    long runId;
    long testId;
    List<String> names;
    List<String> links;
}
