package ru.yandex.ci.engine.pr;

import java.nio.file.Path;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;
import org.apache.commons.text.StringSubstitutor;

import ru.yandex.ci.client.arcanum.ArcanumClient.Require;
import ru.yandex.ci.client.arcanum.ArcanumClientImpl;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementDto;
import ru.yandex.ci.client.arcanum.ArcanumMergeRequirementId;
import ru.yandex.ci.client.arcanum.ArcanumReviewDataDto;
import ru.yandex.ci.client.arcanum.UpdateCheckRequirementRequestDto;
import ru.yandex.ci.client.arcanum.UpdateCheckStatusRequest;
import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.ConfigEntity;
import ru.yandex.ci.core.config.ConfigProblem;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.PermissionRule;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchPullRequestInfo;
import ru.yandex.ci.core.pr.ArcanumCheckType;
import ru.yandex.ci.core.pr.MergeRequirementsCollector;
import ru.yandex.ci.core.pr.PullRequestDiffSet;
import ru.yandex.ci.core.security.PermissionsService;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.flow.utils.UrlService;

import static ru.yandex.ci.client.arcanum.ArcanumClient.Require.required;

@Slf4j
@RequiredArgsConstructor
public class PullRequestService {
    @Nonnull
    private final ArcanumClientImpl arcanumClient;
    @Nonnull
    private final String mergeRequirementSystem;
    @Nonnull
    private final UrlService urlService;
    @Nonnull
    private final PullRequestCommentService pullRequestCommentService;
    @Nonnull
    private final PermissionsService permissionsService;

    private final MergeRequirementIdFactory mergeRequirementFactory = new MergeRequirementIdFactory();

    public void sendAYmlNotYamlWasChangedComment(long pullRequestId, Collection<Path> paths) {
        Preconditions.checkState(!paths.isEmpty());
        boolean plural = paths.size() > 1;

        var fileList = paths.stream()
                .map(Path::toString)
                .map(s -> "***" + s + "***")
                .collect(Collectors.joining(", "));
        String rusMessage = StringSubstitutor.replace(
                """
                        Предупреждение от CI
                        ==
                        Обратите внимание, что в пулл-реквесте ${filesPre} ${fileList}.
                        ${extPre} **.yml**, однако только файлы **a.yaml** являются репозиторными конфигами Аркадии.
                        Возможно, имя было выбрано ошибочно. Переименуйте в **a.yaml**,\
                         в противном случае файл не будет обработан CI.""",
                Map.of(
                        "filesPre", plural ? "были затронуты файлы:" : "был затронут файл",
                        "fileList", fileList,
                        "extPre", plural ? "Они имеют расширение" : "Он имеет расширение"
                ));
        String engMessage = StringSubstitutor.replace(
                """
                        CI Warning
                        ==
                        Please note that following ${filesPre} ${fileList}.
                        Files **a.yml** are not Arcadia repository configs, only **a.yaml** are.
                        Probably name **a.yml** was selected by mistake, change it to **a.yaml**.
                        CI doesn't process **a.yml** files.""",
                Map.of(
                        "filesPre", plural ? "files were affected in PR:" : "file was affected in PR:",
                        "fileList", fileList
                ));

        pullRequestCommentService.scheduleCreatePrComment(pullRequestId, joinMessages(rusMessage, engMessage));
    }

    public void sendSecurityProblemComment(long pullRequestId, ConfigBundle configBundle) {
        var configEntity = configBundle.getConfigEntity();

        var config = configBundle.getValidAYamlConfig();
        var secretKey = config.getCi().getSecret();
        if (StringUtils.isEmpty(secretKey)) { // This cannot be
            return; // ---
        }

        var rules = permissionsService.getCiApprovalRules(configBundle.getConfigPath(), config.getService());
        var rulesList = rules.isEmpty()
                ? List.of(PermissionRule.ofScopes(config.getService()))
                : rules;

        var rulesString = rulesList.stream()
                .map(rule -> " * " + rule + "\n")
                .collect(Collectors.joining());

        var params = Map.of(
                "path", configEntity.getConfigPath(),
                "secretKey", secretKey,
                "secretUrl", urlService.toYavSecretKey(secretKey),
                "delegationUrl", urlService.toDelegationUrl(configEntity.getConfigPath(), pullRequestId),
                "securityDocsUrl", urlService.getCiSecretDocsUrl(),
                "validationMessage", configEntity.getSecurityState().getValidationStatus().getMessage(),
                "rules", rulesString
        );

        var rusMessage = StringSubstitutor.replace("""
                Сообщение от Arcadia CI
                ==
                Конфигурация **${path}** изменилась и требует подтверждения с делегацией токена.
                Для делегации необходимо иметь доступ к секрету [${secretKey}](${secretUrl}).

                Подтверждение является обязательным, если аккаунт не соответствует требованиям:
                ${rules}
                Проверки Arcadia CI будут запущены после делегации.
                * Для делегации и запуска проверок перейдите по [ссылке](${delegationUrl}).
                * [Зачем CI требует этот токен](${securityDocsUrl})

                Оригинальное сообщение: `${validationMessage}`""", params);
        var engMessage = StringSubstitutor.replace("""
                Arcadia CI Note
                ==
                Configuration **${path}** is changed and must be approved with token delegation.
                One should have access to secret [${secretKey}](${secretUrl}).

                Approval is required unless account meets requirements:
                ${rules}
                Arcadia CI checks will be launched after delegation.
                * To delegate token and start checks open [link](${delegationUrl}).
                * [Why CI needs this token](${securityDocsUrl})

                Original message: `${validationMessage}`""", params);

        pullRequestCommentService.scheduleCreatePrComment(pullRequestId, joinMessages(rusMessage, engMessage));
    }

