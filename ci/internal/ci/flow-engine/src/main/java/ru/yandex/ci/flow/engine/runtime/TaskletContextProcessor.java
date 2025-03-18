package ru.yandex.ci.flow.engine.runtime;

import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;
import java.util.function.Supplier;
import java.util.stream.Collectors;
import java.util.stream.Stream;
import java.util.stream.StreamSupport;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.core.job.JobResource;
import ru.yandex.ci.core.job.JobResources;
import ru.yandex.ci.core.launch.ReleaseVcsInfo;
import ru.yandex.ci.core.proto.converter.CiContextConverter;
import ru.yandex.ci.core.resolver.DocumentSource;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.model.FlowLaunchContext;
import ru.yandex.ci.flow.utils.UrlService;
import ru.yandex.ci.job.ConfigInfo;
import ru.yandex.ci.job.JobInstanceId;
import ru.yandex.ci.job.ReleaseVscInfo;
import ru.yandex.ci.job.TaskletContext;

@RequiredArgsConstructor
public class TaskletContextProcessor {
    @Nonnull
    private final UrlService urlService;

    public TaskletContext getTaskletContext(FlowLaunchContext launchContext, @Nullable String secretUid) {
        var launchId = launchContext.getLaunchId();
        var processId = launchId.getProcessId();

        TaskletContext.Builder builder = TaskletContext.newBuilder()
                .setJobInstanceId(
                        JobInstanceId.newBuilder()
                                .setFlowLaunchId(launchContext.getFlowLaunchId().asString())
                                .setJobId(launchContext.getJobId())
                                .setNumber(launchContext.getJobLaunchNumber())
                                .build()
                )
                .setTargetRevision(CiContextConverter.toProto(launchContext.getTargetRevision()))
                .setTargetCommit(
                        CiContextConverter.commit(launchContext.getTargetCommit(), launchContext.getTargetRevision())
                )
                .setConfigInfo(
                        ConfigInfo.newBuilder()
                                .setDir(processId.getDir())
                                .setId(processId.getSubId())
                                .setPath(processId.getPath().toString()) // TODO: deprecated
                                .build()
                )
                .setLaunchNumber(launchId.getNumber())
                .setVersion(launchContext.getLaunchInfo().getVersion().asString())
                .setVersionInfo(CiContextConverter.versionToProto(launchContext.getLaunchInfo().getVersion()))
                .setFlowTriggeredBy(Objects.requireNonNullElse(launchContext.getTriggeredBy(), "Undefined"))
                .setBranch(launchContext.getSelectedBranch().asString());

        Optional.ofNullable(secretUid)
                .ifPresent(builder::setSecretUid);

        var flowType = switch (Objects.requireNonNullElse(launchContext.getFlowType(), Common.FlowType.FT_DEFAULT)) {
            case FT_DEFAULT, UNRECOGNIZED -> TaskletContext.FlowType.DEFAULT;
            case FT_HOTFIX -> TaskletContext.FlowType.HOTFIX;
            case FT_ROLLBACK -> TaskletContext.FlowType.ROLLBACK;
        };
        builder.setFlowType(flowType);

        Optional.ofNullable(urlService.toLaunch(launchContext))
                .ifPresent(builder::setCiUrl);

        Optional.ofNullable(urlService.toJobInLaunch(launchContext))
                .ifPresent(builder::setCiJobUrl);

        Optional.ofNullable(launchContext.getRollbackToVersion())
                .map(CiContextConverter::versionToProto)
                .ifPresent(builder::setRollbackToVersion);

        ReleaseVcsInfo vcsReleaseInfo = launchContext.getVcsReleaseInfo();
        if (vcsReleaseInfo != null) {
            ReleaseVscInfo.Builder releaseVscInfo = ReleaseVscInfo.newBuilder();
            Optional.ofNullable(vcsReleaseInfo.getStableRevision())
                    .map(CiContextConverter::toProto)
                    .ifPresent(builder::setPreviousRevision);

            Optional.ofNullable(vcsReleaseInfo.getPreviousRevision())
                    .map(CiContextConverter::toProto)
                    .ifPresent(releaseVscInfo::setPreviousReleaseRevision);

            builder.setReleaseVscInfo(releaseVscInfo);
        } else if (launchContext.getPreviousRevision() != null) {
            builder.setPreviousRevision(
                    CiContextConverter.toProto(launchContext.getPreviousRevision())
            );
        }

        Optional.ofNullable(launchContext.getLaunchPullRequestInfo())
                .map(CiContextConverter::pullRequestInfo)
                .ifPresent(builder::setLaunchPullRequestInfo);

        Optional.ofNullable(launchContext.getTitle())
                .ifPresent(builder::setTitle);

        if (launchContext.getJobTriggeredBy() != null) {
            builder.setJobTriggeredBy(launchContext.getJobTriggeredBy());
        } else {
            Optional.ofNullable(launchContext.getManualTriggeredBy())
                    .ifPresent(builder::setJobTriggeredBy);

            Optional.ofNullable(launchContext.getManualTriggeredAt())
                    .map(ProtoConverter::convert)
                    .ifPresent(builder::setJobTriggeredAt);
        }

        builder.setFlowStartedAt(ProtoConverter.convert(launchContext.getFlowStarted()));
        builder.setJobStartedAt(ProtoConverter.convert(launchContext.getJobStarted()));

        return builder.build();
    }

