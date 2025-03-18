package ru.yandex.ci.core.abc;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.Set;

import com.google.common.collect.MultimapBuilder;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.abc.AbcTableClient;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.FavoriteProject;
import ru.yandex.ci.core.db.model.FavoriteProject.Mode;

@Slf4j
@RequiredArgsConstructor
public class AbcFavoriteProjectsService {

    private final AbcTableClient client;
    private final CiMainDb db;

    public void syncFavoriteProjects() {
        var newMapping = client.getUserToServicesMapping();
        log.info("New mapping before filter: {} ({} logins)",
                newMapping.size(), newMapping.keySet().size());

        var knownProjects = Set.copyOf(db.currentOrReadOnly(() ->
                db.configStates().listProjects(null, null, true, 0)));

        // Filter mapping - remove all services we don't know
        newMapping.values().removeIf(slug -> !knownProjects.contains(slug));
        log.info("New mapping with known services: {} ({} logins)",
                newMapping.size(), newMapping.keySet().size());


        var existingMapping = MultimapBuilder.hashKeys().hashSetValues()
                .<String, FavoriteProject>build();
        db.currentOrReadOnly(() -> db.favoriteProjects().readTable()
                .forEach(project -> existingMapping.put(project.getUser(), project)));

        log.info("Current mapping: {} ({} logins)",
                existingMapping.size(), existingMapping.keySet().size());

        var toRemove = new HashSet<FavoriteProject.Id>();
        existingMapping.values().forEach(project -> {
            if (!newMapping.remove(project.getUser(), project.getProject())) {
                if (project.getMode() == Mode.SET_AUTO) {
                    toRemove.add(project.getId());
                }
            }
        });

        var toSet = new ArrayList<FavoriteProject>(newMapping.size());
        newMapping.forEach((user, project) ->
                toSet.add(FavoriteProject.of(user, project, Mode.SET_AUTO)));

        log.info("Total projects to remove: {}", toRemove.size());
        log.info("Total projects to set: {}", toSet.size());

        db.currentOrTx(() -> {
            db.favoriteProjects().delete(toRemove);
            db.favoriteProjects().save(toSet);
        });

    }
}
