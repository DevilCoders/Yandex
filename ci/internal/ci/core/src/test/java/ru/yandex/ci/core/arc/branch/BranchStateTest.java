package ru.yandex.ci.core.arc.branch;

import java.util.List;
import java.util.Set;
import java.util.stream.Stream;

import org.assertj.core.util.Sets;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.launch.LaunchState.Status;
import ru.yandex.ci.core.timeline.BranchState;

import static org.assertj.core.api.Assertions.assertThat;

class BranchStateTest {

    @ParameterizedTest(name = "{0}, inBaseBranch = {1}")
    @MethodSource
    void registerLaunch(Status status,
                        boolean inBaseBranch,
                        Set<Integer> active,
                        Set<Integer> cancelled,
                        Set<Integer> cancelledBase,
                        Set<Integer> completed) {

        BranchState original = BranchState.builder()
                .activeLaunches(List.of(1, 2, 3))
                .cancelledBranchLaunches(List.of(4, 5, 6))
                .cancelledBaseLaunches(List.of(7, 8, 9))
                .completedLaunches(List.of(10, 11, 12))
                .build();

        BranchState state = original.registerLaunch(5, status, inBaseBranch);

        assertThat(state.getActiveLaunches()).isEqualTo(active);
        assertThat(state.getCancelledBranchLaunches()).isEqualTo(cancelled);
        assertThat(state.getCancelledBaseLaunches()).isEqualTo(cancelledBase);
        assertThat(state.getCompletedLaunches()).isEqualTo(completed);
    }

    static Stream<Arguments> registerLaunch() {
        return Stream.of(
                Arguments.of(Status.RUNNING, false,
                        Sets.newLinkedHashSet(1, 2, 3, 5),
                        Sets.newLinkedHashSet(4, 6),
                        Sets.newLinkedHashSet(7, 8, 9),
                        Sets.newLinkedHashSet(10, 11, 12)),
                Arguments.of(Status.CANCELED, false,
                        Sets.newLinkedHashSet(1, 2, 3),
                        Sets.newLinkedHashSet(4, 5, 6),
                        Sets.newLinkedHashSet(7, 8, 9),
                        Sets.newLinkedHashSet(10, 11, 12)),
                Arguments.of(Status.CANCELED, true,
                        Sets.newLinkedHashSet(1, 2, 3),
                        Sets.newLinkedHashSet(4, 6),
                        Sets.newLinkedHashSet(5, 7, 8, 9),
                        Sets.newLinkedHashSet(10, 11, 12)),
                Arguments.of(Status.SUCCESS, false,
                        Sets.newLinkedHashSet(1, 2, 3),
                        Sets.newLinkedHashSet(4, 6),
                        Sets.newLinkedHashSet(7, 8, 9),
                        Sets.newLinkedHashSet(5, 10, 11, 12)),
                Arguments.of(Status.WAITING_FOR_MANUAL_TRIGGER, false,
                        Sets.newLinkedHashSet(1, 2, 3, 5),
                        Sets.newLinkedHashSet(4, 6),
                        Sets.newLinkedHashSet(7, 8, 9),
                        Sets.newLinkedHashSet(10, 11, 12))
        );
    }
}