    public void sendConfigValidationResult(
            long pullRequestId,
            long diffSetId,
            ConfigEntity configEntity,
            boolean required,
            @Nullable String checkUri,
            MergeRequirementsCollector mergeRequirementsCollector) {
        ArcanumMergeRequirementId requirementId = mergeRequirementFactory.configValidation(
                configEntity.getConfigPath()
        );

        if (required) {
            arcanumClient.setMergeRequirement(pullRequestId, requirementId, required());
            mergeRequirementsCollector.add(requirementId);
        }

        var isConfigValid = configEntity.getStatus().isValidCiConfig();

        var status = isConfigValid
                ? ArcanumMergeRequirementDto.Status.SUCCESS
                : ArcanumMergeRequirementDto.Status.FAILURE;

        var description = isConfigValid ? null : "See Comments";

        arcanumClient.setMergeRequirementStatus(pullRequestId, diffSetId, requirementId, status, checkUri, description);

        if (!isConfigValid) {
            sendInvalidConfigComment(pullRequestId, configEntity);
        }
    }

    public void sendAutocheckStartupFailure(long pullRequestId, String autocheckFailureReason) {
        var msg = """
                Autocheck Error
                ==
                Unable to start autocheck for this PR: %s
                """.formatted(autocheckFailureReason);
        pullRequestCommentService.scheduleCreatePrComment(pullRequestId, msg);
    }

    /**
     * Thread this config as 'bypass' (basically send SUCCESS)
     *
     * @param pullRequestId PR id
     * @param diffSetId     set id
     * @param configEntity  configuration (file name)
     */
    public void sendConfigBypassResult(long pullRequestId, long diffSetId, ConfigEntity configEntity) {
        var requirementId = mergeRequirementFactory.configValidation(configEntity.getConfigPath());
        var status = ArcanumMergeRequirementDto.Status.SUCCESS;
        arcanumClient.setMergeRequirementStatus(pullRequestId, diffSetId, requirementId, status);
    }

    public void sendTokenDelegated(long pullRequestId, String user, Path configPath) {
        var message = "Token for configuration **%s** successfully delegated by @%s".formatted(configPath, user);
        pullRequestCommentService.scheduleCreatePrComment(pullRequestId, message);
    }

    public void sendRequirementAndMergeStatus(
            LaunchPullRequestInfo pullRequestInfo,
            ArcanumMergeRequirementDto.Status status,
            @Nullable String projectId,
            @Nullable LaunchId launchId,
            Require require,
            MergeRequirementsCollector mergeRequirementsCollector,
            @Nullable String description
    ) {
        Preconditions.checkState(pullRequestInfo.getRequirementId() != null, "Requirement id cannot be null");

        arcanumClient.setMergeRequirement(
                pullRequestInfo.getPullRequestId(),
                pullRequestInfo.getRequirementId(),
                require);
        sendMergeRequirementStatus(
                pullRequestInfo,
                pullRequestInfo.getRequirementId(),
                status,
                projectId,
                launchId,
                description
        );
        if (require.isRequired()) {
            mergeRequirementsCollector.add(pullRequestInfo.getRequirementId());
        }
    }

    public void sendMergeRequirementStatus(
            LaunchPullRequestInfo pullRequestInfo,
            ArcanumMergeRequirementId requirementId,
            ArcanumMergeRequirementDto.Status status,
            @Nullable String projectId,
            @Nullable LaunchId launchId,
            @Nullable String description
    ) {
        arcanumClient.setMergeRequirementStatus(
                pullRequestInfo.getPullRequestId(),
                pullRequestInfo.getDiffSetId(),
                requirementId,
                status,
                (projectId == null || launchId == null)
                        ? null
                        : urlService.toFlowLaunch(projectId, launchId),
                description
        );
    }

    public void sendMergeRequirementStatus(
            long reviewRequestId,
            long diffSetId,
            UpdateCheckStatusRequest request) {
        arcanumClient.setMergeRequirementStatus(reviewRequestId, diffSetId, request);
    }

    public void sendProcessedByCiMergeRequirementStatus(long pullRequestId, long diffSetId) {
        arcanumClient.setMergeRequirementStatus(pullRequestId, diffSetId, mergeRequirementFactory.processedByCi(),
                ArcanumMergeRequirementDto.Status.SUCCESS);
    }

