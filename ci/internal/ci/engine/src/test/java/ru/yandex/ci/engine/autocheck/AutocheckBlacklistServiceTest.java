package ru.yandex.ci.engine.autocheck;

import java.util.List;
import java.util.Set;

import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ActiveProfiles;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;
import ru.yandex.ci.engine.spring.clients.StaffClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;

@ActiveProfiles(ArcClientTestConfig.CANON_ARC_PROFILE)
@ContextConfiguration(classes = {
        ArcClientTestConfig.class,
        AutocheckBlacklistService.class,
        StaffClientTestConfig.class,
})
class AutocheckBlacklistServiceTest extends CommonYdbTestBase {

    @Autowired
    ArcService arcService;

    @Autowired
    AutocheckBlacklistService autocheckBlacklistService;

    @Test
    void isOnlyBlacklistPathsAffected_shouldReturnTrue() {
        var revision = ArcRevision.of("884851e8d9508101f7c8a9ef9ab0d3efee593281");
        assertThat(autocheckBlacklistService.isOnlyBlacklistPathsAffected(revision)).isTrue();
    }

    @Test
    void isOnlyBlacklistPathsAffected_shouldReturnFalse() {
        var revision = ArcRevision.of("1dc043f3978330e123d2947f2c91e77cb0bd7b98");
        assertThat(autocheckBlacklistService.isOnlyBlacklistPathsAffected(revision)).isFalse();
    }

    @Test
    void isOnlyBlacklistPathsAffected_whenOneBlacklistEntryCoversAllPaths() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of("ab/1", "ab/2", "ab/3"), Set.of("ab")
        )).isTrue();

        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of("ab"), Set.of("ab")
        )).isTrue();
    }

    @Test
    void isOnlyBlacklistPathsAffected__whenOneBlacklistEntryNotCoversAllPaths() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of("ab/1", "ab/2", "ab/3"), Set.of("ab/2")
        )).isFalse();
    }

    @Test
    void isOnlyBlacklistPathsAffected_whenTwoBlacklistEntriesCoversAllPaths() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of("ab/1", "ab/2", "ab/3", "junk"), Set.of("ab", "junk")
        )).isTrue();
    }

    @Test
    void isOnlyBlacklistPathsAffected__whenTwoBlacklistEntriesNotCoversAllPaths() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of("ab/1", "ab/2", "ab/3", "junk", "contrib"), Set.of("ab", "junk")
        )).isFalse();
    }

    @Test
    void isOnlyBlacklistPathsAffected__whenOnlyRootDirectoryAffected() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of(""), Set.of("ab", "junk")
        )).isFalse();
    }

    @Test
    void isOnlyBlacklistPathsAffected__whenDirectoryListIsEmpty() {
        assertThat(AutocheckBlacklistService.isOnlyBlacklistPathsAffected(
                List.of(), Set.of("ab", "junk")
        )).isFalse();
    }

    @Test
    void pathStartsWith() {
        assertThat(AutocheckBlacklistService.pathStartsWith("ab", "ab")).isTrue();
        assertThat(AutocheckBlacklistService.pathStartsWith("ab/cd", "ab")).isTrue();
        assertThat(AutocheckBlacklistService.pathStartsWith("abc/d", "ab")).isFalse();
    }

    @Test
    void parseAutoCheckBlacklist() {
        var content = "" +
                "# Autocheck blacklist\n" +
                " # comment\n" +
                "          \n" +
                "direct/frontend/\n" +
                " junk\n" +
                "/kinopoisk/frontend\n" +
                "      \n" +
                "mobile # comment\n" +
                "\n";

        assertThat(AutocheckBlacklistService.parseBlacklist(content)).containsExactlyInAnyOrder(
                "direct/frontend",
                "junk",
                "kinopoisk/frontend",
                "mobile"
        );
    }

}
