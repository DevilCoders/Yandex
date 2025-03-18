package ru.yandex.ci.engine.discovery.filter;

import java.nio.file.Path;
import java.util.List;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.CsvFileSource;
import org.mockito.Mockito;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.config.a.model.FilterConfig;
import ru.yandex.ci.core.discovery.DiscoveryType;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.discovery.DiscoveryContext;
import ru.yandex.ci.util.GlobMatchers;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;
import static ru.yandex.ci.engine.discovery.filter.PathFilterProcessor.addDirectoryToBeginOfPaths;

class PathFilterProcessorTest {

    @ParameterizedTest
    @CsvFileSource(delimiter = ';', resources = "has-correct-abspath.csv")
    void hasCorrectAbsPath(String absPaths, String notAbsPaths, String affected, boolean expected) {
        boolean actual = GlobMatchers.pathsMatchGlobs(
                commaSeparatedStringToList(affected),
                commaSeparatedStringToList(absPaths),
                commaSeparatedStringToList(notAbsPaths)
        );
        assertThat(actual)
                .withFailMessage(
                        "input: (root path %s, affected paths %s), got: %s, expected: %s",
                        absPaths, affected, actual, expected
                )
                .isEqualTo(expected);
    }

    @Test
    void hasCorrectSubPathIncludePathWithExcludePath() {
        var affectedPaths = List.of("maps/a.yaml", "maps/libs/directions/testpalm/BlahBlah.yml");
        var dir = "maps/";
        boolean actual = GlobMatchers.pathsMatchGlobs(
                affectedPaths,
                addDirectoryToBeginOfPaths(dir, List.of("libs/auth/**", "libs/directions/**")),
                addDirectoryToBeginOfPaths(dir, List.of("libs/**/testpalm/*.yml"))
        );

        Assertions.assertFalse(actual);
    }

    @ParameterizedTest
    @CsvFileSource(delimiter = ';', resources = "has-correct-subpath.csv")
    void hasCorrectSubPath(String positiveSamples, String negativeSamples, String affectedPaths, String configPath,
                           boolean expected) {
        var bundle = Mockito.mock(ConfigBundle.class);
        when(bundle.getConfigPath()).thenReturn(Path.of(configPath));

        var discoveryContext = DiscoveryContext.builder()
                .revision(TestData.TRUNK_R1)
                .commit(TestData.TRUNK_COMMIT_1)
                .upstreamBranch(ArcBranch.trunk())
                .affectedPaths(commaSeparatedStringToList(affectedPaths))
                .discoveryType(DiscoveryType.DIR)
                .configBundle(bundle)
                .build();

        var filter = FilterConfig.builder()
                .subPaths(commaSeparatedStringToList(positiveSamples))
                .notSubPaths(commaSeparatedStringToList(negativeSamples))
                .build();

        assertThat(PathFilterProcessor.hasCorrectPaths(discoveryContext, filter)).isEqualTo(expected);
    }

    private List<String> commaSeparatedStringToList(@Nullable String source) {
        if (source == null) {
            return List.of();
        }
        return List.of(source.split(",\\s*"));
    }

}
