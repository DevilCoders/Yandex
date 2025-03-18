package ru.yandex.ci.core.abc;

import java.util.List;
import java.util.Set;
import java.util.stream.Stream;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.client.abc.AbcTableTestServer;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.FavoriteProject;
import ru.yandex.ci.core.spring.AbcConfig;
import ru.yandex.ci.core.spring.clients.AbcClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;

@ContextConfiguration(classes = {
        AbcConfig.class,
        AbcClientTestConfig.class
})
class AbcFavoriteProjectsServiceTest extends CommonYdbTestBase {

    @Autowired
    private AbcFavoriteProjectsService abcFavoriteProjectsService;

    @Autowired
    private AbcTableTestServer abcTableTestServer;

    @BeforeEach
    void init() {
        abcTableTestServer.reset();
    }

    @Test
    void testEmpty() {
        abcFavoriteProjectsService.syncFavoriteProjects();
        checkFavorites();
    }

    @Test
    void testEmptyWithFavorites() {
        setFavorites(
                FavoriteProject.of("user1", "project1", FavoriteProject.Mode.NONE),
                FavoriteProject.of("user1", "project2", FavoriteProject.Mode.SET),
                FavoriteProject.of("user1", "project3", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user1", "project4", FavoriteProject.Mode.UNSET)
        );

        // unset with `set_auto` only
        abcFavoriteProjectsService.syncFavoriteProjects();
        checkFavorites(
                FavoriteProject.of("user1", "project1", FavoriteProject.Mode.NONE),
                FavoriteProject.of("user1", "project2", FavoriteProject.Mode.SET),
                FavoriteProject.of("user1", "project4", FavoriteProject.Mode.UNSET)
        );
    }

    @Test
    void testDefaultMappedWithoutKnownProjects() {
        abcTableTestServer.addMap("user1", "project1-1");
        abcTableTestServer.addMap("user1", "project1-2");
        abcTableTestServer.addMap("user2", "project2-1");
        abcTableTestServer.addMap("user2", "project2-2");

        abcFavoriteProjectsService.syncFavoriteProjects();
        checkFavorites();
    }

    @Test
    void testDefaultMapped() {
        abcTableTestServer.addMap("user1", "project1-1");
        abcTableTestServer.addMap("user1", "project1-2");
        abcTableTestServer.addMap("user2", "project2-1");
        abcTableTestServer.addMap("user2", "project2-2");

        setConfigStates("project1-1", "project1-2", "project2-1", "project2-2");

        abcFavoriteProjectsService.syncFavoriteProjects();
        checkFavorites(
                FavoriteProject.of("user1", "project1-1", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user1", "project1-2", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user2", "project2-1", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user2", "project2-2", FavoriteProject.Mode.SET_AUTO)
        );
    }

    @Test
    void testOverrideMapping() {

        setFavorites(
                FavoriteProject.of("user1", "project1", FavoriteProject.Mode.NONE),
                FavoriteProject.of("user1", "project2", FavoriteProject.Mode.SET),
                FavoriteProject.of("user1", "project3", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user1", "project4", FavoriteProject.Mode.UNSET)
        );

        abcTableTestServer.addMap("user1", "project1");
        abcTableTestServer.addMap("user1", "project2");
        abcTableTestServer.addMap("user1", "project5");

        abcTableTestServer.addMap("user2", "project6");
        abcTableTestServer.addMap("user2", "project7");

        setConfigStates("project1", "project2", "project3", "project4", "project5", "project6");
        abcFavoriteProjectsService.syncFavoriteProjects();
        checkFavorites(
                FavoriteProject.of("user1", "project1", FavoriteProject.Mode.NONE),
                FavoriteProject.of("user1", "project2", FavoriteProject.Mode.SET),
                FavoriteProject.of("user1", "project4", FavoriteProject.Mode.UNSET),
                FavoriteProject.of("user1", "project5", FavoriteProject.Mode.SET_AUTO),
                FavoriteProject.of("user2", "project6", FavoriteProject.Mode.SET_AUTO)
        );
    }

    private void setFavorites(FavoriteProject... projects) {
        db.currentOrTx(() -> db.favoriteProjects().save(List.of(projects)));
    }

    private void setConfigStates(String... projects) {
        var configStates = Stream.of(projects)
                .map(project -> ConfigState.builder()
                        .id(ConfigState.Id.of("/" + project))
                        .project(project)
                        .status(ConfigState.Status.OK)
                        .build())
                .toList();
        db.currentOrTx(() -> db.configStates().save(configStates));
    }

    private void checkFavorites(FavoriteProject... expect) {
        var actual = db.currentOrReadOnly(() -> db.favoriteProjects().findAll());
        assertThat(Set.copyOf(actual))
                .isEqualTo(Set.of(expect));
    }
}
