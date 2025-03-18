package ru.yandex.ci.ayamler;

import java.nio.file.Path;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;
import java.util.concurrent.CompletableFuture;

import javax.annotation.Nullable;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.ayamler.api.spring.AYamlerServiceConfig;
import ru.yandex.ci.ayamler.api.spring.AbcClientTestConfig;
import ru.yandex.ci.ayamler.fetcher.OidCache;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.arc.RepoStat;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.AutocheckConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.StrongModeConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.spring.clients.ArcClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.doThrow;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static ru.yandex.ci.util.CollectionUtils.linkedSet;

@ContextConfiguration(classes = {
        AYamlerServiceConfig.class,

        AbcClientTestConfig.class,
        ArcClientTestConfig.class
})
public class AYamlerServiceTest extends AYamlerTestBase {

    private static final ArcRevision REVISION = ArcRevision.of("a".repeat(40));
    private static final ArcRevision LAST_CHANGED_REVISION = ArcRevision.of("b".repeat(40));

    @Autowired
    AYamlerService aYamlerService;

    @SpyBean
    ArcService arcService;
    @SpyBean
    AYamlService aYamlService;
    @SpyBean
    AbcService abcService;

    @BeforeEach
    void setUp() {
        aYamlerService.resetCaches();
    }

    @Test
    void getStrongMode_shouldReturnFirstAYamlWithDefinedStrongMode() throws Exception {
        var path = Path.of("ci/tms/src");
        var scopeSlugs = Set.of("devops_test_scope", "developer_test_scope");

        mockFileNotExists("ci/tms/src/a.yaml");
        mockFileNotExists("ci/tms/a.yaml");
        mockFileExists("ci/a.yaml");
        mockFileExists("a.yaml");

        mockFileContent(
                "ci/a.yaml",
                StrongModeConfig.builder()
                        .enabled(true)
                        .abcScopes(scopeSlugs).build()
        );

        doReturn(true).when(abcService).isUserBelongsToService(eq("check-author"), eq("ci"), eq(scopeSlugs));

        Optional<AYaml> result = aYamlerService.getStrongMode(path, REVISION, "check-author");

        verifyFileExistenceChecked("ci/tms/src/a.yaml");
        verifyFileExistenceChecked("ci/tms/a.yaml");
        verifyFileExistenceChecked("ci/a.yaml");
        verifyContentFetched("ci/a.yaml");

        verifyFileExistenceChecked("a.yaml");
        verifyContentNotFetched("a.yaml");

        assertThat(result).isEqualTo(Optional.of(
                AYaml.valid(Path.of("ci/a.yaml"), "ci", new StrongMode(true, scopeSlugs), true)
        ));
    }

    static List<Arguments> getStrongMode_whenNestedDirectoryOverridesStrongMode() {
        var scopeSlugs = linkedSet("devops_test_scope", "developer_test_scope");

        return List.of(
                Arguments.of(
                        StrongModeConfig.of(false),
                        StrongModeConfig.of(true),
                        new StrongMode(false, StrongModeConfig.DEFAULT_ABC_SCOPES)
                ),
                Arguments.of(
                        StrongModeConfig.builder()
                                .enabled(true)
                                .abcScopes(scopeSlugs).build(),
                        StrongModeConfig.of(true),
                        new StrongMode(true, scopeSlugs)
                )
        );
    }

    @ParameterizedTest
    @MethodSource
    void getStrongMode_whenNestedDirectoryOverridesStrongMode(
            StrongModeConfig nestedStrongModeConfig,
            StrongModeConfig parentStrongModeConfig,
            StrongMode expectedStrongMode
    ) throws Exception {
        var path = Path.of("ci");

        mockFileExists("ci/a.yaml");
        mockFileContent("ci/a.yaml", nestedStrongModeConfig);

        mockFileExists("a.yaml");
        mockFileContent("a.yaml", parentStrongModeConfig);

        doReturn(true).when(abcService).isUserBelongsToService(
                eq("check-author"), eq("ci"),
                eq(expectedStrongMode.getAbcScopes())
        );

        Optional<AYaml> result = aYamlerService.getStrongMode(path, REVISION, "check-author");

        verifyFileExistenceChecked("ci/a.yaml");
        verifyContentFetched("ci/a.yaml");

        verifyFileExistenceChecked("a.yaml");
        verifyContentNotFetched("a.yaml");

        assertThat(result)
                .isNotEmpty()
                .isEqualTo(Optional.of(
                        AYaml.valid(Path.of("ci/a.yaml"), "ci", expectedStrongMode, result.get().isOwner())
                ));
    }

    @Test
    void getStrongMode_shouldReturnFalseWhenAYamlIsNotValid() throws Exception {
        var path = Path.of("ci");

        mockFileExists("ci/a.yaml");
        mockFileExists("a.yaml");
        mockInvalidAYamlConfig("ci/a.yaml", new RuntimeException("some exception"));

        Optional<AYaml> result = aYamlerService.getStrongMode(path, REVISION, "check-author");

        verifyContentFetched("ci/a.yaml");
        verifyContentNotFetched("a.yaml");

        assertThat(result).isEqualTo(Optional.of(
                AYaml.invalid(Path.of("ci/a.yaml"), "some exception", false)
                        .withStrongMode(new StrongMode(false, Set.of()))
        ));
    }

    @Test
    void aYamlOidCacheKey_equalsAndHashCode() {
        var key1 = OidCache.Key.of(
                Path.of("a"), ArcRevision.of("rev")
        );
        var key2 = OidCache.Key.of(
                Path.of("a"), ArcRevision.of("rev")
        );
        assertThat(key1).isEqualTo(key2);
        assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    }

