package ru.yandex.ci.core.security;

import java.nio.file.Path;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.TreeMap;
import java.util.function.Consumer;
import java.util.stream.Stream;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.client.abc.AbcServiceMemberInfo;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Name;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Person;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Role;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Scope;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Service;
import ru.yandex.ci.client.abc.AbcUserDutyInfo;
import ru.yandex.ci.client.abc.AbcUserDutyInfo.Schedule;
import ru.yandex.ci.core.abc.Abc;
import ru.yandex.ci.core.abc.AbcService;
import ru.yandex.ci.core.abc.AbcServiceStub;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigSecurityState.ValidationStatus;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.PermissionForOwner;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.spring.AbcStubConfig;
import ru.yandex.ci.core.spring.clients.AbcClientTestConfig;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.spring.PermissionsConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.clock.OverridableClock;

import static org.assertj.core.api.Assertions.assertThatThrownBy;

@ContextConfiguration(classes = {
        PermissionsConfig.class,
        AbcStubConfig.class,
        AbcClientTestConfig.class
})
public class PermissionsServiceTest extends YdbCiTestBase {

    private static final String SERVICE = Abc.CI.getSlug();

    private final CiProcessId ciProcessIdAction = CiProcessId.ofFlow(Path.of("ci/a.yaml"), "action");
    private final CiProcessId ciProcessIdRelease = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release");
    private final String someUser = "username";
    private final String defaultUser = TestData.CI_USER;

    private final String noConfigTrunk = "INTERNAL: Cannot check permission scope for config ci/a.yaml, " +
            "no config with status READY found in trunk";
    private final String noConfigRevision = "Unable to find key [ConfigEntity.Id(configPath=ci/a.yaml, " +
            "branch=trunk, commitNumber=2)] in table [main/ConfigHistory]";
    private final String noConfigState = "Unable to find key [ConfigState.Id(configPath=ci/a.yaml)] " +
            "in table [main/ConfigState]";

    @Autowired
    private PermissionsService permissionsService;

    @Autowired
    private AbcService abcService;

    @Autowired
    private OverridableClock clock;

    private AbcServiceStub abcServiceStub;

    private OrderedArcRevision revision;

    @BeforeEach
    void init() {
        abcServiceStub = (AbcServiceStub) abcService;

        clock.stop();
        revision = TestData.TRUNK_R2;
    }

    @AfterEach
    void close() {
        clock.flush();
    }

    @Test
    void checkAccessAndPermissionsWithoutConfig() {
        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(noConfigTrunk);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(noConfigTrunk);

        assertThatThrownBy(() ->
                checkJobApprovers(someUser, ciProcessIdAction, revision, "flow", "job", "trigger"))
                .hasMessage(noConfigRevision);
    }

    @Test
    void checkAccessAndPermissionsWithoutConfigHistory() {
        registerConfigState(builder -> {
        });

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(noConfigTrunk);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(noConfigTrunk);

        assertThatThrownBy(() ->
                checkJobApprovers(someUser, ciProcessIdAction, revision, "flow", "job", "trigger"))
                .hasMessage(noConfigRevision);
    }

    @Test
    void checkAccessDefaultWithNullPermissionsAndNoConfigState() {
        registerConfigEntity(ConfigStatus.READY, null);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(noConfigState);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(noConfigState);

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(noConfigState);

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(noConfigState);
    }


