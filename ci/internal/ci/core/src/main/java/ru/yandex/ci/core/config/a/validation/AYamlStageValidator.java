package ru.yandex.ci.core.config.a.validation;

import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.AllArgsConstructor;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.EntryStream;

import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;
import ru.yandex.ci.core.config.a.model.StageConfig;

@Slf4j
@AllArgsConstructor
public class AYamlStageValidator {

    @Nonnull
    private final CiConfig ci;
    @Nonnull
    private final ValidationErrors errors;

    public void validate() {
        for (ReleaseConfig release : ci.getReleases().values()) {
            validateRelease(release);
        }
    }

    private void validateRelease(ReleaseConfig release) {
        List<String> declaredStages = release.getStages().stream()
                .map(StageConfig::getId)
                .collect(Collectors.toList());

        List<String> implicitStages = release.getStages().stream()
                .filter(StageConfig::isImplicit)
                .map(StageConfig::getId)
                .collect(Collectors.toList());

        if (implicitStages.size() > 1) {
            errors.add(String.format(
                    "Found more than one implicit stage: %s",
                    implicitStages
            ));
        }

        var implicitStage = implicitStages.isEmpty()
                ? null
                : implicitStages.get(0);

        validateReleaseFlow(release, declaredStages, implicitStage, release.getFlow());
        Stream.of(release.getHotfixFlows(), release.getRollbackFlows())
                .flatMap(Collection::stream) // Flows in hotfix and rollbacks are different from default release flow
                .forEach(ref -> validateReleaseFlow(release, declaredStages, implicitStage, ref.getFlow()));
    }

    private void validateReleaseFlow(ReleaseConfig release,
                                     List<String> declaredStages,
                                     @Nullable String implicitStage,
                                     String flowId) {
        var flowOptional = ci.findFlow(flowId);
        if (flowOptional.isEmpty()) {
            log.info("Flow {} of release {} not found, flow stages validation skipped",
                    flowId, release.getId());
            return; // ---
        }
        var flow = flowOptional.get();

        var state = errors.state();

        if (flow.getCleanupJobs() != null) {
            for (var cleanupJob : flow.getCleanupJobs().values()) {
                if (cleanupJob.getStage() != null) {
                    errors.add(String.format(
                            "Cleanup job %s must not declare any stage",
                            cleanupJob.getId()
                    ));
                }
            }
        }

        if (flow.getJobs().isEmpty()) {
            log.info("No jobs in flow {}, flow stages validation skipped", flow.getId());
            return; // ---
        }
        String flowPointer = flow.getParseInfo().getParsedPath();

        for (JobConfig job : flow.getJobs().values()) {
            if (job.getStage() == null) {
                continue;
            }

            if (!declaredStages.contains(job.getStage())) {
                if (implicitStage != null) {
                    errors.add(String.format(
                            "stage '%s' in %s not declared in release %s. " +
                                    "Stages in release config are not provided, " +
                                    "one implicit stage '%s' is available. " +
                                    "Declare stages in release section or fix job stage property",
                            job.getStage(),
                            job.getParseInfo().getParsedPath("stage"),
                            release.getParseInfo().getParsedPath("stages"),
                            implicitStage
                    ));
                } else {
                    errors.add(String.format(
                            "stage '%s' in %s not declared in release %s. Declared: %s",
                            job.getStage(),
                            job.getParseInfo().getParsedPath("stage"),
                            release.getParseInfo().getParsedPath("stages"),
                            declaredStages
                    ));
                }
            }
        }

        if (state.hasNewErrors()) {
            return; // используются неизвестные стадии, дальнейшая валидация невозможна
        }

        try {
            // В cleanupJobs стадий нет, поэтому их проверять не нужно
            Map<String, JobNode> nodes = createNodes(flow.getJobs());
            boolean hasStages = validateAllJobsHaveStages(nodes, flowPointer);
            if (!hasStages) {
                return; // не все задачи имеют стадии
            }

            List<String> usedStages = getUsedStages(declaredStages, nodes);

            validateStagesOrder(declaredStages, nodes);
            validateDownstreams(nodes, flowPointer);
            validateNoParallelStages(nodes, usedStages, flowPointer);
            validateOnlyOneEntryForStage(nodes, usedStages, flowPointer, implicitStage != null);

        } catch (SeveralStagesException e) {
            // не накапливаем ошибки по каждой джобе, количество ошибок такого типа растет достаточно быстро
            // считай - все узлы после узла с несколькими стейджами - также поломанные. Выводим первый проблемный узел
            errors.add(e.getMessage());
        }
    }

