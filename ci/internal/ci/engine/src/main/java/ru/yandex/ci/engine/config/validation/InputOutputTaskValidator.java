package ru.yandex.ci.engine.config.validation;

import java.nio.file.Path;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.Multimap;
import com.google.gson.JsonObject;
import com.google.gson.JsonParser;
import lombok.Builder;
import lombok.RequiredArgsConstructor;
import lombok.Singular;
import lombok.Value;
import lombok.With;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.api.proto.Common;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.ActionConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.a.validation.ValidationReport;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.job.TaskUnrecoverableException;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.FlowRunConfig;
import ru.yandex.ci.core.launch.FlowVarsService;
import ru.yandex.ci.core.schema.FakeDataFactory;
import ru.yandex.ci.core.schema.JsonSchemaFaker;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.engine.config.validation.ValidationResourceProvider.ResourceParentJobId;
import ru.yandex.ci.engine.launch.FlowCustomizedConfig;
import ru.yandex.ci.engine.launch.FlowFactory;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.job.Job;
import ru.yandex.ci.flow.engine.runtime.ExecutorType;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.engine.source_code.model.ConsumedResource;
import ru.yandex.lang.NonNullApi;

@RequiredArgsConstructor
public class InputOutputTaskValidator {
    private static final Logger log = LoggerFactory.getLogger(InputOutputTaskValidator.class);
    private static final JobResourceType CI_TASKLET_CONTEXT = JobResourceType.of("ci.TaskletContext");

    private final FlowFactory flowFactory;
    private final SourceCodeService sourceCodeService;
    private final ValidationResourceProvider resourceProvider;
    private final TaskletMetadataService taskletMetadataService;
    @SuppressWarnings("UnusedVariable")
    private final FlowVarsService flowVarsService;

    private final JsonSchemaFaker jsonSchemaFaker = new JsonSchemaFaker(new FakeDataFactory());

    public Report validate(Path configPath, AYamlConfig aYamlConfig, Map<TaskId, TaskConfig> taskConfigs) {
        var violationsByFlows = new HashMap<ReportId, FlowViolations>();
        var checkedFlowIds = new HashSet<String>();

        // All this checks are made to validate flows with flow-vars
        validateFlows(
                aYamlConfig.getCi().getReleases().values().stream()
                        .map(ReleaseConfig::getId)
                        .collect(Collectors.toList()),
                CiProcessId.Type.RELEASE,
                null,
                configPath,
                aYamlConfig,
                taskConfigs,
                violationsByFlows,
                checkedFlowIds
        );

        validateFlows(
                aYamlConfig.getCi().getActions().values().stream()
                        .map(ActionConfig::getId)
                        .collect(Collectors.toList()),
                CiProcessId.Type.FLOW,
                "Action",
                configPath,
                aYamlConfig,
                taskConfigs,
                violationsByFlows,
                checkedFlowIds
        );

        // Check only flows without direct reference from actions or releases
        validateFlows(
                aYamlConfig.getCi().getFlows().values().stream()
                        .map(FlowConfig::getId)
                        .filter(flowId -> !checkedFlowIds.contains(flowId))
                        .collect(Collectors.toList()),
                CiProcessId.Type.FLOW,
                null,
                configPath,
                aYamlConfig,
                taskConfigs,
                violationsByFlows,
                checkedFlowIds
        );

        return new Report(violationsByFlows, getDeprecatedTasks(aYamlConfig, taskConfigs));
    }

