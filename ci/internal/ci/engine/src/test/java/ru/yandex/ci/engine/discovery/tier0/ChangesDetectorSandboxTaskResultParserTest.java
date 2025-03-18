package ru.yandex.ci.engine.discovery.tier0;

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.fasterxml.jackson.core.JsonParseException;
import com.fasterxml.jackson.core.io.JsonEOFException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.discovery.GraphDiscoveryTask;
import ru.yandex.ci.engine.discovery.TriggeredProcesses;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

class ChangesDetectorSandboxTaskResultParserTest {

    @Test
    void parseValidJson() {
        var affectedDirs = new HashMap<String, List<String>>();

        ChangesDetectorSandboxTaskResultParser.parseJson(
                new ByteArrayInputStream(
                        "{\"linux\": [\"ci/tms\", \"ci/storage\"]}".getBytes(StandardCharsets.UTF_8)
                ),
                affectedPathMatcher(affectedDirs)
        );
        assertThat(affectedDirs).containsAllEntriesOf(Map.of(
                "linux", List.of("ci/tms", "ci/storage")
        ));
    }

    @Test
    void parseEmptyJson() {
        var affectedDirs = new HashMap<String, List<String>>();

        ChangesDetectorSandboxTaskResultParser.parseJson(
                new ByteArrayInputStream(
                        "{}".getBytes(StandardCharsets.UTF_8)
                ),
                affectedPathMatcher(affectedDirs)
        );
        assertThat(affectedDirs).containsAllEntriesOf(Map.of());
    }

    @Test
    void parseJsonWithEmptyList() {
        var affectedDirs = new HashMap<String, List<String>>();

        ChangesDetectorSandboxTaskResultParser.parseJson(
                new ByteArrayInputStream(
                        "{\"linux\": []}".getBytes(StandardCharsets.UTF_8)
                ),
                affectedPathMatcher(affectedDirs)
        );
        assertThat(affectedDirs).containsAllEntriesOf(Map.of());
    }

    @Test
    void parseInvalidTruncatedJson() {
        assertThatThrownBy(() -> {
            var affectedDirs = new HashMap<String, List<String>>();

            ChangesDetectorSandboxTaskResultParser.parseJson(
                    new ByteArrayInputStream(
                            "{\"linux\": [\"ci/tms\"".getBytes(StandardCharsets.UTF_8)
                    ),
                    affectedPathMatcher(affectedDirs)
            );
        }).isInstanceOf(GraphDiscoveryException.class)
                .hasCauseInstanceOf(JsonEOFException.class);
    }

    @Test
    void parseInvalidJson() {
        assertThatThrownBy(() -> {
            var affectedDirs = new HashMap<String, List<String>>();

            ChangesDetectorSandboxTaskResultParser.parseJson(
                    new ByteArrayInputStream(
                            "{\"linux\": \"string\"}".getBytes(StandardCharsets.UTF_8)
                    ),
                    affectedPathMatcher(affectedDirs)
            );
        }).isInstanceOf(GraphDiscoveryException.class)
                .hasCauseInstanceOf(JsonParseException.class);
    }

    @Test
    void decompressAndUnTar() throws IOException {
        InputStream in = ChangesDetectorSandboxTaskResultParser.decompressAndUnTar(
                "out.json.tar.gz",
                new ByteArrayInputStream(
                        TestUtils.binaryResource("graph-discovery/out.json.tar.gz")
                )
        );
        String result = new String(in.readAllBytes(), StandardCharsets.UTF_8);
        assertThat(result).isEqualTo("{\"linux\": [\"ci/tms\", \"ci/storage\"]}\n");
    }

    @Test
    void parseTarGz() {
        var affectedDirs = new HashMap<String, List<String>>();
        var parser = new ChangesDetectorSandboxTaskResultParser(
                affectedPathMatcher(affectedDirs)
        );
        parser.parse(
                "out.json.tar.gz",
                TestUtils.binaryResource("graph-discovery/out.json.tar.gz")
        );
        assertThat(affectedDirs).containsAllEntriesOf(Map.of(
                "linux", List.of("ci/tms", "ci/storage")
        ));
    }

    private static AffectedPathMatcher affectedPathMatcher(Map<String, List<String>> affectedDirs) {
        return new AffectedPathMatcher() {

            @Override
            public void accept(GraphDiscoveryTask.Platform platform, Path path) throws GraphDiscoveryException {
                affectedDirs.computeIfAbsent(platform.name().toLowerCase(), key -> new ArrayList<>())
                        .add(path.toString());
            }

            @Override
            public void flush() {
                // noop
            }

            @Override
            public Set<TriggeredProcesses.Triggered> getTriggered() {
                return Set.of();
            }
        };
    }

}