    @Test
    void getStrongModeCacheKey_equalsAndHashCode() {
        var key1 = StrongModeCache.Key.of(
                Path.of("a"), ArcRevision.of("rev"), "check-author"
        );
        var key2 = StrongModeCache.Key.of(
                Path.of("a"), ArcRevision.of("rev"), "check-author"
        );
        assertThat(key1).isEqualTo(key2);
        assertThat(key1.hashCode()).isEqualTo(key2.hashCode());
    }

    @Test
    void getAbcServiceSlugBatch_whenNoAYamlFound() throws Exception {
        var path = Path.of("path/not/found");

        for (var aYaml : List.of(
                "path/not/found/a.yaml",
                "path/not/a.yaml",
                "path/a.yaml",
                "a.yaml"
        )) {
            mockFileNotExists(aYaml);
        }

        Map<Path, Optional<AYaml>> result = aYamlerService.getAbcServiceSlugBatch(REVISION, Set.of(path));

        verifyFileExistenceChecked("path/not/found/a.yaml");
        verifyFileExistenceChecked("path/not/a.yaml");
        verifyFileExistenceChecked("path/a.yaml");
        verifyFileExistenceChecked("a.yaml");

        verify(aYamlService, times(0)).getConfig(any(), any());

        assertThat(result).isEqualTo(Map.of(
                Path.of("path/not/found"),
                Optional.empty()
        ));
    }

    @Test
    void getAbcServiceSlugBatch_shouldReturnSlugFromFirstValidAYaml() throws Exception {
        var path = Path.of("ci/tms");

        mockFileExists("ci/tms/a.yaml");
        mockFileExists("ci/a.yaml");
        mockFileExists("a.yaml");

        // ci/tms/a.yaml is invalid
        mockInvalidAYamlConfig("ci/tms/a.yaml", new AYamlValidationException("job1", "error"));
        mockFileContent("ci/a.yaml", StrongModeConfig.of(false));

        Map<Path, Optional<AYaml>> result = aYamlerService.getAbcServiceSlugBatch(REVISION, Set.of(path));

        verifyFileExistenceChecked("ci/tms/a.yaml");
        verifyContentFetched("ci/tms/a.yaml");

        verifyFileExistenceChecked("ci/a.yaml");
        verifyContentFetched("ci/a.yaml");

        verifyFileExistenceChecked("a.yaml");
        verifyContentNotFetched("a.yaml");

        assertThat(result).isEqualTo(Map.of(
                Path.of("ci/tms"),
                Optional.of(
                        AYaml.valid(Path.of("ci/a.yaml"), "ci",
                                new StrongMode(false, StrongModeConfig.DEFAULT_ABC_SCOPES), false)
                ))
        );
    }

    private void mockFileExists(String path) {
        var stat = fileRepoStat(path, oidFromPath(path));

        doReturn(CompletableFuture.completedFuture(Optional.of(stat)))
                .when(arcService).getStatAsync(eq(path), eq(REVISION), eq(false));
    }

    private void mockFileNotExists(String aYaml) {
        doReturn(CompletableFuture.completedFuture(
                Optional.empty()
        )).when(arcService).getStatAsync(eq(aYaml), eq(REVISION), eq(false));
    }

    private void mockFileContent(String path, StrongModeConfig strongModeConfig) throws Exception {
        var stat = fileRepoStat(path, oidFromPath(path));
        doReturn(Optional.of(stat)).when(arcService).getStat(eq(path), eq(REVISION), eq(true));

        doReturn(createAYamlConfig(strongModeConfig)).when(aYamlService).getConfig(
                eq(Path.of(path)), eq(LAST_CHANGED_REVISION)
        );
    }

    private void mockInvalidAYamlConfig(String path, Exception exception) throws Exception {
        var stat = fileRepoStat(path, oidFromPath(path));
        doReturn(Optional.of(stat)).when(arcService).getStat(eq(path), eq(REVISION), eq(true));

        doThrow(exception).when(aYamlService).getConfig(
                eq(Path.of(path)), eq(LAST_CHANGED_REVISION)
        );
    }

    private static String oidFromPath(String path) {
        return path + ":oid";
    }

    private static AYamlConfig createAYamlConfig(@Nullable StrongModeConfig strongMode) {
        var autocheck = AutocheckConfig.builder()
                .strongMode(strongMode)
                .build();
        var ciConfig = CiConfig.builder()
                .autocheck(autocheck)
                .build();

        return new AYamlConfig("ci", "title", ciConfig, null);
    }

    private static RepoStat fileRepoStat(String name, String oid) {
        return new RepoStat(
                name, RepoStat.EntryType.FILE, 0, false, false,
                oid,
                ArcCommit.builder()
                        .id(ArcCommit.Id.of(LAST_CHANGED_REVISION.getCommitId()))
                        .build(),
                false
        );
    }

    private void verifyFileExistenceChecked(String path) {
        verify(arcService, times(1)).getStatAsync(eq(path), eq(REVISION), eq(false));
    }

    private void verifyContentFetched(String path) throws Exception {
        verify(aYamlService, times(1)).getConfig(eq(Path.of(path)), eq(LAST_CHANGED_REVISION));
    }

    private void verifyContentNotFetched(String path) throws Exception {
        verify(aYamlService, times(0)).getConfig(eq(Path.of(path)), any());
    }

}
