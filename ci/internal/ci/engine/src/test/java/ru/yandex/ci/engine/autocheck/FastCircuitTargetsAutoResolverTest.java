package ru.yandex.ci.engine.autocheck;

import java.nio.file.Path;
import java.time.Duration;
import java.util.Collections;
import java.util.Optional;
import java.util.Set;
import java.util.function.Supplier;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.MockBean;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.context.annotation.Import;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.BDDMockito.given;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;

@ContextConfiguration(classes = {
        FastCircuitTargetsAutoResolverTest.Config.class
})
public class FastCircuitTargetsAutoResolverTest extends CommonTestBase {

    @MockBean
    private ArcService arcService;

    @MockBean
    private CiMainDb db;

    @Autowired
    private FastCircuitTargetsAutoResolver fastCircuitTargetsAutoResolver;

    @BeforeEach
    public void setup() {
        // remove the config cached by previous test
        fastCircuitTargetsAutoResolver.clearCache();
        // turn on the auto-target detection feature for each call
        given(db.currentOrReadOnly(any(Supplier.class))).willReturn(100);
    }

    @Test
    public void shouldGetAllItemsUnderPartition() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b", "c"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a")))).contains("a");
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("b")))).contains("b");
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("c")))).contains("c");
    }

    @Test
    public void neverReturnsMultipleTargets() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b", "c"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a"), Path.of("b"))))
                .isEmpty();
    }

    @Test
    public void multiplePathsResolvedToTheSameRoot() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a/b"), Path.of("a/c/d"))))
                .contains("a");
    }

    @Test
    public void supportsEmptyFileNames() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b", "c"]
                    }
                  }
                }
                """);
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("/x/y/z")))).isEmpty();
    }

    @Test
    public void shouldCollectItemsFromAllPartitions() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "2": {
                      "0": ["a", "b"],
                      "1": ["x", "y"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a/m")))).contains("a");
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("x/n")))).contains("x");
    }

    @Test
    public void shouldUseGroupWithMaxPartitionCount() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b"]
                    },
                    "2": {
                      "0": ["A", "B"],
                      "1": ["x", "y"]
                    },
                    "3": {
                      "0": ["m", "n"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(
                Set.of(Path.of("a"), Path.of("x"), Path.of("m"))
        )).contains("m");
    }

    @Test
    public void shouldCacheFilesByCommit() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b", "c"]
                    }
                  }
                }
                """
        );
        fastCircuitTargetsAutoResolver.getFastTarget(Collections.emptySet());
        fastCircuitTargetsAutoResolver.getFastTarget(Collections.emptySet());

        verify(arcService, times(1)).getFileContent(anyString(), any());
    }

    @Test
    public void theFeatureCanBeTurnedOff() {
        given(db.currentOrReadOnly(any(Supplier.class))).willReturn(0);
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a", "b", "c"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a")))).isEmpty();
    }

    @Test
    public void shouldResolveModuleNamesContainingSlash() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a/b"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("a/b/File.java")))).contains("a/b");
    }

    @Test
    public void leadingSlashIsIgnoredInAffectedPath() {
        givenTheConfigIs("""
                {
                  "partitions": {
                    "1": {
                      "0": ["a/b"]
                    }
                  }
                }
                """
        );
        assertThat(fastCircuitTargetsAutoResolver.getFastTarget(Set.of(Path.of("/a/b/C.java")))).contains("a/b");
    }

    private void givenTheConfigIs(String content) {
        ArcRevision head = ArcRevision.of("head");
        given(arcService.getLastRevisionInBranch(ArcBranch.trunk())).willReturn(head);
        given(arcService.getFileContent(FastCircuitTargetsAutoResolver.CONFIG_PATH, head))
                .willReturn(Optional.of(content));
    }

    @Configuration
    @Import(ArcClientTestConfig.class)
    public static class Config {
        @Bean
        public FastCircuitTargetsAutoResolver getModuleResolverService(ArcService arcService, CiMainDb db) {
            return new FastCircuitTargetsAutoResolver(arcService, db, Duration.ofMinutes(3));
        }
    }
}
