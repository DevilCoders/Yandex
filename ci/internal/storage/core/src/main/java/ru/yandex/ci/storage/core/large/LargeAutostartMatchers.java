package ru.yandex.ci.storage.core.large;

import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.Getter;
import lombok.ToString;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check.LargeAutostart;
import ru.yandex.ci.storage.core.db.model.check.NativeBuild;
import ru.yandex.ci.storage.core.db.model.test_diff.TestDiffEntity;
import ru.yandex.ci.util.GlobMatchers;

@Slf4j
public class LargeAutostartMatchers {
    public List<? extends LargeTestMatcher> collectLargeTestsMatchers(CheckEntity check) {
        boolean matchAllTests = check.getRunLargeTestsAfterDiscovery();
        if (matchAllTests) {
            return getLargeTestsMatchAll();
        } else {
            return toLargeTestsMatchers(check.getAutostartLargeTests());
        }
    }

    public List<NativeBuildAutostartMatcher> collectNativeBuildsMatchers(CheckEntity check) {
        return check.getNativeBuilds().stream()
                .map(NativeBuildAutostartMatcher::new)
                .toList();
    }
        //

    private static List<? extends LargeTestMatcher> getLargeTestsMatchAll() {
        return List.of(new LargeTestMatcher() {

            @Nullable
            @Override
            public String getConfigPath() {
                return null;
            }

            @Override
            public boolean isAccepted(TestDiffEntity.Id id) {
                return true;
            }
        });
    }

    private static List<? extends LargeTestMatcher> toLargeTestsMatchers(List<LargeAutostart> autoStarts) {
        var autostartMap = new LinkedHashMap<String, Set<String>>(autoStarts.size());
        var configPathMap = new HashMap<String, String>(autoStarts.size());
        for (var autostart : autoStarts) {
            var toolchains = autostart.getToolchains();
            var target = testenvCompatible(autostart.getTarget());
            configPathMap.putIfAbsent(target, autostart.getPath());

            var prev = autostartMap.get(target);
            if (prev != null) {
                if (!prev.isEmpty()) {
                    if (toolchains.isEmpty()) {
                        prev.clear(); // all toolchains
                    } else {
                        prev.addAll(toolchains);
                    }
                }
            } else {
                autostartMap.put(target, new HashSet<>(toolchains));
            }
        }

        log.info("Total {} Large tests matched", autostartMap.size());
        return autostartMap.entrySet().stream()
                .map(e -> new LargeTestMatcherImpl(e.getKey(), e.getValue(), configPathMap.get(e.getKey())))
                .toList();
    }

    private static String testenvCompatible(String path) {
        if (path.contains("**")) {
            path = path.replace("**", "*");
        }
        return path.replace("*", "**");
    }

    interface LargeTestMatcher {
        @Nullable
        String getConfigPath();

        boolean isAccepted(TestDiffEntity.Id id);
    }

    @Getter
    @ToString
    static class LargeTestMatcherImpl implements LargeTestMatcher {
        @ToString.Exclude
        PathMatcher pathMatcher;
        String path;
        Set<String> toolchains;
        String configPath;

        LargeTestMatcherImpl(@Nonnull String path, @Nonnull Set<String> toolchains, @Nullable String configPath) {
            this.pathMatcher = GlobMatchers.getGlobMatcher(path);
            this.path = path;
            this.toolchains = toolchains;
            this.configPath = configPath;

            log.info("Registered Large tests matcher, path: \"{}\", toolchains: {}, config: \"{}\"",
                    path, toolchains, configPath);
        }

        @Override
        public String getConfigPath() {
            return configPath;
        }

        @Override
        public boolean isAccepted(TestDiffEntity.Id id) {
            if (toolchains.isEmpty() || toolchains.contains(id.getToolchain())) {
                return pathMatcher.matches(Path.of(id.getPath()));
            }
            return false;
        }
    }

    @Getter
    @ToString
    static class NativeBuildAutostartMatcher {
        String path;
        @ToString.Exclude
        String dir;
        String toolchain;
        Set<String> matchTargets;

        Set<String> selectedTargets;
        List<TestDiffEntity> diffs;

        NativeBuildAutostartMatcher(@Nonnull NativeBuild build) {
            this.path = build.getPath();
            this.dir = dirFromPath(path);
            this.toolchain = build.getToolchain();

            this.matchTargets = new LinkedHashSet<>(build.getTargets());
            this.selectedTargets = new LinkedHashSet<>(matchTargets.size());
            this.diffs = new ArrayList<>(matchTargets.size());

            log.info("Registered Native build matcher, path: \"{}\", toolchain: \"{}\", targets: {}",
                    dir, toolchain, matchTargets);
        }

        void checkAccepted(TestDiffEntity diff) {
            var idPath = diff.getId().getPath();
            if (matchTargets.contains(idPath)) {
                if (selectedTargets.add(idPath)) {
                    diffs.add(diff);
                }
            }
        }

        private static String dirFromPath(String pathOrDir) {
            return Path.of(pathOrDir).getParent().toString();
        }

    }

}
