package ru.yandex.ci.core.security;

import java.util.List;
import java.util.Optional;

import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.Permissions;

interface PermissionSource {
    String getProject();

    List<PermissionRule> getApprovals();

    Optional<Permissions> findRelease(String releaseId);

    Optional<Permissions> findAction(String actionId);

    Optional<ConfigPermissions.FlowPermissions> findFlow(String flowId);

    static PermissionSource fromPermissions(ConfigPermissions permissions) {
        return new PermissionSource() {

            @Override
            public String getProject() {
                return permissions.getProject();
            }

            @Override
            public List<PermissionRule> getApprovals() {
                return permissions.getApprovals();
            }

            @Override
            public Optional<Permissions> findRelease(String releaseId) {
                return Optional.ofNullable(permissions.getReleases().get(releaseId));
            }

            @Override
            public Optional<Permissions> findAction(String actionId) {
                return Optional.ofNullable(permissions.getActions().get(actionId));
            }

            @Override
            public Optional<ConfigPermissions.FlowPermissions> findFlow(String flowId) {
                return Optional.ofNullable(permissions.getFlows().get(flowId));
            }
        };
    }

}