    private LinkedHashMap<String, String> getDeprecatedTasks(AYamlConfig aYamlConfig,
                                                             Map<TaskId, TaskConfig> taskConfigs) {
        return aYamlConfig.getCi().getFlows().values().stream()
                .flatMap(flow -> Stream.of(flow.getJobs(), flow.getCleanupJobs()))
                .map(LinkedHashMap::values)
                .flatMap(Collection::stream)
                .map(JobConfig::getTaskId)
                .filter(TaskId::requireTaskConfig)
                .map(taskId -> {
                    var config = taskConfigs.get(taskId);
                    if (config == null) {
                        return null;
                    }
                    var deprecated = config.getDeprecated();
                    if (deprecated == null) {
                        return null;
                    }

                    return Map.entry(taskId.getId(), config.getDeprecated().trim());
                })
                .filter(Objects::nonNull)
                .collect(Collectors.toMap(Map.Entry::getKey, Map.Entry::getValue, (v1, v2) -> v1, LinkedHashMap::new));
    }

    private void validateFlows(
            List<String> ids,
            CiProcessId.Type type,
            @Nullable String subType,
            Path configPath,
            AYamlConfig aYamlConfig,
            Map<TaskId, TaskConfig> taskConfigs,
            Map<ReportId, FlowViolations> violationsByFlows,
            Set<String> checkedFlowIds
    ) {
        BiConsumer<CiProcessId, ReportId> validateImpl = (processId, reportId) -> {

            var runConfig = FlowRunConfig.lookup(processId, aYamlConfig.getCi(), reportId.getFlowReference());
            var fakeUi = createFakeValuesForRequiredFields(runConfig);

            JsonObject flowVars = flowVarsService.prepareFlowVarsFromUi(
                    fakeUi,
                    runConfig.getFlowVars(),
                    runConfig.getFlowVarsUi(),
                    false
            );

            var customConfig = FlowCustomizedConfig.builder()
                    .flowReference(reportId.getFlowReference())
                    .flowIdListener(checkedFlowIds::add)
                    .flowVars(flowVars)
                    .build();

            Flow flow;
            try {
                flow = flowFactory.create(processId, aYamlConfig, taskConfigs, customConfig);

                FlowViolations flowViolations = validateFlow(flow, reportId);
                if (flowViolations.hasViolations()) {
                    Preconditions.checkState(
                            !violationsByFlows.containsKey(reportId),
                            "%s has been already matched",
                            reportId
                    );

                    violationsByFlows.put(reportId, flowViolations);
                }
            } catch (TaskUnrecoverableException e) {
                log.info("config validation failed ({}}, configPath {})",
                        reportId, configPath, e);

                var flowViolationsBuilder = FlowViolations.builder();
                flowViolationsBuilder.otherViolation(e.getMessage());
                violationsByFlows.put(reportId, flowViolationsBuilder.build());

                // Do not interrupt cycle - collect as many validation errors as we can
            } catch (AYamlValidationException e) {
                log.info("config validation failed ({}, configPath {})",
                        reportId, configPath, e);

                var flowViolationsBuilder = FlowViolations.builder();
                ValidationReport report = e.getValidationReport();

                flowViolationsBuilder.violationsByJobs(report.getFlowReport().getJobErrors());

                var otherViolations = new HashSet<>(report.getSchemaReportMessages());
                otherViolations.addAll(report.getStaticErrors());
                flowViolationsBuilder.otherViolations(otherViolations);

                violationsByFlows.put(reportId, flowViolationsBuilder.build());

                // Same as above
            }
        };

        for (String id : ids) {
            var reportId = ReportId.of(id, type).withSubType(subType);
            var processId = switch (type) {
                case RELEASE -> CiProcessId.ofRelease(configPath, id);
                case FLOW -> CiProcessId.ofFlow(configPath, id);
                default -> throw new IllegalStateException("Unsupported type: " + type);
            };

            switch (type) {
                case RELEASE -> {
                    var release = aYamlConfig.getCi().findRelease(reportId.id);
                    if (release.isPresent()) {
                        var releaseConfig = release.get();
                        validateImpl.accept(processId, reportId.withFlowReference(
                                FlowReference.of(releaseConfig.getFlow(), Common.FlowType.FT_DEFAULT)
                        ));
                        BiConsumer<Common.FlowType, List<FlowWithFlowVars>> validateRelease = (flowType, refs) -> {
                            for (var flowRef : refs) {
                                var flowId = flowRef.getFlow();
                                if (!Objects.equals(reportId.id, flowId)) {
                                    var newReport = reportId.withFlowReference(FlowReference.of(flowId, flowType));
                                    validateImpl.accept(processId, newReport);
                                }
                            }
                        };

                        validateRelease.accept(Common.FlowType.FT_HOTFIX, releaseConfig.getHotfixFlows());
                        validateRelease.accept(Common.FlowType.FT_ROLLBACK, releaseConfig.getRollbackFlows());
                    }
                }
                default -> {
                    validateImpl.accept(processId, reportId.withFlowReference(
                            FlowReference.defaultIfNull(aYamlConfig.getCi(), processId, null))
                    );
                }
            }
        }
    }