    public DocumentSource getDocumentSource(JobContext jobContext) {
        var flowLaunchContext = jobContext.createFlowLaunchContext();
        var jobResources = jobContext.resources().getJobResources();
        return getDocumentSource(flowLaunchContext, jobResources);
    }

    public void resolveResources(JobContext jobContext) {
        var documentSource = getDocumentSource(jobContext);
        var resources = jobContext.resources();
        resources.resolveJobResources(resource -> doSubstitute(resource, documentSource));
    }

    public DocumentSource getDocumentSource(FlowLaunchContext launchContext, JobResources jobResources) {
        var taskletContext = getTaskletContext(launchContext, null);
        return getDocumentSource(launchContext, taskletContext, jobResources.getUpstreamResources());
    }

    public DocumentSource getDocumentSource(
            FlowLaunchContext launchContext,
            TaskletContext taskletContext,
            Supplier<Map<String, JsonObject>> upstreamProduction
    ) {
        return DocumentSource.of(
                () -> {
                    var ctx = new JsonObject();
                    ctx.add(PropertiesSubstitutor.CONTEXT_KEY, PropertiesSubstitutor.asJsonObject(taskletContext));
                    if (launchContext.getFlowVars() != null) {
                        ctx.add(PropertiesSubstitutor.FLOW_VARS_KEY, launchContext.getFlowVars());
                    }
                    return ctx;
                },
                () -> PropertiesSubstitutor.asTasks(upstreamProduction.get())
        );
    }

    public List<JobResource> doSubstitute(List<JobResource> resources, DocumentSource documentSource) {
        return resources.stream()
                .flatMap(resource -> doSubstitute(resource, documentSource))
                .collect(Collectors.toList());
    }

    private static Stream<JobResource> doSubstitute(JobResource resource, DocumentSource documentSource) {
        var unwrap = unwrapObject(PropertiesSubstitutor.substitute(resource.getData(), documentSource));
        if (unwrap.isJsonArray()) {
            return StreamSupport.stream(unwrap.getAsJsonArray().spliterator(), false)
                    .map(element -> resource
                            .withData(element.getAsJsonObject())
                            .withParentField(null)); // Ресурсы будут объединены по типу, а не по полю
        } else {
            return Stream.of(resource.withData(unwrap.getAsJsonObject()));
        }
    }

    private static JsonElement unwrapObject(JsonElement value) {
        return SchemaService.unwrapObject(value.getAsJsonObject());
    }
}
