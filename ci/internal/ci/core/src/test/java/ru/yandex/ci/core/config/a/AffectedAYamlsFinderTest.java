package ru.yandex.ci.core.config.a;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.stream.Collectors;

import org.junit.jupiter.api.Test;

import ru.yandex.arc.api.Repo;

import static org.assertj.core.api.Assertions.assertThat;

public class AffectedAYamlsFinderTest {

    @Test
    void addPotentialConfigsRecursively() {
        List<Path> result = new ArrayList<>();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of("a/b/c/abx.txt"), false, result);
        assertThat(result).isEqualTo(paths("a/b/c/a.yaml", "a/b/a.yaml", "a/a.yaml", "a.yaml"));

        result = new ArrayList<>();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of("a/b/c/a.yaml"), false, result);
        assertThat(result).isEqualTo(paths("a/b/c/a.yaml", "a/b/a.yaml", "a/a.yaml", "a.yaml"));
    }

    @Test
    void addPotentialConfigsRecursivelyDir() {
        List<Path> result = new ArrayList<>();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of("c/d/f/"), true, result);
        assertThat(result).isEqualTo(paths("c/d/f//a.yaml", "c/d/a.yaml", "c/a.yaml", "a.yaml"));

        result.clear();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of("ci"), true, result);
        assertThat(result).isEqualTo(paths("ci/a.yaml", "a.yaml"));

        result.clear();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of(""), true, result);
        assertThat(result).isEqualTo(paths("a.yaml"));

        result.clear();
        AffectedAYamlsFinder.addPotentialConfigsRecursively(Path.of("/"), true, result);
        assertThat(result).isEqualTo(paths("/a.yaml"));
    }

    @Test
    void isInExcludedDirs() {
        assertThat(AffectedAYamlsFinder.isInExcludedDirs(
                Path.of("ci/core/src/test/resources/test-repo/ds1/a/b/c/a.yaml")
        )).isTrue();
        assertThat(AffectedAYamlsFinder.isInExcludedDirs(
                Path.of("arcanum/core/src/test/resources/test-repo/ds1/a/b/c/a.yaml")
        )).isFalse();

        assertThat(AffectedAYamlsFinder.isInExcludedDirs(
                Path.of("ci/a.yaml")
        )).isFalse();
    }

    @Test
    void changeConsumer_addPotentialAffectedConfigs_whenOnlyConfigChanged() {
        var consumer = new AffectedAYamlsFinder.ChangeConsumer();

        consumer.accept(
                Repo.ChangelistResponse.Change.newBuilder()
                        .setPath("a/b/a.yaml")
                        .setChange(Repo.ChangelistResponse.ChangeType.Modify)
                        .build()
        );

        assertThat(consumer.getModifiedConfigs()).containsOnly(Path.of("a/b/a.yaml"));
        assertThat(consumer.getPotentialAffectedConfigs()).containsOnly(
                Path.of("a.yaml"), Path.of("a/a.yaml"), Path.of("a/b/a.yaml")
        );
    }

    private static List<Path> paths(String... paths) {
        return Arrays.stream(paths).map(Path::of).collect(Collectors.toList());
    }

}