    @Test
    void checkAccessDefaultWithNullPermissionsAndConfigState() {
        registerConfigEntity(ConfigStatus.READY, null);
        registerConfigState(builder -> {
        });

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW);
        checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW);
    }


    @Test
    void checkAccessRevisionDefault() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW);
        checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW);
    }

    @Test
    void checkAccessAnyRevisionDefault() {
        registerConfigEntity(ConfigStatus.READY, TestData.TRUNK_R1, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        // Not OK for start_job (with revision)
        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(noConfigRevision);

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(noConfigRevision);

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(noConfigRevision);

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(noConfigRevision);


        // OK for start_flow (any revision with ready)
        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW);
        checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW);
    }

    @Test
    void checkAccessAnyRevisioMany() {
        registerConfigEntity(ConfigStatus.READY, TestData.TRUNK_R1, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        registerConfigEntity(ConfigStatus.SECURITY_PROBLEM, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        // OK for start_flow (any revision with ready)
        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW);
        checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW);
    }

    @Test
    void checkAccessRevisionDefaultReset() {
        abcServiceStub.reset();

        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(defaultUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(defaultUser, PermissionScope.START_JOB));


        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdAction, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(defaultUser, PermissionScope.START_FLOW));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdRelease, PermissionScope.START_FLOW))
                .hasMessage(userDeniedDefault(defaultUser, PermissionScope.START_FLOW));

    }

    @Test
    void checkAccessRevisionConfiguredDefault() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), Permissions.EMPTY)
                .release(ciProcessIdRelease.getSubId(), Permissions.EMPTY)
                .build());

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionConfigured() {
        var rule = PermissionRule.ofScopes(SERVICE);
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(rule))
                .release(ciProcessIdRelease.getSubId(), permissions(rule))
                .build());

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionConfiguredWithScope() {
        var rule = PermissionRule.ofScopes(SERVICE, "development");
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(rule))
                .release(ciProcessIdRelease.getSubId(), permissions(rule))
                .build());

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        assertThatThrownBy(() ->
                checkAccess(someUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(defaultUser, PermissionScope.START_JOB, rule));

        assertThatThrownBy(() ->
                checkAccess(defaultUser, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(defaultUser, PermissionScope.START_JOB, rule));

    }

    @Test
    void checkAccessRevisionAsOwner() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        var cfg = OwnerConfig.of(TestData.PR_1, someUser);

        checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);

        // Configured for PR actions by default
        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionAsOwnerConfigured() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(PermissionRule.ofScopes(SERVICE))
                        .withDefaultPermissionsForOwner(null))
                .release(ciProcessIdRelease.getSubId(), permissions(PermissionRule.ofScopes(SERVICE))
                        .withDefaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE)))
                .build());

        var cfg = OwnerConfig.of(TestData.PR_1, someUser);

        checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionAsOwnerConfiguredInvalid() {
        var rule = PermissionRule.ofScopes(SERVICE);
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(rule)
                        .withDefaultPermissionsForOwner(null))
                .release(ciProcessIdRelease.getSubId(), permissions(rule)
                        .withDefaultPermissionsForOwner(List.of(PermissionForOwner.PR))) // useless for releases
                .build());

        var cfg = OwnerConfig.of(TestData.PR_1, someUser);
        checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);

        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionAsOwnerForNoPR() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        var cfg = OwnerConfig.of(TestData.TRUNK_R2, someUser);
        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedDefault(someUser, PermissionScope.START_JOB));

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionAsOwnerForNoPRConfigured() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(PermissionRule.ofScopes(SERVICE))
                        .withDefaultPermissionsForOwner(List.of(PermissionForOwner.COMMIT)))
                .release(ciProcessIdRelease.getSubId(), permissions(PermissionRule.ofScopes(SERVICE))
                        .withDefaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE)))
                .build());

        var cfg = OwnerConfig.of(TestData.TRUNK_R2, someUser);
        checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkAccessRevisionAsOwnerDeniedConfigured() {
        var rule = PermissionRule.ofScopes(SERVICE);
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions(rule)
                        .withDefaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE)))
                .release(ciProcessIdRelease.getSubId(), permissions(rule)
                        .withDefaultPermissionsForOwner(List.of()))
                .build());

        var cfg = OwnerConfig.of(TestData.PR_1, someUser);
        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        assertThatThrownBy(() ->
                checkAccess(someUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB))
                .hasMessage(userDeniedRules(someUser, PermissionScope.START_JOB, rule));

        checkAccess(defaultUser, cfg, ciProcessIdAction, revision, PermissionScope.START_JOB);
        checkAccess(defaultUser, cfg, ciProcessIdRelease, revision, PermissionScope.START_JOB);
    }

    @Test
    void checkJobPermissionsNoConfigState() {
        registerConfigEntity(ConfigStatus.READY, null);

        assertThatThrownBy(() ->
                checkJobApprovers(defaultUser, ciProcessIdAction, revision, "flow", "job", "trigger"))
                .hasMessage(noConfigState);
    }

    @ParameterizedTest
    @MethodSource("invalidConfigStatuses")
    void checkJobPermissionsConfigStatusInvalid(ConfigStatus status) {
        registerConfigEntity(status, null);

        assertThatThrownBy(() ->
                checkJobApprovers(defaultUser, ciProcessIdAction, revision, "flow", "job", "trigger"))
                .hasMessage(configHistoryInvalidState(status));
    }


    @Test
    void checkJobPermissionsDefault() {
        registerConfigEntity(ConfigStatus.READY, null);
        registerConfigState(builder -> {
        });

        // No flow configuration
        checkJobApprovers(someUser, ciProcessIdAction, revision, "flow", "job", "trigger");
        checkJobApprovers(defaultUser, ciProcessIdAction, revision, "flow", "job", "trigger");
    }

    @Test
    void checkJobPermissionsWithJobFlowNew() {
        var rule = PermissionRule.ofScopes(SERVICE);
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .flow("flow", ConfigPermissions.FlowPermissions.builder()
                        .jobApprover("job", List.of(rule))
                        .build())
                .build()
        );

        assertThatThrownBy(() ->
                checkJobApprovers(someUser, ciProcessIdAction, revision, "flow", "job", "trigger"))
                .hasMessage(triggerDenied(someUser, rule));

        checkJobApprovers(defaultUser, ciProcessIdAction, revision, "flow", "job", "trigger");
    }

    @ParameterizedTest
    @MethodSource("checkPermissionsWithOption")
    void checkPermissionsWithAllOptions(boolean expectAccess, PermissionRule rule) {
        var permissions = permissions(rule);
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .action(ciProcessIdAction.getSubId(), permissions)
                .release(ciProcessIdRelease.getSubId(), permissions)
                .build());

        abcServiceStub.addUserMembership(someUser, new AbcServiceMemberInfo(
                new Service(SERVICE),
                new Role(
                        1L,
                        "role",
                        new Name("role.en", "Роль.ру"),
                        new Scope(10L, "scope.slug",
                                new Name("scope.en", "Скоуп.ру"))),
                new Person(someUser)
        ));

        // Unsupported, not approved
        abcServiceStub.addDuty(someUser, new AbcUserDutyInfo(
                101L,
                new Schedule(1001L, "Расписание1", "schedule.slug1"),
                false,
                clock.instant().minusSeconds(1),
                clock.instant().plusSeconds(1)
        ));

        abcServiceStub.addDuty(someUser, new AbcUserDutyInfo(
                100L,
                new Schedule(1000L, "Расписание", "schedule.slug"),
                true,
                clock.instant().minusSeconds(1),
                clock.instant().plusSeconds(1)
        ));

        // Unsupported, out of range
        abcServiceStub.addDuty(someUser, new AbcUserDutyInfo(
                102L,
                new Schedule(1002L, "Расписание2", "schedule.slug2"),
                true,
                clock.instant().plusSeconds(2),
                clock.instant().plusSeconds(3)
        ));

        Runnable runAction = () ->
                checkAccess(someUser, ciProcessIdAction, PermissionScope.START_FLOW);

        Runnable runRelease = () ->
                checkAccess(someUser, ciProcessIdRelease, PermissionScope.START_FLOW);

        if (expectAccess) {
            runAction.run();
            runRelease.run();
        } else {
            assertThatThrownBy(runAction::run)
                    .hasMessage(userDeniedRules(someUser, PermissionScope.START_FLOW, rule));
            assertThatThrownBy(runRelease::run)
                    .hasMessage(userDeniedRules(someUser, PermissionScope.START_FLOW, rule));
        }
    }

    @Test
    void checkAccessWithoutRevision() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.of(SERVICE));

        var check = new PermissionChecks();
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.MODIFY));
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.START_FLOW));
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.START_HOTFIX));
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.ROLLBACK_FLOW));
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.ADD_COMMIT));
        checkAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.CREATE_BRANCH));
        checkBadAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.CANCEL_FLOW));
        checkBadAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.START_JOB));
        checkBadAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.KILL_JOB));
        checkBadAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.SKIP_JOB));
        checkBadAccess(defaultUser, ciProcessIdAction, check.scope(PermissionScope.MANUAL_TRIGGER));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.TOGGLE_AUTORUN));
        check.verify();
    }

    @Test
    void checkAccessWithRevision() {
        registerConfigEntity(ConfigStatus.READY, ConfigPermissions.builder()
                .project(SERVICE)
                .build());

        var check = new PermissionChecks();
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.MODIFY));
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.CANCEL_FLOW));
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.START_JOB));
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.KILL_JOB));
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.SKIP_JOB));
        checkAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.MANUAL_TRIGGER));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.START_FLOW));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.START_HOTFIX));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.ROLLBACK_FLOW));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.ADD_COMMIT));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.CREATE_BRANCH));
        checkBadAccess(defaultUser, ciProcessIdAction, revision, check.scope(PermissionScope.TOGGLE_AUTORUN));
        check.verify();

        assertThatThrownBy(() ->
                permissionsService.checkAccess(defaultUser, ciProcessIdAction, null, PermissionScope.MODIFY))
                .hasMessage("ConfigRevision cannot be null when checking permission scope %s"
                        .formatted(PermissionScope.MODIFY));
    }

    private void checkBadAccess(String login, CiProcessId ciProcessId, PermissionScope scope) {
        assertThatThrownBy(() -> permissionsService.checkAccess(login, ciProcessId, scope))
                .hasMessage("Permission scope %s cannot be used without config revision".formatted(scope));
    }

    private void checkBadAccess(String login, CiProcessId ciProcessId, OrderedArcRevision rev, PermissionScope scope) {
        assertThatThrownBy(() -> permissionsService.checkAccess(login, ciProcessId, rev, scope))
                .hasMessage("Permission scope %s cannot be used with config revision".formatted(scope));
    }

    private void checkAccess(String login, CiProcessId ciProcessId, PermissionScope scope) {
        permissionsService.checkAccess(login, ciProcessId, scope);
    }

    private void checkAccess(String login, CiProcessId ciProcessId, OrderedArcRevision rev, PermissionScope scope) {
        checkAccess(login, null, ciProcessId, rev, scope);
    }

    private void checkAccess(
            String login,
            @Nullable OwnerConfig owner,
            CiProcessId ciProcessId,
            OrderedArcRevision rev,
            PermissionScope scope
    ) {
        permissionsService.checkAccess(login, owner, ciProcessId, rev, scope);
    }

    private void checkJobApprovers(String login, CiProcessId ciProcessId, OrderedArcRevision configRevision,
                                   String flowId, String jobId, String message) {
        permissionsService.checkJobApprovers(login, ciProcessId, configRevision, flowId, jobId, message);
    }

    private void registerConfigState(Consumer<ConfigState.Builder> builder) {
        var config = ConfigState.builder()
                .id(ConfigState.Id.of(ciProcessIdAction.getPath()))
                .project(SERVICE);
        builder.accept(config);
        db.currentOrTx(() -> db.configStates().save(config.build()));
    }

    private void registerConfigEntity(ConfigStatus configStatus, @Nullable ConfigPermissions permissions) {
        registerConfigEntity(configStatus, revision, permissions);
    }

    private void registerConfigEntity(
            ConfigStatus configStatus,
            OrderedArcRevision revision,
            @Nullable ConfigPermissions permissions
    ) {
        var config = ConfigEntity.builder()
                .id(ConfigEntity.Id.of(ciProcessIdAction.getPath(), revision))
                .commitId(revision.getCommitId())
                .permissions(permissions)
                .status(configStatus)
                .problems(configStatus == ConfigStatus.INVALID
                        ? List.of(ConfigProblem.crit("error"))
                        : List.of())
                .securityState(configStatus != ConfigStatus.READY
                        ? new ConfigSecurityState(null, ValidationStatus.INVALID_USER)
                        : new ConfigSecurityState(YavToken.Id.of("123"), ValidationStatus.NEW_TOKEN))
                .taskRevisions(new TreeMap<>())
                .build();
        db.currentOrTx(() -> db.configHistory().save(config));
    }


    private String userDeniedDefault(String username, PermissionScope scope) {
        return "PERMISSION_DENIED: User [%s] has no access to scope [%s], not in project [ci]"
                .formatted(username, scope);
    }

    private String userDeniedRules(String username, PermissionScope scope, PermissionRule... rules) {
        return "PERMISSION_DENIED: User [%s] has no access to scope [%s] within rules %s"
                .formatted(username, scope, List.of(rules));
    }

    private String triggerDenied(String username, PermissionRule... rules) {
        return "PERMISSION_DENIED: User [%s] has no access to trigger within rules %s"
                .formatted(username, List.of(rules));
    }

    private String configHistoryInvalidState(ConfigStatus status) {
        return "INTERNAL: Cannot check permission scope for config ci/a.yaml on revision %s with status %s"
                .formatted(revision, status);
    }

    static List<ConfigStatus> invalidConfigStatuses() {
        return Stream.of(ConfigStatus.values())
                .filter(status -> status != ConfigStatus.READY)
                .toList();
    }

    static List<Arguments> checkPermissionsWithOption() {
        return List.of(
                Arguments.of(true, PermissionRule.ofScopes(SERVICE)),
                Arguments.of(false, PermissionRule.ofScopes("unknown-service")),

                Arguments.of(true, PermissionRule.ofScopes(SERVICE, "scope.slug")),
                Arguments.of(true, PermissionRule.ofScopes(SERVICE, "scope.en")),
                Arguments.of(true, PermissionRule.ofScopes(SERVICE, "Скоуп.ру")),
                Arguments.of(true, PermissionRule.ofScopes(SERVICE, "10")),
                Arguments.of(true, PermissionRule.ofScopes(SERVICE, "scope.slug1", "scope.slug")),

                Arguments.of(false, PermissionRule.ofScopes(SERVICE, "scope.slug1")),
                Arguments.of(false, PermissionRule.ofScopes(SERVICE, "scope.en1")),
                Arguments.of(false, PermissionRule.ofScopes(SERVICE, "Скоуп.ру1")),
                Arguments.of(false, PermissionRule.ofScopes(SERVICE, "101")),
                Arguments.of(false, PermissionRule.ofScopes(SERVICE, "scope.slug1", "scope.slug2")),

                Arguments.of(true, PermissionRule.ofRoles(SERVICE, "role")),
                Arguments.of(true, PermissionRule.ofRoles(SERVICE, "role.en")),
                Arguments.of(true, PermissionRule.ofRoles(SERVICE, "Роль.ру")),
                Arguments.of(true, PermissionRule.ofRoles(SERVICE, "1")),
                Arguments.of(true, PermissionRule.ofRoles(SERVICE, "role1", "role")),
                Arguments.of(true, PermissionRule.of(SERVICE, List.of("scope.slug1"), List.of("role"), List.of())),

                Arguments.of(false, PermissionRule.ofRoles(SERVICE, "role1")),
                Arguments.of(false, PermissionRule.ofRoles(SERVICE, "role.en1")),
                Arguments.of(false, PermissionRule.ofRoles(SERVICE, "Роль.ру1")),
                Arguments.of(false, PermissionRule.ofRoles(SERVICE, "11")),
                Arguments.of(false, PermissionRule.of(SERVICE, List.of("scope.slug1"), List.of("role1"), List.of())),

                Arguments.of(true, PermissionRule.ofDuties(SERVICE, "schedule.slug")),
                Arguments.of(true, PermissionRule.ofDuties(SERVICE, "Расписание")),
                Arguments.of(true, PermissionRule.ofDuties(SERVICE, "1000")),
                Arguments.of(true, PermissionRule.ofDuties(SERVICE, "schedule.slug1", "schedule.slug")),
                Arguments.of(true, PermissionRule.of(SERVICE,
                        List.of("scope.slug1"), List.of("role1"), List.of("schedule.slug"))),

                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "schedule.slug1")),
                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "Расписание1")),
                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "1001")),

                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "schedule.slug2")),
                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "Расписание2")),
                Arguments.of(false, PermissionRule.ofDuties(SERVICE, "1002")),
                Arguments.of(false, PermissionRule.of(SERVICE,
                        List.of("scope.slug1"), List.of("role1"), List.of("schedule.slug1"))),
                Arguments.of(true, PermissionRule.of(SERVICE,
                        List.of("scope.slug1"), List.of("role1"), List.of("schedule.slug"))) // Again with new time

        );
    }

    static Permissions permissions(PermissionRule rule) {
        return Permissions.builder().add(PermissionScope.START_FLOW, rule).build();
    }

    static class PermissionChecks {
        private final Set<PermissionScope> scopes = new HashSet<>();

        PermissionScope scope(PermissionScope scope) {
            Preconditions.checkState(scopes.add(scope), "Duplicate check for scope %s", scope);
            return scope;
        }

        void verify() {
            var unchecked = new HashSet<>(Set.of(PermissionScope.values()));
            unchecked.removeAll(scopes);
            Preconditions.checkState(unchecked.isEmpty(), "Scopes were not checked: %s", unchecked);
        }
    }
}
