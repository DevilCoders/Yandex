package ru.yandex.ci.engine.autocheck;

import java.nio.file.Path;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

import javax.annotation.Nullable;

import org.apache.commons.lang3.StringUtils;

import ru.yandex.arc.api.Repo;
import ru.yandex.arc.api.Shared;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;

public final class AffectedDirectoriesHelper {

    private AffectedDirectoriesHelper() {
    }

    // Inspired by https://nda.ya.ru/t/O3qcx8sw4GdZ9L
    public static List<String> collectAndSort(ArcRevision arcRevision, ArcService arcService) {
        var affectedDirectories = new LinkedHashSet<String>();
        arcService.processChanges(arcRevision, null, change -> {
            addPathToAffectedDirectories(Path.of(change.getPath()), change, affectedDirectories);
            if (change.hasSource() && change.getChange() != Repo.ChangelistResponse.ChangeType.Copy) {
                addPathToAffectedDirectories(
                        Path.of(change.getSource().getPath()),
                        change, affectedDirectories
                );
            }
        });
        return affectedDirectories.stream().sorted().toList();
    }

    private static void addPathToAffectedDirectories(
            Path path, Repo.ChangelistResponse.Change change, Set<String> affectedDirectories
    ) {
        boolean isFile = change.getType() == Shared.TreeEntryType.TreeEntryFile;
        if (isFile) {
            path = path.getParent();
        }
        affectedDirectories.add(normalizePath(path));
    }

    private static String normalizePath(@Nullable Path path) {
        return normalizePath(path != null ? path.toString() : "");
    }

    static String normalizePath(String result) {
        return StringUtils.strip(result, "/");
    }

}
