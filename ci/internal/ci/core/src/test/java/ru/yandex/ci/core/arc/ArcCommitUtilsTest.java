package ru.yandex.ci.core.arc;

import java.util.List;
import java.util.Optional;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import static org.assertj.core.api.Assertions.assertThat;

public class ArcCommitUtilsTest {

    @ParameterizedTest
    @MethodSource
    void parsePullRequestId(String message, @Nullable Integer reviewId) {
        assertThat(ArcCommitUtils.parsePullRequestId(message))
                .isEqualTo(Optional.ofNullable(reviewId));
    }

    static Stream<Arguments> parsePullRequestId() {
        return Stream.of(
                Arguments.of("commit message REVIEW: 1234", 1234),
                Arguments.of("commit message REVIEW: 1234 REVIEW: 4567", 1234),
                Arguments.of("commit message REVIEW: no", null)
        );
    }

    @ParameterizedTest
    @MethodSource
    void parseTickets(String message, List<String> tickets) {
        assertThat(ArcCommitUtils.parseTickets(message))
                .containsExactlyInAnyOrderElementsOf(tickets);
    }


    @ParameterizedTest
    @MethodSource
    void parseStQueues(String message, List<String> queues) {
        assertThat(ArcCommitUtils.parseStQueues(message))
                .containsExactlyInAnyOrderElementsOf(queues);
    }

    @ParameterizedTest
    @MethodSource
    void messageWithoutPullRequestId(String message, String expected) {
        assertThat(ArcCommitUtils.messageWithoutPullRequestId(message))
                .isEqualTo(expected);
    }

    @ParameterizedTest
    @MethodSource
    void cleanupMessage(String message, String expected) {
        assertThat(ArcCommitUtils.cleanupMessage(message))
                .isEqualTo(expected);
    }

    static Stream<Arguments> parseTickets() {
        return Stream.of(
                Arguments.of("commit message CI-796 message CI-803", List.of("CI-796", "CI-803")),
                Arguments.of("commit message", List.of())
        );
    }

    static Stream<Arguments> parseStQueues() {
        return Stream.of(
                Arguments.of("commit message TESTENV-803", List.of("TESTENV")),
                Arguments.of("commit message CI-796 message TESTENV-803", List.of("CI", "TESTENV")),
                Arguments.of("st-queue length less than 2: C-796", List.of())
        );
    }

    static Stream<Arguments> messageWithoutPullRequestId() {
        return Stream.of(
                Arguments.of("ci: fix up sandbox examples \n\nREVIEW: 1308821", "ci: fix up sandbox examples \n\n"),
                Arguments.of("just message", "just message")
        );
    }

    static Stream<Arguments> cleanupMessage() {
        return Stream.of(
                Arguments.of("just message", "just message"),
                Arguments.of("just message\n  ", "just message"),
                Arguments.of("\n  just message\n  ", "just message"),
                Arguments.of("\tjust message", "just message"),
                Arguments.of("message with ticket CI-123", "message with ticket CI-123"),
                Arguments.of("message with ticket\n\nCI-123", "message with ticket\n\nCI-123"),
                Arguments.of("ci: fix up sandbox examples \n\nREVIEW: 1308821", "ci: fix up sandbox examples"),
                Arguments.of("only <!-- DEVEXP BEGIN --> tag", "only <!-- DEVEXP BEGIN --> tag"),
                Arguments.of("only <!-- DEVEXP END --> tag", "only <!-- DEVEXP END --> tag"),
                Arguments.of(
                        "swapped tags <!-- DEVEXP END --> review <!-- DEVEXP BEGIN -->",
                        "swapped tags <!-- DEVEXP END --> review <!-- DEVEXP BEGIN -->"
                ),
                Arguments.of(
                        """
                                CI-1196 Allow * in target paths for large autostart

                                <!-- DEVEXP BEGIN -->
                                ![review](https://codereview.yandex-team.ru/badges/review-complete-green.svg) \
                                [![andreevdm](https://codereview.yandex-team.ru/badges/andreevdm-ok-green.svg)]\
                                (https://staff.yandex-team.ru/andreevdm)
                                <!-- DEVEXP END -->""",
                        "CI-1196 Allow * in target paths for large autostart"
                ),
                Arguments.of("""
                                head
                                <!-- DEVEXP BEGIN -->
                                first
                                <!-- DEVEXP END -->
                                middle text<!-- DEVEXP BEGIN -->
                                second<!-- DEVEXP END -->
                                trailing""",
                        "head\n\nmiddle text\ntrailing"
                )
        );
    }
}
