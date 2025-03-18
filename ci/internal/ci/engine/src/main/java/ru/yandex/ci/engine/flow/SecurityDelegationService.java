package ru.yandex.ci.engine.flow;

import java.nio.file.Path;
import java.time.Instant;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.sandbox.SandboxClient;
import ru.yandex.ci.client.yav.YavClient;
import ru.yandex.ci.client.yav.model.DelegatingTokenResponse;
import ru.yandex.ci.core.config.ConfigSecurityState;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.pr.PullRequestService;

@Slf4j
@AllArgsConstructor
public class SecurityDelegationService {

    @Nonnull
    private final YavClient yavClient;
    @Nonnull
    private final CiMainDb db;
    @Nonnull
    private final SandboxClient sandboxClient;
    @Nonnull
    private final PullRequestService pullRequestService;
    @Nonnull
    private final PermissionsService permissionsService;

    public void delegateYavTokens(
            ConfigBundle configBundle,
            String tvmUserTicket,
            String user
    ) throws YavDelegationException {

        var configEntity = configBundle.getConfigEntity();
        var aYamlConfig = configBundle.getValidAYamlConfig();
        var secret = aYamlConfig.getCi().getSecret();

        if (!"always skip".equals(secret)) { //TODO CI-4022
            log.warn("SKIP APPROVALS CHECK: See https://st.yandex-team.ru/CI-4022#62c706e3eec278776da08161");
        } else {
            permissionsService.checkCiApprovals(user, configEntity.getConfigPath(), aYamlConfig.getService());
        }

        delegateSecretToSandbox(secret, tvmUserTicket);

        var delegatingToken = getDelegatingToken(configEntity.getConfigPath(), secret, tvmUserTicket);
        var yavTokenUid = YavToken.Id.of(delegatingToken.getTokenUuid());

        var token = YavToken.builder()
                .id(yavTokenUid)
                .configPath(configEntity.getId().getConfigPath())
                .secretUuid(secret)
                .token(delegatingToken.getToken())
                .delegationCommitId(configEntity.getRevision().getCommitId())
                .delegatedBy(user)
                .abcService(aYamlConfig.getService())
                .created(Instant.ofEpochMilli(System.currentTimeMillis()))
                .build();

        db.currentOrTx(() -> {
            db.yavTokensTable().save(token);

            log.info("Created delegating token for {}, uid={}", secret, yavTokenUid);

            var securityState = new ConfigSecurityState(yavTokenUid, ConfigSecurityState.ValidationStatus.NEW_TOKEN);
            db.configHistory().updateSecurityState(
                    configEntity.getConfigPath(),
                    configEntity.getRevision(),
                    securityState);
        });

        if (configBundle.getRevision().getBranch().isPr()) {
            var pullRequestId = configBundle.getRevision().getBranch().getPullRequestId();
            pullRequestService.sendTokenDelegated(pullRequestId, user, configBundle.getConfigPath());
        }
    }

    private DelegatingTokenResponse getDelegatingToken(Path configPath, String secretUuid, String tvmUserTicket) {
        return yavClient.createDelegatingToken(
                tvmUserTicket,
                secretUuid,
                toSignature(configPath),
                "CI для запуска конфигурации " + configPath + "; https://nda.ya.ru/t/nZmXMFjT3VxYHZ"
        );
    }

    private static String toSignature(Path configPath) {
        return configPath.toString();
    }

    private void delegateSecretToSandbox(String secretUid, String tvmUserTicket) throws YavDelegationException {
        var result = sandboxClient.delegateYavSecret(secretUid, tvmUserTicket);
        if (!result.isDelegated()) {
            throw new YavDelegationException(
                    String.format(
                            "delegating secret %s failed: %s",
                            result.getSecret(),
                            result.getMessage()
                    )
            );
        }

        log.info("Secret delegated to sandbox {}", result);
    }


}