    @Nullable
    private JsonObject createFakeValuesForRequiredFields(FlowRunConfig runConfig) {
        if (runConfig.getFlowVarsUi() == null) {
            return null;
        }
        var jsonNode = jsonSchemaFaker.tryMakeValueBySchema(runConfig.getFlowVarsUi().getSchema(), true);
        if (!jsonNode.isObject()) {
            return null;
        }
        return JsonParser.parseString(jsonNode.toString()).getAsJsonObject();
    }

    private FlowViolations validateFlow(Flow flow, ReportId reportId) {
        var flowViolations = new HashMap<String, Set<String>>();
        var outdatedTasklets = new HashSet<String>();

        for (var jobList : List.of(flow.getJobs(), flow.getCleanupJobs())) {
            var validationJobMapping = jobList.stream()
                    .collect(Collectors.toMap(Function.identity(), this::toValidationJob));

            var jobs = jobList.stream()
                    .map(job -> toValidationJobWithUpstreams(job, validationJobMapping)).toList();

            for (var job : jobs) {
                var resourceDependencyMap = job.getJobExecutorObject().getResourceDependencyMap();

                var actualConsumedResources = resourceProvider.getResources(job, resourceDependencyMap);

                //TODO CI-1047 Check exact resource number
                //TODO Check additional information e.g. parent field
                var violations = validateJobInputOutput(actualConsumedResources, resourceDependencyMap);

                if (!violations.isEmpty()) {
                    Preconditions.checkState(
                            !flowViolations.containsKey(job.getId()),
                            "Job with id %s has been already matched in %s",
                            job.getId(),
                            reportId
                    );

                    flowViolations.put(job.getId(), violations);
                }

                if (ExecutorType.selectFor(job.getExecutorContext()) == ExecutorType.TASKLET) {
                    var taskletKey = Objects.requireNonNull(job.getExecutorContext().getTasklet()).getTaskletKey();
                    var taskletMetadata = taskletMetadataService.fetchMetadata(taskletKey);
                    if (!taskletMetadata.getFeatures().isConsumesSecretId()) {
                        outdatedTasklets.add(job.getJobProperties().getTaskId().getId());
                    }
                }
            }
        }

        return FlowViolations.builder()
                .violationsByJobs(flowViolations)
                .outdatedTasklets(outdatedTasklets)
                .build();
    }

    private ValidationJob toValidationJobWithUpstreams(Job job, Map<Job, ValidationJob> validationJobsMapping) {
        ValidationJob validationJob = validationJobsMapping.get(job);

        validationJob.addAllUpstreams(
                job.getUpstreams().stream()
                        .map(link -> new UpstreamLink<>(
                                validationJobsMapping.get(link.getEntity()),
                                link.getType(),
                                link.getStyle()
                        )).collect(Collectors.toSet())
        );

        return validationJob;
    }

    private ValidationJob toValidationJob(Job job) {
        var executorContext = job.getExecutorContext();
        return ValidationJob.of(job, sourceCodeService.getJobExecutor(executorContext), executorContext);
    }

