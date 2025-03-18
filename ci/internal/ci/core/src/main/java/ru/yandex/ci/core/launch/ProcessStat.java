package ru.yandex.ci.core.launch;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.RequiredArgsConstructor;
import lombok.With;

@Data
@RequiredArgsConstructor
@AllArgsConstructor
public class ProcessStat {
    @Nonnull
    private final String processId;
    @Nonnull
    private final String project;
    @Nonnull
    private final Activity activity;

    @With
    long count;

    @With
    long withRetries;

    ProcessStat merge(ProcessStat stat) {
        count += stat.getCount();
        withRetries += stat.getWithRetries();
        return this;
    }

}