    private List<String> getUsedStages(List<String> declaredStages, Map<String, JobNode> nodes) {
        Set<String> usedStages = nodes.values()
                .stream().map(JobNode::getStage)
                .collect(Collectors.toSet());

        return declaredStages.stream()
                .filter(usedStages::contains)
                .collect(Collectors.toList());
    }

    private void validateOnlyOneEntryForStage(
            Map<String, JobNode> nodes,
            List<String> usedStages,
            String flowPointer,
            boolean implicitStage) {


        for (String stage : usedStages) {
            List<JobNode> stageEntries = nodes.values().stream()
                    .filter(job -> job.getStage().equals(stage) && isStageEntry(stage, job))
                    .toList();

            if (stageEntries.size() != 1) {
                if (implicitStage) {
                    errors.add(
                            String.format("Implicit stage '%s' must have one entry point. Flow %s", stage, flowPointer)
                    );
                } else {
                    errors.add(
                            String.format("Stage %s must have one entry point. Flow %s", stage, flowPointer)
                    );
                }
                return; // ---
            }
        }
    }

    private static boolean isStageEntry(String stage, JobNode job) {
        return job.getNeeds().isEmpty()
                || job.getNeeds().stream()
                .anyMatch(upstream -> !upstream.getStage().equals(stage));
    }

    private void validateNoParallelStages(Map<String, JobNode> nodes, List<String> stages, String flowPointer) {
        for (JobNode node : nodes.values()) {
            int jobStageIndex = stages.indexOf(node.getStage());

            for (JobNode upstream : node.getNeeds()) {
                int upstreamStageIndex = stages.indexOf(upstream.getStage());

                if (jobStageIndex - upstreamStageIndex > 1) {
                    errors.add(
                            String.format("Parallel stages detected in flow %s", flowPointer)
                    );
                    return; // ---
                }
            }
        }
    }

    private void validateDownstreams(Map<String, JobNode> nodes, String flowPointer) {
        List<JobNode> leaves = nodes.values().stream()
                .filter(JobNode::isLeaf)
                .toList();

        long downstreamStagesCount = leaves.stream().map(JobNode::getStage).distinct().count();
        if (downstreamStagesCount != 1) {
            String message = leaves.stream()
                    .collect(Collectors.groupingBy(
                            JobNode::getStage,
                            Collectors.mapping(node -> node.getConfig().getId(), Collectors.toList())
                    ))
                    .entrySet()
                    .stream()
                    .map(sameStage -> sameStage.getKey() + " for jobs " + String.join(", ", sameStage.getValue()))
                    .collect(Collectors.joining("; "));

            errors.add(
                    String.format(
                            "Staged flow must have jobs without downstream only on last stage, got %s, flow %s",
                            message,
                            flowPointer
                    )
            );
        }
    }

    private boolean validateAllJobsHaveStages(Map<String, JobNode> nodes, String flowPointer) {
        List<String> jobsWithoutStage = nodes.values().stream()
                .filter(n -> n.getStage() == null)
                .map(n -> n.getConfig().getId())
                .toList();

        if (!jobsWithoutStage.isEmpty()) {
            errors.add(
                    String.format(
                            "All jobs in flow %s must have stages, invalid jobs: %s",
                            flowPointer,
                            jobsWithoutStage
                    )
            );
            return false;
        }

        return true;
    }