    public String getPullRequestTitle(CiProcessId processId, CiConfig ciConfig) {
        var actionConfig = ciConfig.getAction(processId.getSubId());
        if (actionConfig.getTitle() != null) {
            return actionConfig.getTitle();
        }

        var flowConfig = ciConfig.getFlow(actionConfig.getFlow());
        return flowConfig.getTitle();
    }

    public LaunchPullRequestInfo toLaunchPullRequestInfo(
            PullRequestDiffSet diffSet,
            CiProcessId ciProcessId,
            String title) {
        ArcanumMergeRequirementId mergeRequirementId = mergeRequirementFactory.flowRequirement(ciProcessId, title);
        return new LaunchPullRequestInfo(
                diffSet.getPullRequestId(),
                diffSet.getDiffSetId(),
                diffSet.getAuthor(),
                diffSet.getSummary(),
                diffSet.getDescription(),
                mergeRequirementId,
                diffSet.getVcsInfo(),
                diffSet.getIssues(),
                diffSet.getLabels(),
                diffSet.getEventCreated()
        );
    }

    public String generateUrlForArcPathAtTrunkHead(Path path) {
        return arcanumClient.generateUrlForArcPathAtTrunkHead(path);
    }

    public String getMergeRequirementSystem() {
        return mergeRequirementSystem;
    }

    public Optional<ArcanumReviewDataDto> getActiveDiffSetAndMergeRequirements(long pullRequestId) {
        return arcanumClient.getReviewRequestData(pullRequestId, "active_diff_set(id)," +
                "checks(system,type,required,satisfied,status,disabling_policy)");
    }

    /**
     * @throws HttpException with code 400, when diffSetId != activeDiffSetId of pull request
     */
    public void setMergeRequirementsWithCheckingActiveDiffSet(
            long reviewRequestId,
            long diffSetId,
            Map<ArcanumMergeRequirementId, Boolean> requirements,
            @Nullable String reason
    ) {
        var requirementDtos = requirements.entrySet()
                .stream()
                .map(entry -> new UpdateCheckRequirementRequestDto(
                        entry.getKey().getSystem(),
                        entry.getKey().getType(),
                        entry.getValue(),
                        new UpdateCheckRequirementRequestDto.ReasonDto(reason)
                ))
                .collect(Collectors.toList());

        arcanumClient.setMergeRequirementsWithCheckingActiveDiffSet(reviewRequestId, diffSetId, requirementDtos);
    }

    public void skipArcanumDefaultChecks(PullRequestDiffSet diffSet) {
        skipArcanumDefaultChecks(diffSet.getPullRequestId(), diffSet.getDiffSetId());
    }

    public void skipArcanumDefaultChecks(long pullRequestId, long diffSetId) {
        for (var check : ArcanumCheckType.values()) {
            var arcanumRequest = UpdateCheckStatusRequest.builder()
                    .system(check.getSystem())
                    .type(check.getType())
                    .status(ArcanumMergeRequirementDto.Status.SKIPPED)
                    // send empty string to prevent `HttpException: "system_check_id is empty"`
                    .systemCheckId("")
                    .build();
            log.info("Changing check status for pullRequest {}, diffSet {}: {}",
                    pullRequestId, diffSetId, arcanumRequest);
            sendMergeRequirementStatus(pullRequestId, diffSetId, arcanumRequest);
        }
    }

    private String joinMessages(String rusMessage, String engMessage) {
        return rusMessage + "\n\n---\n" + engMessage;
    }

    private void sendInvalidConfigComment(long pullRequestId, ConfigEntity configEntity) {
        var msg = """
                CI Error
                ==
                Found invalid configuration: **[%s](%s)**

                **List of issues:**
                """.formatted(configEntity.getConfigPath(),
                urlService.toArcFile(configEntity.getRevision(), configEntity.getConfigPath()));
        StringBuilder commentBuilder = new StringBuilder();
        commentBuilder.append(msg);
        for (ConfigProblem problem : configEntity.getProblems()) {
            commentBuilder.append("* ").append(problem.getTitle());
            if (!Strings.isNullOrEmpty(problem.getDescription())) {
                commentBuilder.append(":\n ```\n").append(problem.getDescription()).append("\n ```");
            }
            commentBuilder.append("\n");
        }

        pullRequestCommentService.scheduleCreatePrComment(pullRequestId, commentBuilder.toString());
    }

    private class MergeRequirementIdFactory {

        private static final String SYSTEM_ARCANUM = "arcanum";

        ArcanumMergeRequirementId flowRequirement(CiProcessId ciProcessId, String title) {
            var type = ciProcessId.getDir().isEmpty()
                    ? title
                    : (ciProcessId.getDir() + ": " + title);
            return ArcanumMergeRequirementId.of(mergeRequirementSystem, type);
        }

        ArcanumMergeRequirementId configValidation(Path configPath) {
            return ArcanumMergeRequirementId.of(mergeRequirementSystem, "[validation] " + configPath);
        }

        ArcanumMergeRequirementId processedByCi() {
            return ArcanumMergeRequirementId.of(SYSTEM_ARCANUM, "processed_by_ci");
        }
    }
}
