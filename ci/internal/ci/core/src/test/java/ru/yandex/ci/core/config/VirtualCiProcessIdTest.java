package ru.yandex.ci.core.config;

import java.nio.file.Path;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;

import static org.assertj.core.api.Assertions.assertThat;


class VirtualCiProcessIdTest {

    @Test
    void testVirtualTypeOfDefault() {
        var id = CiProcessId.ofFlow(Path.of("autocheck/a.yaml"), "test-flow");
        var virtualId = VirtualCiProcessId.of(id);

        assertThat(virtualId.getCiProcessId())
                .isEqualTo(id)
                .isSameAs(id);
        assertThat(virtualId.getResolvedPath())
                .isEqualTo(id.getPath());
        assertThat(virtualId.getVirtualType())
                .isNull();
    }

    @Test
    void testVirtualTypeOfLargeTest() {
        var id = CiProcessId.ofFlow(Path.of("large-test@autocheck/a.yaml"), "test-flow");
        var virtualId = VirtualCiProcessId.of(id);

        assertThat(virtualId.getCiProcessId())
                .isSameAs(id);
        assertThat(virtualId.getResolvedPath())
                .isEqualTo(VirtualCiProcessId.LARGE_FLOW.getPath());
        assertThat(virtualId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_LARGE_TEST);
        assertThat(VirtualType.VIRTUAL_LARGE_TEST.getService())
                .isEqualTo("autocheck");
    }

    @Test
    void testVirtualToVirtualLargeTest() {
        var id = CiProcessId.ofFlow(Path.of("autocheck/a.yaml"), "test-flow");
        var mappedId = VirtualCiProcessId.toVirtual(id, VirtualType.VIRTUAL_LARGE_TEST);

        assertThat(mappedId).isNotEqualTo(id);
        assertThat(mappedId).isEqualTo(CiProcessId.ofFlow(Path.of("large-test@autocheck/a.yaml"), "test-flow"));

        var virtualId = VirtualCiProcessId.of(mappedId);
        assertThat(virtualId.getCiProcessId())
                .isNotEqualTo(id)
                .isSameAs(mappedId);
        assertThat(virtualId.getResolvedPath())
                .isEqualTo(VirtualCiProcessId.LARGE_FLOW.getPath());
        assertThat(virtualId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_LARGE_TEST);
    }

    @Test
    void testVirtualTypeOfNativeBuild() {
        var id = CiProcessId.ofFlow(Path.of("native-build@autocheck/a.yaml"), "test-flow");
        var virtualId = VirtualCiProcessId.of(id);

        assertThat(virtualId.getCiProcessId())
                .isSameAs(id);
        assertThat(virtualId.getResolvedPath())
                .isEqualTo(VirtualCiProcessId.LARGE_FLOW.getPath());
        assertThat(virtualId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_NATIVE_BUILD);
        assertThat(VirtualType.VIRTUAL_NATIVE_BUILD.getService())
                .isEqualTo("autocheck");
    }

    @Test
    void testVirtualToVirtualNativeBuild() {
        var id = CiProcessId.ofFlow(Path.of("autocheck/a.yaml"), "test-flow");
        var mappedId = VirtualCiProcessId.toVirtual(id, VirtualType.VIRTUAL_NATIVE_BUILD);

        assertThat(mappedId).isNotEqualTo(id);
        assertThat(mappedId).isEqualTo(CiProcessId.ofFlow(Path.of("native-build@autocheck/a.yaml"), "test-flow"));

        var virtualId = VirtualCiProcessId.of(mappedId);
        assertThat(virtualId.getCiProcessId())
                .isNotEqualTo(id)
                .isSameAs(mappedId);
        assertThat(virtualId.getResolvedPath())
                .isEqualTo(VirtualCiProcessId.LARGE_FLOW.getPath());
        assertThat(virtualId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_NATIVE_BUILD);
    }
}
