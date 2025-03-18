package ru.yandex.ci.core.launch.versioning;

import javax.annotation.Nonnull;

import lombok.Builder;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
public class Slot implements Comparable<Slot> {
    @Nonnull
    Version version;

    @With
    boolean inRelease;
    @With
    boolean inBranch;

    @Override
    public int compareTo(Slot o) {
        return version.compareTo(o.getVersion());
    }
}
