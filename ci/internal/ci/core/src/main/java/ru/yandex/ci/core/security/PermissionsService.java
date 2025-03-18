package ru.yandex.ci.core.security;


import java.nio.file.Path;
import java.util.List;
import java.util.Objects;
import java.util.Set;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.grpc.GrpcUtils;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigPermissions;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.VirtualCiProcessId;
import ru.yandex.ci.core.config.a.model.PermissionForOwner;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.config.a.model.PermissionScope;
import ru.yandex.ci.core.config.a.model.Permissions;
import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
@RequiredArgsConstructor
public class PermissionsService {

    @Nonnull
    private final AccessService accessService;

    @Nonnull
    private final CiMainDb db;

    // Support scopes without revision
    public void checkAccess(
            String login,
            CiProcessId ciProcessId,
            PermissionScope scope
    ) {
        Preconditions.checkState(scope.isCheckWithoutRevision(),
                "Permission scope %s cannot be used without config revision", scope);

        var config = getPermissionSource(login, ciProcessId, null, "scope = [%s]".formatted(scope));
        checkAccess(login, null, ciProcessId, config, scope);
    }


    // Support scopes with revision
    public void checkAccess(
            String login,
            CiProcessId ciProcessId,
            OrderedArcRevision configRevision,
            PermissionScope scope
    ) {
        checkAccess(login, null, ciProcessId, configRevision, scope);
    }

    // Check with owner config
    public void checkAccess(
            String login,
            @Nullable OwnerConfig ownerConfig,
            CiProcessId ciProcessId,
            OrderedArcRevision configRevision,
            PermissionScope scope
    ) {
        Preconditions.checkState(scope.isCheckWithRevision(),
                "Permission scope %s cannot be used with config revision", scope);
        Preconditions.checkState(configRevision != null,
                "ConfigRevision cannot be null when checking permission scope %s", scope);

        var config = getPermissionSource(login, ciProcessId, configRevision, "scope = [%s]".formatted(scope));
        checkAccess(login, ownerConfig, ciProcessId, config, scope);
    }

    // Check for job approvers
    public void checkJobApprovers(
            String login,
            CiProcessId ciProcessId,
            OrderedArcRevision configRevision,
            String flowId,
            String jobId,
            String message
    ) {
        var src = getPermissionSource(login, ciProcessId, configRevision,
                "flow = [%s], job = [%s], %s".formatted(flowId, jobId, message));
        var flow = src.findFlow(flowId);
        if (flow.isEmpty()) {
            log.info("No approval configuration for flow [{}]", flowId);
            return; // ---
        }
        var rules = flow.get().getJobApprovers().getOrDefault(jobId, List.of());
        if (rules.isEmpty()) {
            log.info("No approval configuration for flow [{}], job [{}]", flowId, jobId);
            return; // ---
        }

        log.info("Resolved rules: {}", rules);
        checkAccess(login, rules, message);
    }

    // Yes, it is possible we won't find any approved configuration at this point
    public boolean hasCiApprovals(String login, Path path, String service) {
        var rules = getCiApprovalRules(path, service);
        if (rules.isEmpty()) {
            return hasAccess(login, service);
        } else {
            return hasAccess(login, rules);
        }
    }

    // Yes, it is possible we won't find any approved configuration at this point
    public void checkCiApprovals(String login, Path path, String service) {
        log.info("Checking approval for user = [{}], path = [{}]", login, path);

        var rules = getCiApprovalRules(path, service);
        log.info("Resolved rules: {}", rules);

        var message = "approvals";
        if (rules.isEmpty()) {
            checkAccess(login, service, message);
        } else {
            checkAccess(login, rules, message);
        }
    }

    public List<PermissionRule> getCiApprovalRules(Path path, String service) {
        return getPermissionSource(path, service).getApprovals();
    }

    private PermissionSource getPermissionSource(
            String login,
            CiProcessId ciProcessId,
            @Nullable OrderedArcRevision configRevision,
            String description
    ) {
        var virtualType = VirtualCiProcessId.of(ciProcessId);
        var path = virtualType.getResolvedPath();
        log.info("Checking permissions for user = [{}], processId = [{}], path = [{}], revision = [{}], {}",
                login, ciProcessId, path, configRevision, description);
        return getPermissionSource(path, configRevision);
    }

    private PermissionSource getPermissionSource(
            Path path,
            @Nullable OrderedArcRevision configRevision
    ) {
        return db.currentOrReadOnly(() -> {
            var config = configRevision != null
                    ? loadConfig(path, configRevision)
                    : loadConfig(path);
            var permissions = config.getPermissions();
            if (permissions == null) { // That's unfortunate
                log.warn("No permissions in config [{}] on [{}]", path, configRevision);
                var configState = db.configStates().get(path);
                if (configState.getProject() == null) {
                    throw GrpcUtils.internalError("Cannot check permission scope for config state " + path +
                            ", project is unknown");
                }
                permissions = ConfigPermissions.of(configState.getProject());
            }
            return PermissionSource.fromPermissions(permissions);
        });
    }

