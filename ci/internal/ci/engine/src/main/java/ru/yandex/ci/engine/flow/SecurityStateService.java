package ru.yandex.ci.engine.flow;

import java.nio.file.Path;
import java.util.Optional;
import java.util.Set;
import java.util.regex.Pattern;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.config.ConfigStatus;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.security.AccessService;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigParseResult;

@Slf4j
@RequiredArgsConstructor
public class SecurityStateService {

    private static final Pattern TOKEN_PATTERN = Pattern.compile("^sec-[a-z0-9]+$");

    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final AccessService accessService;
    @Nonnull
    private final PermissionsService permissionsService;

    public ConfigSecurityState getSecurityState(
            Path configPath,
            ConfigParseResult newConfigParseResult,
            ArcCommit newConfigCommit,
            @Nullable ConfigBundle previousValidConfig,
            @Nullable ConfigBundle previousPullRequestConfig
    ) {

        if (newConfigParseResult.getStatus() == ConfigParseResult.Status.NOT_CI) {
            log.info("Not a CI");
            return emptyToken(ConfigSecurityState.ValidationStatus.NOT_CI);
        } else if (newConfigParseResult.getStatus() == ConfigParseResult.Status.INVALID) {
            log.info("Configuration is invalid");
            return emptyToken(ConfigSecurityState.ValidationStatus.INVALID_CONFIG);
        }
        Preconditions.checkState(newConfigParseResult.getStatus() == ConfigParseResult.Status.VALID,
                "Configuration status must be valid");

        //Если токен уже существует, например мы заново процессим конфигурацию - берем его.
        var existingToken = db.currentOrReadOnly(() ->
                db.yavTokensTable().findExisting(configPath, newConfigCommit.getRevision()));
        if (existingToken.isPresent()) {
            log.info("Token for {} already exists at revision {}", configPath, newConfigCommit.getRevision());
            return new ConfigSecurityState(
                    existingToken.get().getId(),
                    ConfigSecurityState.ValidationStatus.NEW_TOKEN
            );
        }

        var newConfig = newConfigParseResult.getAYamlConfig();
        Preconditions.checkState(newConfig != null, "Internal error. Config must exists for %s", configPath);

        var securityState = calculateState(configPath, newConfig, newConfigCommit, previousValidConfig);
        log.info("Calculated security state from previous valid config: {}", securityState);

        if (securityState.isValid()) {
            return securityState;
        }

        var reviewState = getStateFromReview(configPath, newConfig, newConfigCommit, previousPullRequestConfig);
        log.info("Calculated review state from previous pull request config: {}", reviewState);

        return (reviewState != null) ? reviewState : securityState;
    }

    @Nullable
    private ConfigSecurityState getStateFromReview(
            Path configPath,
            AYamlConfig newConfig,
            ArcCommit newConfigCommit,
            @Nullable ConfigBundle previousPullRequestConfig
    ) {
        if (previousPullRequestConfig == null) {
            log.info("Previous pull request config is null");
            return null;
        }

        if (previousPullRequestConfig.getStatus() != ConfigStatus.READY) {
            log.info("Previous pull request config is not ready");
            return null;
        }

        if (newConfig.equals(previousPullRequestConfig.getOptionalAYamlConfig().orElse(null))) {
            log.info("Previous pull request config same as current");

            // Если в ревью нет изменений, возвращаем стейт as is.
            // Это позводит прорасти правильному статусу в транк через итерации ревью.
            return previousPullRequestConfig.getConfigEntity().getSecurityState();
        }

        var state = calculateState(configPath, newConfig, newConfigCommit, previousPullRequestConfig);
        log.info("Calculated state for review: {}", state);
        return state.isValid() ? state : null;
    }

