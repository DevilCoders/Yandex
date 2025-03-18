package ru.yandex.ci.core.security;

import java.util.List;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;

import static org.assertj.core.api.Assertions.assertThat;

class PermissionsServiceUnitTest {

    @Test
    void testResolvePermissionsDirect() {
        var permissionsBuilder = Permissions.builder();
        for (var scope : PermissionScope.values()) {
            permissionsBuilder.add(scope, rule(scope));
        }
        var permissions = permissionsBuilder.build();

        for (var scope : PermissionScope.values()) {
            assertThat(PermissionsService.resolvePermissions(permissions, scope))
                    .isEqualTo(List.of(rule(scope)));
        }
    }

    @Test
    void testResolvePermissionsHierarchyStartFlow() {
        var ruleModify = rule(PermissionScope.MODIFY);
        var ruleStartFlow = rule(PermissionScope.START_FLOW);

        var permissions = Permissions.builder()
                .add(PermissionScope.MODIFY, ruleModify)
                .add(PermissionScope.START_FLOW, ruleStartFlow)
                .build();

        for (var scope : PermissionScope.values()) {
            var expect = scope == PermissionScope.MODIFY
                    ? ruleModify
                    : ruleStartFlow;
            assertThat(PermissionsService.resolvePermissions(permissions, scope))
                    .isEqualTo(List.of(expect));
        }
    }

    @Test
    void testResolvePermissionsHierarchyModify() {
        var ruleModify = rule(PermissionScope.MODIFY);

        var permissions = Permissions.builder()
                .add(PermissionScope.MODIFY, ruleModify)
                .build();

        for (var scope : PermissionScope.values()) {
            assertThat(PermissionsService.resolvePermissions(permissions, scope))
                    .isEqualTo(List.of(ruleModify));
        }
    }

    private static PermissionRule rule(PermissionScope scope) {
        return PermissionRule.ofScopes(scope.toString());
    }

}
