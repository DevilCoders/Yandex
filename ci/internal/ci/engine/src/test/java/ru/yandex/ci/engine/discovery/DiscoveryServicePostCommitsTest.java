package ru.yandex.ci.engine.discovery;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.assertj.core.util.Sets;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.model.FilterConfig;

import static org.assertj.core.api.Assertions.assertThat;

class DiscoveryServicePostCommitsTest {

    @ParameterizedTest
    @MethodSource
    void collectDirDiscovery(FilterConfig filter, Set<String> expectPaths) {
        Set<String> paths = new HashSet<>();
        DiscoveryServicePostCommits.collectAbsPathPrefixes(filter, paths);
        assertThat(paths).isEqualTo(expectPaths);
    }

    static List<Arguments> collectDirDiscovery() {
        return List.of(
                Arguments.of(
                        FilterConfig.builder().build(),
                        Sets.newLinkedHashSet()),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.GRAPH)
                                .absPath("a/b/c")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c")),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .absPath("a/b/c")
                                .absPath("a/c")
                                .notAbsPath("a/b/c")
                                .build(),
                        Sets.newLinkedHashSet("a/c")),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .absPath("a/b/c")
                                .notAbsPath("a/b/*")
                                .build(),
                        Sets.newLinkedHashSet()),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .absPath("a/b/c")
                                .notAbsPath("a/**")
                                .build(),
                        Sets.newLinkedHashSet()),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.DIR)
                                .absPath("a/b/c")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c")),
                Arguments.of(
                        FilterConfig.builder()
                                .discovery(FilterConfig.Discovery.ANY)
                                .absPath("a/b/c")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c1") // All choices
                                .absPath("a/b/c2")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c1", "a/b/c2")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c1/a.yaml") // Files are OK
                                .absPath("a/b/c2")
                                .build(),
                        Sets.newLinkedHashSet("a/b/c1/a.yaml", "a/b/c2")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b1/**") // Simple dir globs are OK
                                .absPath("a/b2/*")
                                .absPath("a/b3/?")
                                .absPath("a/b4/[ab]")
                                .absPath("a/b5/{ab}")
                                .build(),
                        Sets.newLinkedHashSet("a/b1", "a/b2", "a/b3", "a/b4", "a/b5")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c*") // Complex globs are OK
                                .build(),
                        Sets.newLinkedHashSet("a/b")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c?") // Complex globs are OK
                                .build(),
                        Sets.newLinkedHashSet("a/b")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c[]") // Complex globs are OK
                                .build(),
                        Sets.newLinkedHashSet("a/b")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c{}") // Complex globs are OK
                                .build(),
                        Sets.newLinkedHashSet("a/b")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/b/c**") // Complex globs are OK
                                .build(),
                        Sets.newLinkedHashSet("a/b")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a/*")
                                .build(),
                        Sets.newLinkedHashSet("a")),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("a*") // Empty
                                .build(),
                        Sets.newLinkedHashSet()),
                Arguments.of(
                        FilterConfig.builder()
                                .absPath("*") // Empty
                                .build(),
                        Sets.newLinkedHashSet())
        );
    }
}
