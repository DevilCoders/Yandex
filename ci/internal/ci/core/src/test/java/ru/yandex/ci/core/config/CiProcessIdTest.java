package ru.yandex.ci.core.config;

import java.nio.file.Path;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

class CiProcessIdTest {

    @Test
    void ofFlow() {
        var flowFullId = new FlowFullId("my/path", "my-flow");
        var flowFullIdPath = FlowFullId.of(Path.of("my/path/a.yaml"), "my-flow");
        assertThat(flowFullId.getDir()).isEqualTo("my/path");
        assertThat(flowFullId.asString()).isEqualTo("my/path::my-flow");
        assertThat(flowFullId.toString()).isNotEqualTo(flowFullId.asString());

        assertThat(flowFullId)
                .isEqualTo(flowFullIdPath);

        var ciProcessId = CiProcessId.ofString("my/path/a.yaml:f:my-flow");
        assertThat(ciProcessId.getDir()).isEqualTo("my/path");
        assertThat(ciProcessId.getPath()).isEqualTo(Path.of("my/path/a.yaml"));
        assertThat(ciProcessId.getType()).isEqualTo(CiProcessId.Type.FLOW);
        assertThat(ciProcessId.getSubId()).isEqualTo("my-flow");
        assertThat(ciProcessId.asString()).isEqualTo("my/path/a.yaml:f:my-flow");
        assertThat(ciProcessId.toString()).isEqualTo(ciProcessId.asString());

        assertThat(ciProcessId)
                .isEqualTo(CiProcessId.ofFlow(flowFullId));

        assertThat(ciProcessId)
                .isEqualTo(CiProcessId.ofFlow(Path.of("my/path/a.yaml"), "my-flow"));
    }

    @Test
    void ofRelease() {
        var ciProcessId = CiProcessId.ofString("my/path/a.yaml:r:release");
        assertThat(ciProcessId.getDir()).isEqualTo("my/path");
        assertThat(ciProcessId.getPath()).isEqualTo(Path.of("my/path/a.yaml"));
        assertThat(ciProcessId.getType()).isEqualTo(CiProcessId.Type.RELEASE);
        assertThat(ciProcessId.getSubId()).isEqualTo("release");
        assertThat(ciProcessId.asString()).isEqualTo("my/path/a.yaml:r:release");
        assertThat(ciProcessId.toString()).isEqualTo(ciProcessId.asString());

        assertThat(ciProcessId)
                .isEqualTo(CiProcessId.ofRelease(Path.of("my/path/a.yaml"), "release"));
    }

    @Test
    void ofFlowRoot() {
        var flowFullId = new FlowFullId("", "my-flow");
        var flowFullIdPath = FlowFullId.of(Path.of("a.yaml"), "my-flow");
        assertThat(flowFullId.asString()).isEqualTo("my-flow");
        assertThat(flowFullId.toString()).isNotEqualTo(flowFullId.asString());

        assertThat(flowFullId)
                .isEqualTo(flowFullIdPath);

        var ciProcessId = CiProcessId.ofString("a.yaml:f:my-flow");
        assertThat(ciProcessId.getDir()).isEqualTo("");
        assertThat(ciProcessId.getPath()).isEqualTo(Path.of("a.yaml"));
        assertThat(ciProcessId.getType()).isEqualTo(CiProcessId.Type.FLOW);
        assertThat(ciProcessId.getSubId()).isEqualTo("my-flow");
        assertThat(ciProcessId.asString()).isEqualTo("a.yaml:f:my-flow");
        assertThat(ciProcessId.toString()).isEqualTo(ciProcessId.asString());
        assertThat(ciProcessId)
                .isEqualTo(CiProcessId.ofFlow(flowFullId));

        assertThat(ciProcessId)
                .isEqualTo(CiProcessId.ofFlow(Path.of("a.yaml"), "my-flow"));
    }

    @Test
    void ofReleaseRoot() {
        var ciProcessId = CiProcessId.ofString("a.yaml:r:release");
        assertThat(ciProcessId.getDir()).isEqualTo("");
        assertThat(ciProcessId.getPath()).isEqualTo(Path.of("a.yaml"));
        assertThat(ciProcessId.getType()).isEqualTo(CiProcessId.Type.RELEASE);
        assertThat(ciProcessId.getSubId()).isEqualTo("release");
        assertThat(ciProcessId.asString()).isEqualTo("a.yaml:r:release");
        assertThat(ciProcessId.toString()).isEqualTo(ciProcessId.asString());

        Assertions.assertEquals(
                ciProcessId,
                CiProcessId.ofRelease(Path.of("a.yaml"), "release")
        );
    }
}
