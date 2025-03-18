package ru.yandex.ci.engine.config;

import java.nio.file.Path;
import java.time.Duration;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.NavigableMap;
import java.util.TreeMap;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.arcanum.util.RevisionNumberPullRequestIdPair;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigCreationInfo;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.VirtualCiProcessId.VirtualType;
import ru.yandex.ci.core.config.a.AYamlService;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.EngineTestBase;
import ru.yandex.ci.engine.flow.YavDelegationException;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.core.test.TestData.DIFF_SET_1;
import static ru.yandex.ci.core.test.TestData.DIFF_SET_2;
import static ru.yandex.ci.core.test.TestData.DIFF_SET_3;
import static ru.yandex.ci.core.test.TestData.TRUNK_R3;
import static ru.yandex.ci.core.test.TestData.TRUNK_R4;
import static ru.yandex.ci.core.test.TestData.TRUNK_R5;

public class ConfigurationServiceTest extends EngineTestBase {

    private static final ConfigSecurityState TOKEN_NOT_FOUND_SECURITY_STATE = new ConfigSecurityState(
            null, ConfigSecurityState.ValidationStatus.TOKEN_NOT_FOUND
    );

    private static final ConfigSecurityState INVALID_CONFIG_SECURITY_STATE = new ConfigSecurityState(
            null, ConfigSecurityState.ValidationStatus.INVALID_CONFIG
    );

    private static final ConfigSecurityState NEW_TOKEN_SECURITY_STATE = new ConfigSecurityState(
            TestData.YAV_TOKEN_UUID, ConfigSecurityState.ValidationStatus.NEW_TOKEN
    );

    private static final ConfigSecurityState CONFIG_NOT_CHANGED_SECURITY_STATE = new ConfigSecurityState(
            TestData.YAV_TOKEN_UUID, ConfigSecurityState.ValidationStatus.CONFIG_NOT_CHANGED
    );

    private static final NavigableMap<TaskId, ArcRevision> R1_TASK_REVISIONS = new TreeMap<>(Map.of(
            TaskId.of("demo/woodflow/sawmill"), TestData.TRUNK_R2.toRevision(),
            TaskId.of("demo/woodflow/woodcutter"), TestData.TRUNK_R2.toRevision()
    ));

    private static final NavigableMap<TaskId, ArcRevision> R4_TASK_REVISIONS = new TreeMap<>(Map.of(
            TaskId.of("demo/woodflow/sawmill"), TestData.TRUNK_R5.toRevision(),
            TaskId.of("demo/woodflow/woodcutter"), TestData.TRUNK_R2.toRevision()
    ));

    private static final ConfigCreationInfo NEW_CREATION_INFO = new ConfigCreationInfo(NOW, null, null, null);

    @BeforeEach
    void setUp() {
        mockYav();

        arcanumTestServer.mockGetReviewRequestBySvnRevision(
                RevisionNumberPullRequestIdPair.of(TRUNK_R3.getNumber(), DIFF_SET_1.getPullRequestId()),
                RevisionNumberPullRequestIdPair.of(TRUNK_R4.getNumber(), DIFF_SET_2.getPullRequestId()),
                RevisionNumberPullRequestIdPair.of(TRUNK_R5.getNumber(), DIFF_SET_3.getPullRequestId())
        );

        mockValidationSuccessful();
    }


