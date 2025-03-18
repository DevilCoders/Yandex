package ru.yandex.ci.util;

import java.nio.file.FileSystems;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.util.List;
import java.util.function.Function;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

public class GlobMatchers {

    // See class Globs
    private static final Pattern GLOB_META_CHARS_PATTERN = Pattern.compile("([*?\\[{\\\\])");

    private GlobMatchers() {
        //
    }

    public static PathMatcher getGlobMatcher(String absPath) {
        return FileSystems.getDefault().getPathMatcher("glob:" + absPath);
    }

    public static List<PathMatcher> getGlobMatchersFor(List<String> absPaths) {
        return getGlobMatchersFor(absPaths, Function.identity());
    }

    public static List<PathMatcher> getGlobMatchersFor(List<String> absPaths, Function<String, String> pathConverter) {
        return absPaths.stream()
                .map(pathConverter)
                .map(GlobMatchers::getGlobMatcher)
                .collect(Collectors.toList());
    }

    public static boolean pathsMatchGlobs(
            List<String> paths,
            List<String> includePathGlobs,
            List<String> excludePathGlobs
    ) {
        var acceptedPaths = paths.stream().map(Path::of).collect(Collectors.toList());
        var includePatterns = includePathGlobs.stream().map(GlobMatchers::getGlobMatcher).toList();

        if (!includePatterns.isEmpty()) {
            acceptedPaths = acceptedPaths.stream()
                    .filter(p -> includePatterns.stream().anyMatch(pattern -> pattern.matches(p)))
                    .collect(Collectors.toList());
            if (acceptedPaths.isEmpty()) {
                return false;
            }
        }

        var excludePatterns = excludePathGlobs.stream().map(GlobMatchers::getGlobMatcher).toList();

        if (!excludePatterns.isEmpty()) {
            acceptedPaths = acceptedPaths.stream()
                    .filter(p -> excludePatterns.stream().noneMatch(pattern -> pattern.matches(p)))
                    .collect(Collectors.toList());

            if (acceptedPaths.isEmpty()) {
                return false;
            }
        }

        return true;
    }

    public static String escapeMetaChars(String path) {
        return GLOB_META_CHARS_PATTERN.matcher(path).replaceAll("\\\\$1");
    }
}