    private void validateStagesOrder(List<String> stages, Map<String, JobNode> nodes) {
        Map<String, Integer> stagePosition = EntryStream.of(stages)
                .map(Function.identity())
                .toMap(Map.Entry::getValue, Map.Entry::getKey);

        for (JobNode node : nodes.values()) {
            Integer currentPosition = stagePosition.get(node.getStage());
            for (JobNode upstream : node.getNeeds()) {
                Integer upstreamPosition = stagePosition.get(upstream.getStage());
                if (upstreamPosition > currentPosition) {
                    errors.add(
                            String.format(
                                    "Downstream cannot have earlier stage than upstream, downstream job = %s at %s, " +
                                            "downstream stage = %s, upstream job = %s at %s, upstream stage = %s",
                                    node.getConfig().getId(), node.getConfig().getParseInfo().getParsedPath(),
                                    node.getStage(),
                                    upstream.getConfig().getId(), upstream.getConfig().getParseInfo().getParsedPath(),
                                    upstream.getStage()
                            )
                    );
                }
            }
        }
    }

    private static Map<String, JobNode> createNodes(Map<String, JobConfig> jobs) {
        Map<String, JobNode> nodes = new HashMap<>(jobs.size());
        Map<String, JobConfig> leaves = new HashMap<>(jobs);

        for (JobConfig job : jobs.values()) {
            leaves.keySet().removeAll(Set.copyOf(job.getNeeds()));
        }

        for (JobConfig value : leaves.values()) {
            getNode(value.getId(), nodes, jobs, true);
        }

        Preconditions.checkState(nodes.size() == jobs.size(), "Not all nodes were traversed");
        return nodes;
    }

    private static JobNode getNode(
            String jobId,
            Map<String, JobNode> nodes,
            Map<String, JobConfig> jobs,
            boolean leaf) {

        JobNode jobNode = nodes.get(jobId);
        if (jobNode != null) {
            return jobNode;
        }
        if (nodes.containsKey(jobId)) {
            // null узел - в текущей обработке
            // честная валидация на все петли циклов производится отдельным валидатором
            // здесь - защита от зацикливания
            throw new CyclesFoundException("Cycle found from job: " + jobId);
        }

        nodes.put(jobId, null);

        JobConfig config = jobs.get(jobId);
        Preconditions.checkState(config != null);

        List<JobNode> needs = config.getNeeds()
                .stream()
                .map(upstream -> getNode(upstream, nodes, jobs, false))
                .collect(Collectors.toList());

        if (config.getStage() != null) {
            jobNode = new JobNode(config, config.getStage(), needs, leaf);
        } else {
            // For validation purposes only
            List<String> possibleStages = needs.stream()
                    .map(JobNode::getStage)
                    .filter(Objects::nonNull)
                    .distinct()
                    .toList();
            if (possibleStages.size() > 1) {
                throw new SeveralStagesException(
                        String.format(
                                "Job %s at %s belongs to several stages %s",
                                jobId,
                                config.getParseInfo().getParsedPath(),
                                possibleStages
                        ));
            } else if (possibleStages.size() == 1) {
                // All stages are updated in AYamlPostProcessor, we don't expect this exception at all
                throw new InvalidStagePostProcessExcepton(
                        String.format(
                                "Job %s at %s has single unconfigured possible stage. This is internal error",
                                jobId,
                                config.getParseInfo().getParsedPath()
                        ));
            }
            jobNode = new JobNode(config, null, needs, leaf);
        }

        nodes.put(jobId, jobNode);
        return jobNode;
    }

    @Value
    private static class JobNode {
        JobConfig config;
        @Nullable
        String stage;
        List<JobNode> needs;
        boolean leaf;
    }

    private static class CyclesFoundException extends RuntimeException {
        CyclesFoundException(String message) {
            super(message);
        }
    }

    private static class SeveralStagesException extends RuntimeException {
        SeveralStagesException(String message) {
            super(message);
        }
    }

    private static class InvalidStagePostProcessExcepton extends RuntimeException {
        InvalidStagePostProcessExcepton(String message) {
            super(message);
        }
    }
}
