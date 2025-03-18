package ru.yandex.ci.core.launch.versioning;

import java.util.List;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;

public class VersionTest {
    @Test
    void order() {
        var versions = List.of(Version.major("8"),
                Version.majorMinor("3", "1"),
                Version.majorMinor("5", "2"),
                Version.major("5"),
                Version.majorMinor("5", "3"),
                Version.major("3")
        );

        List<Version> sorted = versions.stream().sorted().collect(Collectors.toList());
        assertThat(sorted)
                .extracting(Version::asString)
                .containsExactly(
                        "3",
                        "3.1",
                        "5",
                        "5.2",
                        "5.3",
                        "8"
                );
    }
}