    private Set<String> validateJobInputOutput(
            Multimap<JobResourceType, ResourceParentJobId> actualReceivedResources,
            Map<JobResourceType, ConsumedResource> resourceDependencyMap
    ) {
        var missedResources = resourceDependencyMap.entrySet().stream()
                .filter(e -> !actualReceivedResources.containsKey(e.getKey()))
                .filter(e -> !CI_TASKLET_CONTEXT.equals(e.getKey()))
                .filter(e -> !JobResourceType.isSandboxTaskContext(e.getKey()))
                .filter(e -> !e.getValue().isList())
                .map(e -> e.getKey().getMessageName()).collect(Collectors.toList());

        var unexpectedMultipleResources = resourceDependencyMap.entrySet().stream()
                .filter(e -> actualReceivedResources.containsKey(e.getKey()))
                .filter(e -> !e.getValue().isList())
                .filter(e -> actualReceivedResources.get(e.getKey()).size() > 1)
                .map(e -> new ResourceTypeJobId(
                                e.getKey().getMessageName(),
                                actualReceivedResources.get(e.getKey()).stream()
                                        .map(ResourceParentJobId::getJobId).collect(Collectors.toList())
                        )
                )
                .toList();

        var violations = new HashSet<String>();

        if (!missedResources.isEmpty()) {
            violations.add(
                    String.format(
                            "resources of following types are missed: %s, actual received resources: %s",
                            String.join(", ", missedResources),
                            actualReceivedResources.keySet().stream()
                                    .map(JobResourceType::getMessageName)
                                    .collect(Collectors.joining(", "))
                    )
            );
        }

        if (!unexpectedMultipleResources.isEmpty()) {
            violations.add(
                    String.format(
                            "unexpected multiple resources of following types: %s",
                            unexpectedMultipleResources.stream()
                                    .map(r -> String.format(
                                            "%s (got from %s)",
                                            r.getMessageTypeName(),
                                            String.join(", ", r.getJobIds())
                                    ))
                                    .collect(Collectors.joining("; "))
                    )
            );
        }

        return violations;
    }

    @Value
    public static class Report {
        @Nonnull
        Map<ReportId, FlowViolations> violationsByFlows;

        // Пробросить taskId из AYaml конфиг в Job можно, но пока в этом нет смысла
        @Nonnull
        LinkedHashMap<String, String> deprecatedTasks;

        public boolean isValid() {
            return getViolationsByFlows().isEmpty() && deprecatedTasks.isEmpty();
        }
    }

    @Value
    @SuppressWarnings("ReferenceEquality")
    public static class ReportId {
        @Nonnull
        String id;

        @Nonnull
        CiProcessId.Type type;

        @Nullable
        @With
        FlowReference flowReference;

        @With
        @Nullable
        String subType;

        public static ReportId of(@Nonnull String id, @Nonnull CiProcessId.Type type) {
            return new ReportId(id, type, null, null);
        }

        @Override
        public String toString() {
            var processType = subType != null ? subType : this.type.getPrettyPrint();
            var maybeFlowId = flowReference != null
                    && !flowReference.getFlowId().equals(id)
                    && flowReference.getFlowType() != Common.FlowType.FT_DEFAULT
                    ? (", flow \"" + flowReference.getFlowId() + "\"")
                    : "";
            var maybeFlowType = flowReference != null && flowReference.getFlowType() != Common.FlowType.FT_DEFAULT
                    ? " of " + flowReference.getFlowType()
                    : "";

            return processType + maybeFlowType + " with id \"" + id + "\"" + maybeFlowId;
        }
    }

    @Value
    @Builder
    public static class FlowViolations {
        @Singular
        Map<String, Set<String>> violationsByJobs;

        @Singular
        Set<String> otherViolations;

        @Singular
        Set<String> outdatedTasklets;

        public boolean hasViolations() {
            return !violationsByJobs.isEmpty() || !otherViolations.isEmpty() || !outdatedTasklets.isEmpty();
        }
    }

    @Nonnull
    @NonNullApi
    @Value
    private static class ResourceTypeJobId {
        String messageTypeName;
        List<String> jobIds;
    }
}
