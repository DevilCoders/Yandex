package ru.yandex.ci.engine.flow;

import java.nio.file.Path;
import java.time.Instant;
import java.util.Collections;
import java.util.List;
import java.util.NoSuchElementException;
import java.util.Optional;
import java.util.Set;
import java.util.function.Supplier;

import javax.annotation.Nullable;

import org.junit.jupiter.api.Assertions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockito.Mockito;
import org.mockito.stubbing.Answer;

import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigSecurityState.ValidationStatus;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.SoxConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.table.ConfigHistoryTable;
import ru.yandex.ci.core.db.table.ConfigStateTable;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.core.security.YavTokensTable;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigParseResult;

class SecurityServiceTest {

    private static final String EXISTING_ABC = "ci";
    private static final String NEW_ABC = "testenv";
    private static final String ADMIN_GROUP = "ci";
    private static final String ADMIN_SCOPE = "admin";

    private static final String CI_AUTHOR = "firov";
    private static final String NON_CI_AUTHOR = "andreevdm";
    private static final String NON_CI_SOX_AUTHOR = "miroslav2";
    private static final String CI_ADMIN = "teplosvet";
    private static final String TOKEN_DELEGATOR = "algebraic";

    private static final String NON_CI_AUTHOR_FROM_ABC = "abc_user";

    private static final String EXISTING_SEC = "sec-abc42";
    private static final String NEW_SEC = "sec-def42";
    private static final String INVALID_SEC = "ver-123";

    private static final String EXISTING_FLOW = "flow";
    private static final String NEW_FLOW = "newFlow";

    private static final YavToken.Id EXISTING_TOKEN_UUID = YavToken.Id.of("token-for:" + EXISTING_SEC);
    private static final YavToken EXISTING_TOKEN = YavToken.builder()
            .id(EXISTING_TOKEN_UUID)
            .abcService(EXISTING_ABC)
            .delegatedBy(TOKEN_DELEGATOR)
            .build();

    private SecurityStateService securityStateService;

    @Nullable
    private volatile ConfigEntity configEntity;

