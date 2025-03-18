package ru.yandex.ci.core.launch;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@AllArgsConstructor
@Builder(toBuilder = true)
@JsonIgnoreProperties(ignoreUnknown = true)
public class LaunchVcsInfo {
    /**
     * Ревизия, на которой запущен флоу.
     * Ветка и порядковый номер вычисляются и могут не совпадать с выбранной пользователем веткой.
     * Например, пользователь может запустить в ветке на ревизии, которая находится в транке.
     * В этом случае ревизия будет соответствовать trunk. Ветка, которую выбрал пользователь для запуска
     * доступна в поле branch.
     */
    @Nonnull
    OrderedArcRevision revision;

    @Nullable
    ArcCommit commit;

    /**
     * Ревизия, от которой необходимо в общем случае считать изменения
     */
    @Nullable
    OrderedArcRevision previousRevision;

    @Nullable
    LaunchPullRequestInfo pullRequestInfo;

    /**
     * -1 means unknown
     */
    @lombok.Builder.Default
    int commitCount = -1;

    @Nullable
    ReleaseVcsInfo releaseVcsInfo;

    /**
     * Ветка, на которой выполнен запуск. Может отличаться от ветки в revision.
     */
    @Nullable
    ArcBranch selectedBranch;

    @Nonnull
    public ArcBranch getSelectedBranch() {
        if (selectedBranch == null) {
            return revision.getBranch();
        }
        return selectedBranch;
    }
}