    private ConfigSecurityState calculateState(
            Path configPath,
            AYamlConfig newConfig,
            ArcCommit newConfigCommit,
            @Nullable ConfigBundle previousValidConfig
    ) {
        if (previousValidConfig == null || previousValidConfig.getOptionalAYamlConfig().isEmpty()) {
            log.info("No previous valid config");
            return emptyToken(ConfigSecurityState.ValidationStatus.TOKEN_NOT_FOUND);
        }

        Preconditions.checkState(previousValidConfig.getStatus().isValidCiConfig());

        ConfigSecurityState previousSecurityState = previousValidConfig.getConfigEntity().getSecurityState();

        if (!previousSecurityState.getValidationStatus().isValid()) {
            log.info("Previous state is invalid: {}", previousSecurityState.getValidationStatus());
            return emptyToken(ConfigSecurityState.ValidationStatus.TOKEN_NOT_FOUND);
        }

        var previousConfig = previousValidConfig.getValidAYamlConfig();
        if (newConfig.equals(previousConfig)) {
            log.info("Previous state is same as current one");
            return copyToken(previousSecurityState, ConfigSecurityState.ValidationStatus.CONFIG_NOT_CHANGED);
        }

        if (newConfig.getCi() == null || previousConfig.getCi() == null || newConfig.getCi().getSecret() == null) {
            log.info("No secret in any config (old or new)");
            return emptyToken(ConfigSecurityState.ValidationStatus.TOKEN_NOT_FOUND);
        }

        if (!isCorrectToken(newConfig.getCi().getSecret())) {
            log.info("Token looks invalid: {}", newConfig.getCi().getSecret());
            return emptyToken(ConfigSecurityState.ValidationStatus.TOKEN_INVALID);
        }

        if (!newConfig.getCi().getSecret().equals(previousConfig.getCi().getSecret())) {
            log.info("Secret changed: {} -> {}", previousConfig.getCi().getSecret(), newConfig.getCi().getSecret());
            return emptyToken(ConfigSecurityState.ValidationStatus.SECRET_CHANGED);
        }

        if (!newConfig.getService().equals(previousConfig.getService())) {
            log.info("ABC service is changed: {} -> {}", previousConfig.getService(), newConfig.getService());
            return emptyToken(ConfigSecurityState.ValidationStatus.ABC_SERVICE_CHANGED);
        }

        if (accessService.isCiAdmin(newConfigCommit.getAuthor())) {
            log.info("User is CI admin");
            return copyToken(previousSecurityState, ConfigSecurityState.ValidationStatus.USER_IS_ADMIN);
        }

        Preconditions.checkState(previousSecurityState.getYavTokenUuid() != null,
                "Previous security state must be valid at this point, token must be valid");
        var token = db.currentOrReadOnly(() ->
                db.yavTokensTable().get(previousSecurityState.getYavTokenUuid()));
        if (token.getDelegatedBy().equals(newConfigCommit.getAuthor())) {
            log.info("Token is delegated by {}", newConfigCommit.getAuthor());
            return copyToken(previousSecurityState, ConfigSecurityState.ValidationStatus.USER_HAS_TOKEN);
        }

        var soxScope = Optional.ofNullable(previousConfig.getSox())
                .flatMap(config -> Optional.ofNullable(config.getApprovalScope()));

        if (soxScope.isPresent()) {
            boolean hasAccessToService = accessService.hasAccess(
                    newConfigCommit.getAuthor(),
                    newConfig.getService(),
                    Set.of(soxScope.get())
            );
            if (hasAccessToService) {
                log.info("User {} of service {} is a member of scope {}",
                        newConfigCommit.getAuthor(), newConfig.getService(), soxScope.get());
                return copyToken(previousSecurityState, ConfigSecurityState.ValidationStatus.VALID_SOX_USER);
            } else {
                log.info("User {} of service {} is not a member of scope {}",
                        newConfigCommit.getAuthor(), newConfig.getService(), soxScope.get());
                return emptyToken(ConfigSecurityState.ValidationStatus.SOX_NOT_APPROVED);
            }
        }

        if (isUserBelongsToConfig(newConfigCommit, configPath, newConfig)) {
            log.info("User {} is a member of service {}", newConfigCommit.getAuthor(), newConfig.getService());
            return copyToken(previousSecurityState, ConfigSecurityState.ValidationStatus.VALID_USER);
        }

        return emptyToken(ConfigSecurityState.ValidationStatus.INVALID_USER);
    }

    public boolean isUserBelongsToConfig(ArcCommit commit, Path configPath, AYamlConfig config) {
        return permissionsService.hasCiApprovals(commit.getAuthor(), configPath, config.getService());
    }

    private boolean isCorrectToken(String token) {
        return TOKEN_PATTERN.matcher(token).matches();
    }

    private ConfigSecurityState emptyToken(ConfigSecurityState.ValidationStatus status) {
        return new ConfigSecurityState(null, status);
    }

    private ConfigSecurityState copyToken(ConfigSecurityState previousSecurityState,
                                          ConfigSecurityState.ValidationStatus newStatus) {
        return new ConfigSecurityState(previousSecurityState.getYavTokenUuid(), newStatus);
    }

}
