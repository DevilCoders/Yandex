package ru.yandex.ci.core.security;

import java.util.List;

import javax.annotation.Nullable;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.github.fge.jsonschema.core.exceptions.ProcessingException;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.params.ParameterizedTest;
import org.junit.jupiter.params.provider.Arguments;
import org.junit.jupiter.params.provider.MethodSource;

import ru.yandex.ci.core.config.a.AYamlParser;
import ru.yandex.ci.core.config.a.model.PermissionForOwner;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.test.TestUtils;

import static org.assertj.core.api.Assertions.assertThat;

class PermissionsProcessorTest {

    @ParameterizedTest
    @MethodSource
    void testOverridePermissions(@Nullable Permissions parent, @Nullable Permissions child, Permissions expect) {
        assertThat(PermissionsProcessor.overridePermissions(parent, child))
                .isEqualTo(expect);
    }

    @ParameterizedTest
    @MethodSource
    void testFinalizePermissions(Permissions permissions, Permissions expect) {
        assertThat(PermissionsProcessor.finalizePermissions(permissions))
                .isEqualTo(expect);
    }

    @Test
    void testOverridePermissionsCompleteExample() throws JsonProcessingException, ProcessingException {
        var aYaml = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/permissions.yaml"));
        assertThat(aYaml.isSuccess()).isTrue();

        var ci = aYaml.getConfig().getCi();
        var action1 = PermissionsProcessor.overridePermissions(ci.getPermissions(),
                ci.getAction("action-1").getPermissions());
        assertThat(action1)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of())
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("ci-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("ci-3"))
                        .add(PermissionScope.ROLLBACK_FLOW,
                                PermissionRule.ofScopes("ci-4"), PermissionRule.ofScopes("ci-5"))
                        .add(PermissionScope.ADD_COMMIT,
                                PermissionRule.ofScopes("ci-6", "development"))
                        .add(PermissionScope.CREATE_BRANCH,
                                PermissionRule.ofScopes("ci-7", "development", "administration"))
                        .add(PermissionScope.START_JOB,
                                PermissionRule.ofScopes("ci-8", "development", "administration"),
                                PermissionRule.ofScopes("ci-9", "dutywork"))
                        .add(PermissionScope.KILL_JOB,
                                PermissionRule.ofScopes("ci-10"),
                                PermissionRule.ofScopes("ci-10", "development"),
                                PermissionRule.ofRoles("ci-10", "Development"),
                                PermissionRule.ofRoles("ci-10", "Development2"),
                                PermissionRule.ofDuties("ci-10", "task-duty"),
                                PermissionRule.ofDuties("ci-10", "2267"),
                                PermissionRule.ofRoles("ci-10-1", "Development"),
                                PermissionRule.ofDuties("ci-10-2", "22671"))
                        .add(PermissionScope.SKIP_JOB,
                                PermissionRule.ofScopes("ci-11", "development"),
                                PermissionRule.ofScopes("ci-11", "administration"),
                                PermissionRule.of("ci-11", List.of("support"), List.of("Support"), List.of("2265")),
                                PermissionRule.of("ci-11", List.of("support2"), List.of("Support2"), List.of("22652")))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci-13"))
                        .add(
                                PermissionScope.TOGGLE_AUTORUN,
                                PermissionRule.of("ci-12",
                                        List.of("development"),
                                        List.of(),
                                        List.of()
                                )
                        )
                        .build());
        var action2 = PermissionsProcessor.overridePermissions(ci.getPermissions(),
                ci.getAction("action-2").getPermissions());
        assertThat(action2)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of(PermissionForOwner.PR, PermissionForOwner.COMMIT))
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("ci-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("ci-3"))
                        .add(PermissionScope.ROLLBACK_FLOW,
                                PermissionRule.ofScopes("ci-4"), PermissionRule.ofScopes("ci-5"))
                        .add(PermissionScope.ADD_COMMIT,
                                PermissionRule.ofScopes("ci-6", "development"))
                        .add(PermissionScope.CREATE_BRANCH,
                                PermissionRule.ofScopes("ci-7", "development", "administration"))
                        .add(PermissionScope.START_JOB,
                                PermissionRule.ofScopes("ci-8", "development", "administration"),
                                PermissionRule.ofScopes("ci-9", "dutywork"))
                        .add(PermissionScope.KILL_JOB,
                                PermissionRule.ofScopes("ci-10"),
                                PermissionRule.ofScopes("ci-10", "development"),
                                PermissionRule.ofRoles("ci-10", "Development"),
                                PermissionRule.ofRoles("ci-10", "Development2"),
                                PermissionRule.ofDuties("ci-10", "task-duty"),
                                PermissionRule.ofDuties("ci-10", "2267"),
                                PermissionRule.ofRoles("ci-10-1", "Development"),
                                PermissionRule.ofDuties("ci-10-2", "22671"))
                        .add(PermissionScope.SKIP_JOB,
                                PermissionRule.ofScopes("ci-11", "development"),
                                PermissionRule.ofScopes("ci-11", "administration"),
                                PermissionRule.of("ci-11", List.of("support"), List.of("Support"), List.of("2265")),
                                PermissionRule.of("ci-11", List.of("support2"), List.of("Support2"), List.of("22652")))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci-13"))
                        .add(
                                PermissionScope.TOGGLE_AUTORUN,
                                PermissionRule.of("ci-12",
                                        List.of("development"),
                                        List.of(),
                                        List.of()
                                )
                        )
                        .build());

        var release1 = PermissionsProcessor.overridePermissions(ci.getPermissions(),
                ci.getRelease("release-1").getPermissions());
        assertThat(release1)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE))
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("citest-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("citest-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("citest-3"))
                        .add(PermissionScope.ROLLBACK_FLOW, PermissionRule.ofScopes("citest-4"))
                        .add(PermissionScope.ADD_COMMIT, PermissionRule.ofScopes("citest-5"))
                        .add(PermissionScope.CREATE_BRANCH, PermissionRule.ofScopes("citest-6"))
                        .add(PermissionScope.START_JOB, PermissionRule.ofScopes("citest-7"))
                        .add(PermissionScope.KILL_JOB, PermissionRule.ofScopes("citest-8"))
                        .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("citest-9"))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("citest-10"))
                        .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("citest-11"))
                        .build());

        var approvals = ci.getApprovals();
        assertThat(approvals)
                .isEqualTo(List.of(
                        PermissionRule.ofScopes("ci-1"),
                        PermissionRule.ofScopes("ci-1", "s-1"),
                        PermissionRule.ofScopes("ci-2", "s-1")
                ));
    }

    @Test
    void testFinalizePermissionsCompleteExample() throws JsonProcessingException, ProcessingException {
        var aYaml = AYamlParser.parseAndValidate(TestUtils.textResource("ayaml/permissions.yaml"));
        assertThat(aYaml.isSuccess()).isTrue();

        var ci = aYaml.getConfig().getCi();
        var action1 = PermissionsProcessor.finalizePermissions(
                PermissionsProcessor.overridePermissions(ci.getPermissions(),
                        ci.getAction("action-1").getPermissions()));
        assertThat(action1)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of())
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("ci-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("ci-3"))
                        .add(PermissionScope.ROLLBACK_FLOW,
                                PermissionRule.ofScopes("ci-4"), PermissionRule.ofScopes("ci-5"))
                        .add(PermissionScope.ADD_COMMIT,
                                PermissionRule.ofScopes("ci-6", "development"))
                        .add(PermissionScope.CREATE_BRANCH,
                                PermissionRule.ofScopes("ci-7", "development", "administration"))
                        .add(PermissionScope.START_JOB,
                                PermissionRule.ofScopes("ci-8", "development", "administration"),
                                PermissionRule.ofScopes("ci-9", "dutywork"))
                        .add(PermissionScope.KILL_JOB,
                                PermissionRule.ofScopes("ci-10"),
                                PermissionRule.ofRoles("ci-10-1", "Development"),
                                PermissionRule.ofDuties("ci-10-2", "22671"))
                        .add(PermissionScope.SKIP_JOB,
                                PermissionRule.of("ci-11",
                                        List.of("development", "administration", "support", "support2"),
                                        List.of("Support", "Support2"),
                                        List.of("2265", "22652")))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci-13"))
                        .add(
                                PermissionScope.TOGGLE_AUTORUN,
                                PermissionRule.of("ci-12",
                                        List.of("development"),
                                        List.of(),
                                        List.of()
                                )
                        )
                        .build());
        var action2 = PermissionsProcessor.finalizePermissions(
                PermissionsProcessor.overridePermissions(ci.getPermissions(),
                        ci.getAction("action-2").getPermissions()));
        assertThat(action2)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of(PermissionForOwner.PR, PermissionForOwner.COMMIT))
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("ci-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("ci-3"))
                        .add(PermissionScope.ROLLBACK_FLOW,
                                PermissionRule.ofScopes("ci-4"), PermissionRule.ofScopes("ci-5"))
                        .add(PermissionScope.ADD_COMMIT,
                                PermissionRule.ofScopes("ci-6", "development"))
                        .add(PermissionScope.CREATE_BRANCH,
                                PermissionRule.ofScopes("ci-7", "development", "administration"))
                        .add(PermissionScope.START_JOB,
                                PermissionRule.ofScopes("ci-8", "development", "administration"),
                                PermissionRule.ofScopes("ci-9", "dutywork"))
                        .add(PermissionScope.KILL_JOB,
                                PermissionRule.ofScopes("ci-10"),
                                PermissionRule.ofRoles("ci-10-1", "Development"),
                                PermissionRule.ofDuties("ci-10-2", "22671"))
                        .add(PermissionScope.SKIP_JOB,
                                PermissionRule.of("ci-11",
                                        List.of("development", "administration", "support", "support2"),
                                        List.of("Support", "Support2"),
                                        List.of("2265", "22652")))
                        .add(
                                PermissionScope.TOGGLE_AUTORUN,
                                PermissionRule.of("ci-12",
                                        List.of("development"),
                                        List.of(),
                                        List.of()
                                )
                        )
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("ci-13"))
                        .build());

        var release1 = PermissionsProcessor.finalizePermissions(
                PermissionsProcessor.overridePermissions(ci.getPermissions(),
                        ci.getRelease("release-1").getPermissions()));
        assertThat(release1)
                .isEqualTo(Permissions.builder()
                        .defaultPermissionsForOwner(List.of(PermissionForOwner.RELEASE))
                        .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("citest-1"))
                        .add(PermissionScope.START_HOTFIX, PermissionRule.ofScopes("citest-2"))
                        .add(PermissionScope.CANCEL_FLOW, PermissionRule.ofScopes("citest-3"))
                        .add(PermissionScope.ROLLBACK_FLOW, PermissionRule.ofScopes("citest-4"))
                        .add(PermissionScope.ADD_COMMIT, PermissionRule.ofScopes("citest-5"))
                        .add(PermissionScope.CREATE_BRANCH, PermissionRule.ofScopes("citest-6"))
                        .add(PermissionScope.START_JOB, PermissionRule.ofScopes("citest-7"))
                        .add(PermissionScope.KILL_JOB, PermissionRule.ofScopes("citest-8"))
                        .add(PermissionScope.SKIP_JOB, PermissionRule.ofScopes("citest-9"))
                        .add(PermissionScope.MANUAL_TRIGGER, PermissionRule.ofScopes("citest-10"))
                        .add(PermissionScope.TOGGLE_AUTORUN, PermissionRule.ofScopes("citest-11"))
                        .build());

        var approvals = PermissionsProcessor.compressRules(ci.getApprovals());
        assertThat(approvals)
                .isEqualTo(List.of(
                        PermissionRule.ofScopes("ci-1"),
                        PermissionRule.ofScopes("ci-2", "s-1")
                ));
    }


    @MethodSource
    static List<Arguments> testOverridePermissions() {
        var ci1 = Permissions.builder()
                .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1"))
                .build();
        var ci2 = Permissions.builder()
                .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-2"))
                .build();

        return List.of(
                Arguments.of(null, null, Permissions.EMPTY),
                Arguments.of(null, ci1, ci1),
                Arguments.of(ci1, null, ci1),
                Arguments.of(ci1, ci2, ci2)
        );
    }

    @MethodSource
    static List<Arguments> testFinalizePermissions() {
        var ci1 = Permissions.builder()
                .add(PermissionScope.START_FLOW, PermissionRule.ofScopes("ci-1"))
                .build();

        var ci2 = Permissions.builder()
                .add(PermissionScope.START_FLOW,
                        PermissionRule.ofScopes("ci-1"),
                        PermissionRule.ofScopes("ci-2"))
                .build();

        return List.of(
                Arguments.of(Permissions.EMPTY, Permissions.EMPTY),
                Arguments.of(ci1, ci1),
                Arguments.of(ci2, ci2),
                Arguments.of(
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1", "scope-2", "scope-2", "scope-1", "scope-3"))
                                .build(),
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1", "scope-2", "scope-1", "scope-3")) // Remove
                                // duplicates
                                .build()
                ),
                Arguments.of(
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1", "scope-2"),
                                        PermissionRule.ofScopes("ci-1", "scope-1"))
                                .build(),
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1", "scope-2", "scope-1")) // Merge scopes with
                                // same slug
                                .build()
                ),
                Arguments.of(
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1", "scope-2"),
                                        PermissionRule.ofScopes("ci-1"),
                                        PermissionRule.ofScopes("ci-2"))
                                .build(),
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-1"), // No scope means - from any scope
                                        PermissionRule.ofScopes("ci-2"))
                                .build()
                ),
                Arguments.of(
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-2", "scope-1"),
                                        PermissionRule.ofScopes("ci-1"),
                                        PermissionRule.ofScopes("ci-1", "scope-2"))
                                .build(),
                        Permissions.builder()
                                .add(PermissionScope.START_FLOW,
                                        PermissionRule.ofScopes("ci-2", "scope-1"),
                                        PermissionRule.ofScopes("ci-1")) // No scope means - from any scope
                                .build()
                )
        );
    }

}
