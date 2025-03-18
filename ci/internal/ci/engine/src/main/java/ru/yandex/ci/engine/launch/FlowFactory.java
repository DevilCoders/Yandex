package ru.yandex.ci.engine.launch;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.AYamlConfig;
import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.config.a.validation.AYamlValidationException;
import ru.yandex.ci.core.config.registry.TaskConfig;
import ru.yandex.ci.core.config.registry.TaskId;
import ru.yandex.ci.core.launch.FlowReference;
import ru.yandex.ci.core.launch.FlowRunConfig;
import ru.yandex.ci.core.tasklet.SchemaService;
import ru.yandex.ci.core.tasklet.TaskletMetadataService;
import ru.yandex.ci.core.taskletv2.TaskletV2MetadataService;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageBuilder;
import ru.yandex.ci.flow.engine.definition.stage.StageGroup;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.source_code.SourceCodeService;
import ru.yandex.ci.flow.utils.FlowLayoutHelper;

@AllArgsConstructor
public class FlowFactory {
    private final TaskletMetadataService taskletMetadataService;
    private final TaskletV2MetadataService taskletV2MetadataService;
    private final SchemaService schemaService;
    private final SourceCodeService sourceCodeService;

    public Flow create(@Nonnull CiProcessId processId,
                       @Nonnull AYamlConfig yamlConfig,
                       @Nonnull Map<TaskId, TaskConfig> taskConfigs,
                       @Nonnull FlowCustomizedConfig customizedConfig)
            throws AYamlValidationException {
        return createFlow(processId, yamlConfig, taskConfigs, customizedConfig);
    }

    private Flow createFlow(CiProcessId processId,
                            AYamlConfig yamlConfig,
                            Map<TaskId, TaskConfig> taskConfigs,
                            FlowCustomizedConfig customizedConfig)
            throws AYamlValidationException {
        FlowBuilder flowBuilder = FlowBuilder.create();

        CiConfig ciConfig = yamlConfig.getCi();

        String flowId;
        @Nullable StageGroup stageGroup;

        var flowReference = FlowReference.defaultIfNull(yamlConfig.getCi(),
                processId, customizedConfig.getFlowReference());

        var flowVars = customizedConfig.getFlowVars() != null
                ? customizedConfig.getFlowVars()
                : FlowRunConfig.lookup(processId, yamlConfig.getCi(), flowReference)
                        .getFlowVars();

        switch (processId.getType()) {
            case RELEASE -> {
                var release = ciConfig.getRelease(processId.getSubId());

                flowId = customizedConfig.getFlowReference() != null
                        ? customizedConfig.getFlowReference().getFlowId()
                        : release.getFlow();

                stageGroup = release.getStages().stream()
                        .map(s -> StageBuilder
                                .create(s.getId())
                                .withTitle(s.getTitle())
                                .withCanBeInterrupted(true)
                                .withDisplacementOptions(s.getDisplace() != null
                                        ? s.getDisplace().getOnStatus()
                                        : Set.of())
                                .withRollback(s.isRollback())
                        )
                        .collect(Collectors.collectingAndThen(Collectors.toList(), StageGroup::new));

                Preconditions.checkState(!stageGroup.getStages().isEmpty(),
                        "stage group in release %s cannot be empty", processId);
            }
            case FLOW -> {
                var action = ciConfig.findAction(processId.getSubId());
                if (action.isPresent()) {
                    flowId = action.get().getFlow();
                } else {
                    flowId = processId.getSubId();
                }
                stageGroup = null;
            }
            default -> throw new IllegalArgumentException("unexpected process type in process " + processId);
        }

        FlowConfig flow = ciConfig.getFlow(flowId);
        if (customizedConfig.getFlowIdListener() != null) {
            customizedConfig.getFlowIdListener().accept(flowId);
        }

        for (var jobs : List.of(
                Jobs.of(flow.getJobs().values(), JobType.DEFAULT),
                Jobs.of(flow.getCleanupJobs().values(), JobType.CLEANUP))) {
            var builder = new FlowFactoryBuilder(
                    taskletMetadataService,
                    taskletV2MetadataService,
                    schemaService,
                    sourceCodeService,
                    jobs.getJobs(),
                    flowBuilder,
                    jobs.getType(),
                    taskConfigs,
                    stageGroup,
                    flowVars);
            builder.buildJobs();
        }

        var top = FlowLayoutHelper.repositionJobs(flowBuilder.getJobBuilders(), 0);
        FlowLayoutHelper.repositionJobs(flowBuilder.getCleanupJobBuilders(), top);

        checkMultiplyByNames(flowBuilder); // TODO: validate in configuration, not here

        // TODO stages
        return flowBuilder.build();
    }

    private void checkMultiplyByNames(FlowBuilder flowBuilder) throws AYamlValidationException {
        var idMap = Stream.of(flowBuilder.getJobBuilders(), flowBuilder.getCleanupJobBuilders())
                .flatMap(Collection::stream)
                .map(JobBuilder::getId)
                .collect(Collectors.toSet());

        for (var jobs : List.of(flowBuilder.getJobBuilders(), flowBuilder.getCleanupJobBuilders())) {
            for (var job : jobs) {
                var multiply = job.getMultiply();
                if (multiply == null) {
                    continue; // ---
                }
                for (int i = 0; i < JobMultiplyConfig.MAX_JOBS; i++) {
                    var multiId = JobMultiplyConfig.getId(job.getId(), JobMultiplyConfig.index(i));
                    if (!idMap.contains(multiId)) {
                        continue; // ---
                    }
                    throw new AYamlValidationException(job.getId(),
                            String.format(
                                    "Dynamic job [%s] will be generated and intersected with another job [%s]",
                                    job.getId(), multiId
                            )
                    );
                }
            }
        }
    }

    @Value(staticConstructor = "of")
    private static class Jobs {
        Collection<JobConfig> jobs;
        JobType type;
    }
}