    private PermissionSource getPermissionSource(
            Path path,
            String service
    ) {
        return db.currentOrReadOnly(() -> {
            var config = db.configHistory().findLastReadyConfig(path, ArcBranch.trunk());

            ConfigPermissions permissions;
            if (config.isEmpty() || config.get().getPermissions() == null) {
                permissions = ConfigPermissions.of(service);
            } else {
                permissions = config.get().getPermissions();
            }
            return PermissionSource.fromPermissions(permissions);
        });
    }


    private ConfigEntity loadConfig(Path path, OrderedArcRevision configRevision) {
        var config = db.configHistory().getById(path, configRevision);
        if (config.getStatus() != ConfigStatus.READY) {
            throw GrpcUtils.internalError("Cannot check permission scope for config " + path +
                    " on revision " + configRevision + " with status " + config.getStatus());
        }
        return config;
    }

    private ConfigEntity loadConfig(Path path) {
        var configOpt = db.configHistory().findLastReadyConfig(path, ArcBranch.trunk());
        if (configOpt.isEmpty()) {
            throw GrpcUtils.internalError("Cannot check permission scope for config " + path +
                    ", no config with status " + ConfigStatus.READY + " found in " + ArcBranch.trunk());
        }
        return configOpt.get();
    }

    private void checkAccess(
            String login,
            @Nullable OwnerConfig ownerConfig,
            CiProcessId ciProcessId,
            PermissionSource src,
            PermissionScope scope
    ) {
        var project = src.getProject();
        log.info("Check for project = [{}]", project);
        if (project == null) {
            throw GrpcUtils.internalError("No project is configured for " + ciProcessId);
        }

        var permissions = switch (ciProcessId.getType()) {
            case FLOW -> src.findAction(ciProcessId.getSubId()).orElse(null);
            case RELEASE -> src.findRelease(ciProcessId.getSubId()).orElse(null);
            case SYSTEM -> throw new IllegalStateException("Unsupported type: " + ciProcessId);
        };

        log.info("Configured permissions: {}", permissions);

        if (hasOwnerPermissions(login, ownerConfig, ciProcessId, permissions)) {
            log.info("Owner {} has full access to scope {} in PR", ownerConfig.getOwner(), scope);
            return;
        }

        var message = "scope [" + scope + "]";
        if (permissions == null) { // No specific configuration
            checkAccess(login, project, message);
            return;
        }

        var rules = resolvePermissions(permissions, scope);
        log.info("Resolved rules: {}", rules);

        if (rules.isEmpty()) {
            checkAccess(login, project, message);
        } else {
            checkAccess(login, rules, message);
        }
    }

    private boolean hasAccess(String login, String project) {
        return accessService.hasAccessOrAdmin(login, project);
    }

    private void checkAccess(String login, String project, String message) {
        if (!hasAccess(login, project)) {
            throw GrpcUtils.permissionDeniedException("User [%s] has no access to %s, not in project [%s]"
                    .formatted(login, message, project));
        }
    }

    private boolean hasAccess(String login, List<PermissionRule> rules) {
        for (var rule : rules) {
            if (accessService.hasAccessOrAdmin(login, rule.getAbcService(),
                    Set.copyOf(rule.getAbcScopes()),
                    Set.copyOf(rule.getAbcRoles()),
                    Set.copyOf(rule.getAbcDuties()))) {
                return true;
            }
        }
        return false;
    }

    private void checkAccess(String login, List<PermissionRule> rules, String message) {
        if (!hasAccess(login, rules)) {
            throw GrpcUtils.permissionDeniedException("User [%s] has no access to %s within rules %s"
                    .formatted(login, message, rules));
        }
    }

    private static boolean hasOwnerPermissions(
            String login,
            @Nullable OwnerConfig ownerConfig,
            CiProcessId ciProcessId,
            Permissions permissions
    ) {
        if (ownerConfig == null || !Objects.equals(login, ownerConfig.getOwner())) {
            return false;
        }

        var permissionsForOwner = Objects.requireNonNullElse(
                permissions != null ? permissions.getDefaultPermissionsForOwner() : null,
                Permissions.DEFAULT_PERMISSIONS_FOR_OWNER
        );

        // TODO: fix later
        if ("devtools/migration".equals(ciProcessId.getDir()) &&
                "run-migration".equals(ciProcessId.getSubId())) {
            return true;
        }

        if (ciProcessId.getType().isRelease()) {
            return permissionsForOwner.contains(PermissionForOwner.RELEASE);
        } else {
            if (ownerConfig.getRevision().getBranch().isPr()) {
                return permissionsForOwner.contains(PermissionForOwner.PR);
            } else {
                return permissionsForOwner.contains(PermissionForOwner.COMMIT);
            }
        }
    }

    static List<PermissionRule> resolvePermissions(Permissions permissions, PermissionScope scope) {
        var rules = permissions.getPermissions(scope);

        // Permissions has hierarchy with default values: MODIFY -> START_FLOW -> everything else
        // If no permission provided -> check scope START_FLOW
        // If no permissions for START_FLOW -> check scope MODIFY
        if (rules.isEmpty() && scope != PermissionScope.MODIFY) {
            if (scope != PermissionScope.START_FLOW) {
                rules = permissions.getPermissions(PermissionScope.START_FLOW);
            }
            if (rules.isEmpty()) {
                rules = permissions.getPermissions(PermissionScope.MODIFY);
            }
        }

        return rules;
    }
}
