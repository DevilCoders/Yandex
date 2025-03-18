package ru.yandex.ci.core.timeline;


import java.util.HashSet;
import java.util.Set;
import java.util.stream.Stream;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonSetter;
import com.fasterxml.jackson.annotation.Nulls;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.ydb.Persisted;

/**
 * Динамическая информация о процессах в релизе
 */

@Persisted
@Value
@Builder(toBuilder = true)
public class BranchState {
    @Singular
    Set<Integer> activeLaunches;
    @Singular
    Set<Integer> completedLaunches;

    @Singular
    @JsonSetter(nulls = Nulls.AS_EMPTY)
    Set<Integer> cancelledBranchLaunches;

    @Singular
    @JsonAlias("cancelledLaunches")
    Set<Integer> cancelledBaseLaunches;

    int freeCommits;
    @lombok.Builder.Default
    int lastLaunchNumber = -1;

    /**
     * Обновить состояние ветки согласно новому состоянию релиза.
     * @param launchNumber номер релиза
     * @param status новый статус релиза
     * @param inBaseBranch релиз может быть как непосредственно в ветке, так и на несколько коммитов ранее, тоже
     *                     относящихся к текущей ветке. Например, был запуск в транке на ревизии r1, далее на ревизии
     *                     r3 отвели ветку. В ветку войдут коммиты от r2 и r3. Однако после отмены релиза, коммиты ранее
     *                     r1 станут относится к ветке, и сам отмененный релиз должен быть зафиксирован как отмененный
     *                     в ветке. true - значит релиз обновлен в базовой ветке.
     * @return новое состояние ветки
     */
    public BranchState registerLaunch(int launchNumber, LaunchState.Status status, boolean inBaseBranch) {
        var active = new HashSet<>(activeLaunches);
        var completed = new HashSet<>(completedLaunches);
        var cancelled = new HashSet<>(cancelledBranchLaunches);
        var cancelledBase = new HashSet<>(cancelledBaseLaunches);

        Stream.of(active, completed, cancelled, cancelledBase)
                .forEach(c -> c.remove(launchNumber));

        boolean isTerminal = status.isTerminal();
        int updatedLastLaunch;

        if (!isTerminal) {
            active.add(launchNumber);
            updatedLastLaunch = Math.max(lastLaunchNumber, launchNumber);
        } else if (status == LaunchState.Status.CANCELED) {
            (inBaseBranch ? cancelledBase : cancelled).add(launchNumber);
            updatedLastLaunch = Stream.concat(active.stream(), completed.stream())
                    .max(Integer::compareTo)
                    .orElse(-1);
        } else if (status == LaunchState.Status.SUCCESS) {
            completed.add(launchNumber);
            updatedLastLaunch = Math.max(lastLaunchNumber, launchNumber);
        } else {
            throw new IllegalArgumentException("unexpected launch status %s".formatted(status));
        }

        return this.toBuilder()
                .clearActiveLaunches().activeLaunches(active)
                .clearCancelledBranchLaunches().cancelledBranchLaunches(cancelled)
                .clearCancelledBaseLaunches().cancelledBaseLaunches(cancelledBase)
                .clearCompletedLaunches().completedLaunches(completed)
                .lastLaunchNumber(updatedLastLaunch)
                .build();
    }

    public BranchState incrementNumberOfCommits(int number) {
        return this.toBuilder()
                .freeCommits(this.freeCommits +  number)
                .build();
    }
}