    @BeforeEach
    void setUp() {
        AbcService abcService = Mockito.mock(AbcService.class);

        Mockito.when(abcService.isMember(NON_CI_AUTHOR_FROM_ABC, EXISTING_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(CI_AUTHOR, EXISTING_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(NON_CI_AUTHOR, EXISTING_ABC)).thenReturn(false);
        Mockito.when(abcService.isMember(TOKEN_DELEGATOR, EXISTING_ABC)).thenReturn(false);
        Mockito.when(abcService.isMember(CI_ADMIN, EXISTING_ABC)).thenReturn(false);

        Mockito.when(abcService.isMember(CI_AUTHOR, NEW_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(NON_CI_AUTHOR, NEW_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(TOKEN_DELEGATOR, NEW_ABC)).thenReturn(false);
        Mockito.when(abcService.isMember(CI_ADMIN, NEW_ABC)).thenReturn(false);

        Mockito.when(abcService.isMember(CI_AUTHOR, NEW_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(NON_CI_AUTHOR, NEW_ABC)).thenReturn(true);
        Mockito.when(abcService.isMember(TOKEN_DELEGATOR, NEW_ABC)).thenReturn(false);
        Mockito.when(abcService.isMember(CI_ADMIN, NEW_ABC)).thenReturn(false);

        Mockito.when(abcService.isMember(CI_AUTHOR, ADMIN_GROUP, Set.of(ADMIN_SCOPE))).thenReturn(false);
        Mockito.when(abcService.isMember(NON_CI_AUTHOR, ADMIN_GROUP, Set.of(ADMIN_SCOPE))).thenReturn(false);
        Mockito.when(abcService.isMember(TOKEN_DELEGATOR, ADMIN_GROUP, Set.of(ADMIN_SCOPE))).thenReturn(false);
        Mockito.when(abcService.isMember(CI_ADMIN, ADMIN_GROUP, Set.of(ADMIN_SCOPE))).thenReturn(true);

        var yavTokensTable = Mockito.mock(YavTokensTable.class);
        Mockito.when(yavTokensTable.get(EXISTING_TOKEN_UUID)).thenReturn(EXISTING_TOKEN);

        configEntity = null;
        var configHistoryTable = Mockito.mock(ConfigHistoryTable.class);
        Mockito.when(configHistoryTable.findLastReadyConfig(Mockito.any(Path.class), Mockito.any(ArcBranch.class)))
                .thenAnswer(invocation -> Optional.ofNullable(configEntity));

        var configStateTable = Mockito.mock(ConfigStateTable.class);
        Mockito.when(configStateTable.get(Mockito.any(Path.class))).thenThrow(new NoSuchElementException());

        CiMainDb ciMainDb = Mockito.mock(CiMainDb.class);
        Mockito.when(ciMainDb.yavTokensTable()).thenReturn(yavTokensTable);
        Mockito.when(ciMainDb.configHistory()).thenReturn(configHistoryTable);
        Mockito.when(ciMainDb.configStates()).thenReturn(configStateTable);

        //noinspection unchecked
        Mockito.doAnswer((Answer<Object>) invocation ->
                        ((Supplier<?>) invocation.getArgument(0)).get())
                .when(ciMainDb).currentOrReadOnly(Mockito.isA(Supplier.class));

        Mockito.when(abcService.isMember(NON_CI_AUTHOR, EXISTING_ABC, Set.of("sox-scope"))).thenReturn(false);
        Mockito.when(abcService.isMember(NON_CI_SOX_AUTHOR, EXISTING_ABC, Set.of("sox-scope"))).thenReturn(true);

        var accessService = new AccessService(abcService, "ci", "admin");
        var permissionsService = new PermissionsService(accessService, ciMainDb);
        securityStateService = new SecurityStateService(ciMainDb, accessService, permissionsService);
    }

    private static ConfigParseResult validParseResult(String abcService, String secret, boolean configChanged) {
        return validParseResult(abcService, secret, null, configChanged);
    }

    private static ConfigParseResult validParseResult(
            String abcService,
            String secret,
            @Nullable String scope,
            boolean configChanged) {
        String flow = configChanged ? NEW_FLOW : EXISTING_FLOW;
        AYamlConfig aYamlConfig = aYamlConfig(abcService, secret, scope, flow);
        return ConfigParseResult.create(
                aYamlConfig,
                Collections.emptyNavigableMap(),
                Collections.emptyNavigableMap(),
                List.of()
        );
    }

    private static ConfigParseResult invalidParseResult() {
        return ConfigParseResult.singleCrit("", "");
    }

    private static AYamlConfig aYamlConfig(String abcService, String secret, @Nullable String scope, String flow) {
        var ciConfig = CiConfig.builder();
        ciConfig.secret(secret);

        var flowConfig = FlowConfig.builder()
                .id(flow)
                .title(flow)
                .description(flow)
                .build();
        ciConfig.flow(flowConfig);

        var sox = scope != null
                ? SoxConfig.builder().approvalScope(scope).build()
                : null;

        return new AYamlConfig(abcService, "Some service", ciConfig.build(), sox);
    }

    private static ArcCommit commit(String author) {
        return ArcCommit.builder()
                .id(TestData.TRUNK_R4.toCommitId())
                .author(author)
                .message("")
                .createTime(Instant.now())
                .parents(List.of())
                .svnRevision(42)
                .build();
    }


    private ConfigBundle previousBundle(ValidationStatus previousStatus) {
        return previousBundle(previousStatus, null);
    }

    private ConfigBundle previousBundle(ValidationStatus previousStatus, @Nullable String scope) {
        ConfigSecurityState state;
        if (previousStatus.isValid()) {
            state = new ConfigSecurityState(EXISTING_TOKEN_UUID, previousStatus);
        } else {
            state = new ConfigSecurityState(null, previousStatus);
        }

        ConfigEntity entity = new ConfigEntity(
                TestData.CONFIG_PATH_ABC,
                TestData.TRUNK_R4,
                state.isValid() ? ConfigStatus.READY : ConfigStatus.SECURITY_PROBLEM,
                List.of(),
                Collections.emptyNavigableMap(),
                state,
                null,
                null,
                null
        );

        return new ConfigBundle(
                entity,
                aYamlConfig(EXISTING_ABC, EXISTING_SEC, scope, EXISTING_FLOW),
                Collections.emptyNavigableMap()
        );
    }

    @Test
    void noPreviousConfig() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, false),
                commit("algebraic"),
                null,
                ValidationStatus.TOKEN_NOT_FOUND
        );
    }

    @Test
    void invalidNewConfig() {
        check(
                invalidParseResult(),
                commit("algebraic"),
                previousBundle(ValidationStatus.VALID_USER),
                ValidationStatus.INVALID_CONFIG
        );
    }

    @Test
    void noPreviousToken() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, false),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.TOKEN_NOT_FOUND),
                ValidationStatus.TOKEN_NOT_FOUND
        );
    }

    @Test
    void configNotChanged() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, false),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.CONFIG_NOT_CHANGED,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void validUser() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.VALID_USER,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void validUserValidConfig() {
        configEntity = validEntity();
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.VALID_USER,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void adminChanges() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(CI_ADMIN),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.USER_IS_ADMIN,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void invalidUser() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(NON_CI_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.INVALID_USER
        );
    }

    @Test
    void invalidUserCustomConfig() {
        configEntity = validEntity(unacceptedPermissions());
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE),
                ValidationStatus.INVALID_USER
        );
    }

    @Test
    void abcChanged() {
        check(
                validParseResult(NEW_ABC, EXISTING_SEC, true),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.NEW_TOKEN),
                ValidationStatus.ABC_SERVICE_CHANGED
        );
    }

    @Test
    void soxPermittedScopeChanged() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, "sox-scope", true),
                commit(NON_CI_SOX_AUTHOR),
                previousBundle(ValidationStatus.OWNER_APPROVE, "sox-scope"),
                ValidationStatus.VALID_SOX_USER,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void soxNotPermittedScopeChanged() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, "sox-scope", true),
                commit(NON_CI_AUTHOR_FROM_ABC),
                previousBundle(ValidationStatus.OWNER_APPROVE, "sox-scope"),
                ValidationStatus.SOX_NOT_APPROVED
        );
    }

    @Test
    void secretChanged() {
        check(
                validParseResult(EXISTING_ABC, NEW_SEC, true),
                commit(CI_AUTHOR),
                previousBundle(ValidationStatus.CONFIG_NOT_CHANGED),
                ValidationStatus.SECRET_CHANGED
        );
    }

    @Test
    void modifyByTokenDelegator() {
        check(
                validParseResult(EXISTING_ABC, EXISTING_SEC, true),
                commit(TOKEN_DELEGATOR),
                previousBundle(ValidationStatus.VALID_USER),
                ValidationStatus.USER_HAS_TOKEN,
                EXISTING_TOKEN_UUID
        );
    }

    @Test
    void invalidToken() {
        check(
                validParseResult(EXISTING_ABC, INVALID_SEC, true),
                commit("algebraic"),
                previousBundle(ValidationStatus.CONFIG_NOT_CHANGED),
                ValidationStatus.TOKEN_INVALID
        );
    }

    private void check(
            ConfigParseResult newConfigParseResult,
            ArcCommit newConfigCommit,
            @Nullable ConfigBundle previousValidConfig,
            ValidationStatus expectedStatus
    ) {
        check(newConfigParseResult, newConfigCommit, previousValidConfig, expectedStatus, null);
    }

    private void check(
            ConfigParseResult newConfigParseResult,
            ArcCommit newConfigCommit,
            @Nullable ConfigBundle previousValidConfig,
            ValidationStatus expectedStatus,
            @Nullable YavToken.Id expectedToken
    ) {
        Assertions.assertEquals(
                new ConfigSecurityState(expectedToken, expectedStatus),
                securityStateService.getSecurityState(
                        TestData.CONFIG_PATH_ABC,
                        newConfigParseResult,
                        newConfigCommit,
                        previousValidConfig,
                        null
                )
        );
    }

    private ConfigEntity validEntity() {
        return validEntity(null);
    }

    private ConfigEntity validEntity(@Nullable ConfigPermissions permissions) {
        return buildEntity(
                TestData.CONFIG_PATH_ABC,
                ConfigStatus.READY,
                new ConfigSecurityState(YavToken.Id.of("123"), ValidationStatus.NEW_TOKEN),
                permissions
        );
    }

    private static ConfigEntity buildEntity(
            Path path,
            ConfigStatus status,
            ConfigSecurityState securityState,
            @Nullable ConfigPermissions permissions
    ) {
        return ConfigEntity.builder()
                .id(ConfigEntity.Id.of(path.toString(), "trunk", 1))
                .status(status)
                .securityState(securityState)
                .problems(List.of())
                .permissions(permissions)
                .build();
    }

    private static ConfigPermissions unacceptedPermissions() {
        return ConfigPermissions.builder()
                .project(EXISTING_ABC)
                .approval(PermissionRule.ofScopes(NEW_ABC)) // not matched with EXISTING_ABC
                .build();
    }
}
