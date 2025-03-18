package ru.yandex.ci.core.timeline;


import java.time.Instant;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.arc.branch.BranchInfo;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder(toBuilder = true)
public class BranchVcsInfo {
    /**
     * Количество коммитов в базовой ветке, относящиеся тем не менее к текущей ветке, хотя ветка и отведена на
     * ревизии позже. Количество коммитов до первого активного релиза/ветки в базовой ветке (читай - транке).
     */
    @lombok.Builder.Default
    int trunkCommitCount = -1;

    /**
     * Количество коммитов, добавленных непосредственно в ветку. Иными словами, количество коммитов от места,
     * где отвели ветку, до ее head.
     */
    @lombok.Builder.Default
    int branchCommitCount = -1;

    OrderedArcRevision head;

    /**
     * Ревизия, от которой считаются изменения.
     * <p>
     * Не путать с базовой ревизией ветки {@link BranchInfo#getBaseRevision()}
     */
    @Nullable
    OrderedArcRevision previousRevision;

    Instant updatedDate;
}