    @Test
    void trunkPreviousRevisions() {

        ConfigEntity expectedR1Entity = expectedR1Entity(TOKEN_NOT_FOUND_SECURITY_STATE);
        ConfigEntity expectedR2Entity = expectedR2Entity();
        ConfigEntity expectedR3Entity = expectedR3Entity(TOKEN_NOT_FOUND_SECURITY_STATE);

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R3)
                        .orElseThrow().getConfigEntity()
        ).isEqualToIgnoringGivenFields(expectedR2Entity, "problems");

        assertThat(
                configurationService.getConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R3)
                        .getConfigEntity()
        ).isEqualToIgnoringGivenFields(expectedR2Entity, "problems");

        assertThat(
                db.currentOrReadOnly(() ->
                        db.configHistory().getById(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R3))
        ).isEqualToIgnoringGivenFields(expectedR2Entity, "problems");

        assertThat(
                configurationService.getLastValidConfig(TestData.CONFIG_PATH_ABC, ArcBranch.trunk())
                        .getConfigEntity()
        ).isEqualTo(expectedR1Entity);

        assertThat(configurationService.getLastReadyOrNotCiConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R2))
                .isEmpty();

        assertThat(configurationService.getLastReadyOrNotCiConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R1))
                .isEmpty();

        assertThat(configurationService.findConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4))
                .isEmpty();

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .orElseThrow().getConfigEntity()
        ).isEqualTo(expectedR3Entity);

        assertThat(
                configurationService.findConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .orElseThrow()
                        .getConfigEntity()
        ).isEqualTo(expectedR3Entity);


    }

    @Test
    void dontRecreateConfigs() {
        clock.plus(Duration.ofDays(1));

        assertThat(configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R3))
                .isNotEmpty();

        delegateToken(TestData.CONFIG_PATH_ABC);

        var r1Original = configurationService.getLastReadyOrNotCiConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R2)
                .orElseThrow();
        assertThat(r1Original).isNotNull();

        var r1CreationInfo = r1Original.getConfigEntity().getCreationInfo();
        assertThat(r1CreationInfo.getCreated())
                .isEqualTo(clock.instant())
                .isNotEqualTo(NOW);

        clock.plus(Duration.ofDays(3));
        assertThat(configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4))
                .isNotEmpty();

        var r1Fresh = configurationService.getLastReadyOrNotCiConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R2)
                .orElseThrow();
        assertThat(r1Fresh).isNotNull();

        assertThat(r1Fresh.getConfigEntity().getCreationInfo().getCreated())
                .isEqualTo(r1CreationInfo.getCreated());
    }

    @Test
    void trunkDelegateNewToken() throws YavDelegationException {
        ConfigEntity expectedR3Entity = expectedR3Entity(TOKEN_NOT_FOUND_SECURITY_STATE);

        ConfigBundle configBundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                .orElseThrow();

        assertThat(configBundle.getConfigEntity())
                .isEqualTo(expectedR3Entity);

        assertThat(
                configurationService.getConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .getConfigEntity()
        ).isEqualTo(expectedR3Entity);

        securityDelegationService.delegateYavTokens(configBundle, TestData.USER_TICKET, TestData.CI_USER);

        ConfigEntity expectedR3ReadyEntity = expectedR3Entity(NEW_TOKEN_SECURITY_STATE);

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .orElseThrow().getConfigEntity()
        ).isEqualTo(expectedR3ReadyEntity);

        assertThat(
                configurationService.getConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .getConfigEntity()
        ).isEqualTo(expectedR3ReadyEntity);
    }

    @Test
    void existingTokenOnConfigReprocessing() throws YavDelegationException {
        var configBundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                .orElseThrow();
        securityDelegationService.delegateYavTokens(configBundle, TestData.USER_TICKET, TestData.CI_USER);
        db.currentOrTx(() -> db.configHistory().deleteAll());
        assertThat(configurationService.findConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4))
                .isEmpty();

        configBundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                .orElseThrow();
        ConfigEntity expectedR3Entity = expectedR3Entity(NEW_TOKEN_SECURITY_STATE);

        assertThat(configBundle.getConfigEntity()).isEqualTo(expectedR3Entity);

        assertThat(
                configurationService.findConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R4)
                        .orElseThrow()
                        .getConfigEntity()
        ).isEqualTo(expectedR3Entity);
    }


    @Test
    void trunkOnlyTaskChange() throws YavDelegationException {
        ConfigBundle r3ConfigBundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R4).orElseThrow();
        assertThat(r3ConfigBundle.getConfigEntity())
                .isEqualTo(expectedR3Entity(TOKEN_NOT_FOUND_SECURITY_STATE));
        securityDelegationService.delegateYavTokens(r3ConfigBundle, TestData.USER_TICKET, TestData.CI_USER);


        ConfigEntity expectedR4Entity = expectedR4Entity(CONFIG_NOT_CHANGED_SECURITY_STATE);

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R5)
                        .orElseThrow().getConfigEntity()
        ).isEqualTo(expectedR4Entity);

        assertThat(
                configurationService.getConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R5)
                        .getConfigEntity()
        ).isEqualTo(expectedR4Entity);
    }

    @Test
    void trunkConfigNotChanged() {

        ConfigEntity expectedR4Entity = expectedR4Entity(TOKEN_NOT_FOUND_SECURITY_STATE);

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R5)
                        .orElseThrow().getConfigEntity()
        ).isEqualTo(expectedR4Entity);

        assertThat(
                configurationService.getConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R5)
                        .getConfigEntity()
        ).isEqualTo(expectedR4Entity);


        //Check no changes on revision r5
        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R6)
                        .orElseThrow().getConfigEntity()
        ).isEqualTo(expectedR4Entity);

        assertThat(configurationService.findConfig(TestData.CONFIG_PATH_ABC, TestData.TRUNK_R6))
                .isEmpty();
    }

    @Test
    void processNewRevisionsAfterInvalidConfig() {
        ConfigEntity expectedEntity = new ConfigEntity(
                TestData.CONFIG_PATH_INVALID_SIMPLE,
                TestData.TRUNK_R2,
                ConfigStatus.INVALID,
                List.of(ConfigProblem.crit("42")),
                new TreeMap<>(),
                INVALID_CONFIG_SECURITY_STATE,
                new ConfigCreationInfo(NOW, null, null, null),
                null,
                "sid-hugo"
        );

        ConfigBundle expectedBundle = new ConfigBundle(expectedEntity, null, Collections.emptyNavigableMap());
        ConfigBundle actualBundle = configurationService.getOrCreateConfig(
                TestData.CONFIG_PATH_INVALID_SIMPLE, TestData.TRUNK_R6
        ).orElseThrow();

        assertThat(actualBundle)
                .isEqualToIgnoringGivenFields(expectedBundle, "configEntity");

        assertThat(actualBundle.getConfigEntity())
                .isEqualToIgnoringGivenFields(expectedEntity, "problems");
    }

    @Test
    void movedConfig() {
        ConfigEntity expected = new ConfigEntity(
                TestData.CONFIG_PATH_MOVED_TO,
                TestData.TRUNK_R5,
                ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R4_TASK_REVISIONS,
                TOKEN_NOT_FOUND_SECURITY_STATE,
                new ConfigCreationInfo(NOW, null, null, null),
                ConfigPermissions.of("ci"),
                "pochemuto"
        );

        assertThat(
                configurationService.getOrCreateConfig(TestData.CONFIG_PATH_MOVED_TO, TestData.TRUNK_R6)
                        .orElseThrow()
                        .getConfigEntity()
        ).isEqualTo(expected);

        //Тут главное проверить, что выше не будет ошибки, но на свякий ещё проверяем,
        //что не появилось других версий со времен перемещения
        db.currentOrReadOnly(() -> {
            assertThat(db.configHistory().findLastConfig(TestData.CONFIG_PATH_MOVED_TO, TestData.TRUNK_R4))
                    .isEmpty();
            assertThat(db.configHistory().findLastConfig(TestData.CONFIG_PATH_MOVED_FROM, TestData.TRUNK_R4))
                    .isEmpty();
        });
    }

    @Test
    void newPrConfig() throws YavDelegationException {
        initDiffSets();

        OrderedArcRevision revision = TestData.DIFF_SET_1.getOrderedMergeRevision();

        ConfigBundle actualBundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_PR_NEW, revision)
                .orElseThrow();

        ConfigEntity expectedEntity = new ConfigEntity(
                TestData.CONFIG_PATH_PR_NEW,
                revision,
                ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R1_TASK_REVISIONS,
                TOKEN_NOT_FOUND_SECURITY_STATE,
                NEW_CREATION_INFO,
                ConfigPermissions.of("ci"),
                "andreevdm"
        );
        assertThat(actualBundle.getConfigEntity()).isEqualTo(expectedEntity);

        securityDelegationService.delegateYavTokens(actualBundle, TestData.USER_TICKET, TestData.CI_USER);

        ConfigEntity expectedReadyEntity = new ConfigEntity(
                TestData.CONFIG_PATH_PR_NEW,
                revision,
                ConfigStatus.READY,
                List.of(),
                R1_TASK_REVISIONS,
                NEW_TOKEN_SECURITY_STATE,
                NEW_CREATION_INFO,
                ConfigPermissions.of("ci"),
                "andreevdm"
        );

        assertThat(configurationService.getConfig(TestData.CONFIG_PATH_PR_NEW, revision))
                .extracting(ConfigBundle::getConfigEntity)
                .isEqualTo(expectedReadyEntity);

    }

    @Test
    void existingPrConfig() {
        var revision = TestData.DIFF_SET_1.getOrderedMergeRevision();
        var config = new ConfigEntity(
                TestData.CONFIG_PATH_PR_NEW,
                revision,
                ConfigStatus.INVALID,
                List.of(ConfigProblem.crit("crit")),
                new TreeMap<>(),
                new ConfigSecurityState(null, ConfigSecurityState.ValidationStatus.INVALID_CONFIG),
                NEW_CREATION_INFO,
                ConfigPermissions.of("ci"),
                "andreevdm"
        );
        db.currentOrTx(() ->
                db.configHistory().save(config));

        // No exception
        assertThat(configurationService.getConfig(TestData.CONFIG_PATH_PR_NEW, revision))
                .extracting(ConfigBundle::getConfigEntity)
                .isEqualTo(config);
    }

    @Test
    void testDelegateTokenInReview() throws YavDelegationException {
        initDiffSets();
        OrderedArcRevision ds3MergeRevision = TestData.DIFF_SET_3.getOrderedMergeRevision();

        ConfigBundle ds3Bundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_PR_NEW, ds3MergeRevision)
                .orElseThrow();
        securityDelegationService.delegateYavTokens(ds3Bundle, TestData.USER_TICKET, TestData.CI_USER);

        ConfigBundle r4Bundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_PR_NEW, TestData.TRUNK_R5)
                .orElseThrow();

        ConfigEntity expectedTrunkEntity = new ConfigEntity(
                TestData.CONFIG_PATH_PR_NEW,
                TestData.TRUNK_R5,
                ConfigStatus.READY,
                List.of(),
                R4_TASK_REVISIONS,
                NEW_TOKEN_SECURITY_STATE,
                new ConfigCreationInfo(
                        NOW,
                        null,
                        null,
                        ds3MergeRevision
                ),
                ConfigPermissions.of("ci"),
                "pochemuto"
        );
        assertThat(r4Bundle.getConfigEntity()).isEqualTo(expectedTrunkEntity);
    }

    @Test
    void delegateTokenInPreviousDiffSet() throws YavDelegationException {
        initDiffSets();

        OrderedArcRevision ds1MergeRevision = TestData.DIFF_SET_1.getOrderedMergeRevision();
        ConfigBundle ds1Bundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_PR_NEW, ds1MergeRevision)
                .orElseThrow();
        securityDelegationService.delegateYavTokens(ds1Bundle, TestData.USER_TICKET, TestData.CI_USER);

        OrderedArcRevision ds2MergeRevision = TestData.DIFF_SET_2.getOrderedMergeRevision();
        ConfigBundle ds2Bundle = configurationService.getOrCreateConfig(TestData.CONFIG_PATH_PR_NEW, ds2MergeRevision)
                .orElseThrow();

        ConfigEntity expectedDs2Entity = new ConfigEntity(
                TestData.CONFIG_PATH_PR_NEW,
                ds2MergeRevision,
                ConfigStatus.READY,
                List.of(),
                R1_TASK_REVISIONS,
                NEW_TOKEN_SECURITY_STATE,
                new ConfigCreationInfo(
                        NOW,
                        null,
                        null,
                        ds1MergeRevision
                ),
                ConfigPermissions.of("ci"),
                "andreevdm"
        );

        assertThat(ds2Bundle.getConfigEntity()).isEqualTo(expectedDs2Entity);
    }

    @Test
    void changeConfigWithReviewNewToken() throws YavDelegationException {
        initDiffSets();
        ConfigBundle r1Bundle = configurationService.getOrCreateConfig(
                TestData.CONFIG_PATH_CHANGE_DS1, TestData.TRUNK_R2
        ).orElseThrow();
        securityDelegationService.delegateYavTokens(r1Bundle, TestData.USER_TICKET, TestData.CI_USER);

        OrderedArcRevision ds1MergeRevision = TestData.DIFF_SET_1.getOrderedMergeRevision();
        ConfigBundle ds1Bundle = configurationService.getOrCreateConfig(
                TestData.CONFIG_PATH_CHANGE_DS1, ds1MergeRevision
        ).orElseThrow();

        ConfigEntity expectedDs1Entity = new ConfigEntity(
                TestData.CONFIG_PATH_CHANGE_DS1,
                ds1MergeRevision,
                ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R1_TASK_REVISIONS,
                new ConfigSecurityState(null, ConfigSecurityState.ValidationStatus.ABC_SERVICE_CHANGED),
                new ConfigCreationInfo(
                        NOW,
                        TestData.TRUNK_R2,
                        TestData.TRUNK_R2,
                        null
                ),
                ConfigPermissions.of("ci"),
                "andreevdm"
        );

        assertThat(ds1Bundle.getConfigEntity()).isEqualTo(expectedDs1Entity);
        securityDelegationService.delegateYavTokens(ds1Bundle, TestData.USER_TICKET_2, TestData.CI_USER);

        ConfigBundle r2Bundle = configurationService.getOrCreateConfig(
                TestData.CONFIG_PATH_CHANGE_DS1, TestData.TRUNK_R3
        ).orElseThrow();

        ConfigEntity expectedR2Entity = new ConfigEntity(
                TestData.CONFIG_PATH_CHANGE_DS1,
                TestData.TRUNK_R3,
                ConfigStatus.READY,
                List.of(),
                R1_TASK_REVISIONS,
                new ConfigSecurityState(TestData.YAV_TOKEN_UUID_2, ConfigSecurityState.ValidationStatus.NEW_TOKEN),
                new ConfigCreationInfo(
                        NOW,
                        TestData.TRUNK_R2,
                        TestData.TRUNK_R2,
                        ds1MergeRevision
                ),
                ConfigPermissions.of("ci"),
                "algebraic"
        );

        assertThat(r2Bundle.getConfigEntity()).isEqualTo(expectedR2Entity);
    }

    @Test
    void getLastValidOrNotCiConfig_whenR2ConfigIsInvalidButR1IsValid() {
        var configPath = AYamlService.dirToConfigPath("valid-at-r1-becomes-invalid-at-r2");
        var r2Bundle = configurationService.getOrCreateConfig(
                configPath, TestData.TRUNK_R2
        ).orElseThrow();
        assertThat(r2Bundle.getStatus().isValidCiConfig()).isTrue();
        assertThat(r2Bundle.getStatus()).isEqualTo(ConfigStatus.SECURITY_PROBLEM);

        delegateToken(configPath);

        var r3Bundle = configurationService.getOrCreateConfig(
                configPath, TestData.TRUNK_R3
        ).orElseThrow();
        assertThat(r3Bundle.getStatus().isValidCiConfig()).isFalse();

        var expectBundle = new ConfigBundle(
                r2Bundle.getConfigEntity().toBuilder()
                        .status(ConfigStatus.READY)
                        .securityState(new ConfigSecurityState(
                                TestData.YAV_TOKEN_UUID,
                                ConfigSecurityState.ValidationStatus.NEW_TOKEN
                        ))
                        .build(),
                r2Bundle.getValidAYamlConfig(),
                r2Bundle.getTaskConfigs()
        );

        assertThat(configurationService.getLastReadyOrNotCiConfig(configPath, TestData.TRUNK_R3))
                .isPresent()
                .get()
                .isEqualTo(expectBundle);
    }

    @Test
    void getLastValidOrNotCiConfig_whenR2ConfigIsNonciButR1IsValid() {
        var configPath = AYamlService.dirToConfigPath("valid-at-r1-becomes-nonci-at-r2");
        var r1Bundle = configurationService.getOrCreateConfig(
                configPath, TestData.TRUNK_R2
        ).orElseThrow();
        assertThat(r1Bundle.getStatus().isValidCiConfig()).isTrue();
        assertThat(r1Bundle.getStatus()).isEqualTo(ConfigStatus.SECURITY_PROBLEM);

        delegateToken(configPath);

        var r3Bundle = configurationService.getOrCreateConfig(
                configPath, TestData.TRUNK_R3
        ).orElseThrow();
        assertThat(r3Bundle.getStatus().isValidCiConfig()).isFalse();

        assertThat(configurationService.getLastReadyOrNotCiConfig(configPath, TestData.TRUNK_R3))
                .isEmpty(); // Non-ci
    }

    @Test
    void configBundleWithVirtualProcessIdLargeTest() {
        ConfigBundle r3ConfigBundle = configurationService.getOrCreateConfig(
                VirtualType.VIRTUAL_LARGE_TEST.getCiProcessId().getPath(),
                TestData.TRUNK_R2)
                .orElseThrow();

        var processId = CiProcessId.ofFlow(Path.of("large-test@ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualProcessId = VirtualCiProcessId.of(processId);
        assertThat(virtualProcessId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_LARGE_TEST);

        var copyBundle = r3ConfigBundle.withVirtualProcessId(virtualProcessId, "ci");
        assertThat(r3ConfigBundle)
                .isNotSameAs(copyBundle)
                .isNotEqualTo(copyBundle);
        assertThat(copyBundle.getValidAYamlConfig().getService())
                .isEqualTo("ci");
    }

    @Test
    void configBundleWithVirtualProcessIdNativeBuild() {
        ConfigBundle r3ConfigBundle = configurationService.getOrCreateConfig(
                VirtualType.VIRTUAL_NATIVE_BUILD.getCiProcessId().getPath(),
                TestData.TRUNK_R2)
                .orElseThrow();

        var processId = CiProcessId.ofFlow(Path.of("native-build@ci/demo-project/large-tests2/a.yaml"),
                "default-linux-x86_64-release@java");
        var virtualProcessId = VirtualCiProcessId.of(processId);
        assertThat(virtualProcessId.getVirtualType())
                .isEqualTo(VirtualType.VIRTUAL_NATIVE_BUILD);

        var copyBundle = r3ConfigBundle.withVirtualProcessId(virtualProcessId, "ci");
        assertThat(r3ConfigBundle)
                .isNotSameAs(copyBundle)
                .isNotEqualTo(copyBundle);
        assertThat(copyBundle.getValidAYamlConfig().getService())
                .isEqualTo("ci");
    }

    private ConfigEntity expectedR1Entity(ConfigSecurityState securityState) {
        return new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R2,
                securityState.isValid() ? ConfigStatus.READY : ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R1_TASK_REVISIONS,
                securityState,
                NEW_CREATION_INFO,
                ConfigPermissions.of("ci"),
                "sid-hugo"
        );
    }

    private ConfigEntity expectedR2Entity() {
        return new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R3,
                ConfigStatus.INVALID,
                List.of(ConfigProblem.crit("42")),
                new TreeMap<>(),
                INVALID_CONFIG_SECURITY_STATE,
                new ConfigCreationInfo(
                        NOW,
                        TestData.TRUNK_R2,
                        TestData.TRUNK_R2,
                        null
                ),
                null,
                "algebraic"
        );
    }

    private ConfigEntity expectedR3Entity(ConfigSecurityState securityState) {
        return new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R4,
                securityState.isValid() ? ConfigStatus.READY : ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R1_TASK_REVISIONS,
                securityState,
                new ConfigCreationInfo(
                        NOW,
                        TestData.TRUNK_R3,
                        TestData.TRUNK_R2,
                        null
                ),
                ConfigPermissions.of("ci"),
                "firov"
        );
    }


    private ConfigEntity expectedR4Entity(ConfigSecurityState securityState) {
        return new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R5,
                securityState.isValid() ? ConfigStatus.READY : ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                R4_TASK_REVISIONS,
                securityState,
                new ConfigCreationInfo(
                        NOW,
                        TestData.TRUNK_R4,
                        TestData.TRUNK_R4,
                        null
                ),
                ConfigPermissions.of("ci"),
                "pochemuto"
        );
    }
}
