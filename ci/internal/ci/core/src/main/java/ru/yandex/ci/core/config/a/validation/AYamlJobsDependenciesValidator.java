package ru.yandex.ci.core.config.a.validation;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;

import com.google.common.collect.Lists;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.EqualsAndHashCode;
import org.apache.commons.collections4.CollectionUtils;

import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FlowConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.HasFlowRef;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.util.jackson.parse.HasParseInfo;

@AllArgsConstructor
public class AYamlJobsDependenciesValidator {

    @Nonnull
    private final CiConfig ci;
    @Nonnull
    private final ValidationErrors errors;

    public void validate() {
        validateFlows();

        for (var flow : ci.getFlows().values()) {
            validateJobs(flow);
        }
    }

    private void validateFlows() {
        List<String> validNames = ci.getFlows().values().stream()
                .map(FlowConfig::getId)
                .collect(Collectors.toList());
        String validNamesPath = ci.getParseInfo().getParsedPath("flows");

        Stream.of(ci.getTriggers(), ci.getReleases().values(), ci.getActions().values())
                .flatMap(Collection::stream)
                .forEach(container -> checkFlowRef(validNames, validNamesPath, container));

        for (var release : ci.getReleases().values()) {
            BiConsumer<String, List<FlowWithFlowVars>> validateFlows = (pathTag, refs) -> {
                var path = release.getParseInfo().getParsedPath(pathTag);
                var defaultFlow = release.getFlow();
                for (var flowRef : refs) {
                    var flowId = flowRef.getFlow();
                    if (!validNames.contains(flowId)) {
                        errors.add(String.format(
                                "flow '%s' in %s not found in %s",
                                flowId, path, validNamesPath));
                    } else if (flowId.equals(defaultFlow)) {
                        var flowPath = release.getParseInfo().getParsedPath("flow");
                        errors.add(String.format(
                                "flow '%s' in %s is same as %s",
                                flowId, path, flowPath));
                    }
                }
            };
            validateFlows.accept("hotfix-flows", release.getHotfixFlows());
            validateFlows.accept("rollback-flows", release.getRollbackFlows());
        }
    }

    private <T extends HasFlowRef & HasParseInfo> void checkFlowRef(
            List<String> validNames, String validNamesPath, T container
    ) {
        if (!validNames.contains(container.getFlow())) {
            String flowPath = container.getParseInfo().getParsedPath("flow");
            errors.add(String.format(
                    "flow '%s' in %s not found in %s",
                    container.getFlow(),
                    flowPath,
                    validNamesPath
            ));
        }
    }

    private void validateJobs(FlowConfig flowConfig) {
        validateJobs(flowConfig.getJobs().values(), flowConfig.getParseInfo().getParsedPath("jobs"));
        validateJobs(flowConfig.getCleanupJobs().values(), flowConfig.getParseInfo().getParsedPath("cleanup-jobs"));
        validateNoJobIdIntersection(flowConfig);
    }

    private void validateNoJobIdIntersection(FlowConfig flowConfig) {
        var jobs = flowConfig.getJobs();
        var cleanupJobs = flowConfig.getCleanupJobs();

        if (jobs.isEmpty() || cleanupJobs.isEmpty()) {
            return; // ---
        }

        CollectionUtils.intersection(jobs.keySet(), cleanupJobs.keySet()).stream()
                .map(jobId ->
                        String.format("Job with id %s is duplicate, all jobs id must be unique",
                                jobId))
                .forEach(errors::add);
    }

    private void validateJobs(Collection<JobConfig> jobs, String jobsPointer) {

        if (jobs.isEmpty()) {
            return; // ---
        }

        Map<String, Node> nodes = jobs.stream()
                .map(j -> new Node(j.getId(), j))
                .collect(Collectors.toMap(Node::getName, Function.identity()));

        for (Node node : nodes.values()) {
            for (String need : node.getJobConfig().getNeeds()) {
                checkJob(nodes, jobsPointer, node, need);
            }
        }

        List<List<Node>> cycles = new ArrayList<>();
        nodes.values().forEach(node -> findCycles(node, cycles));

        for (List<Node> cycle : cycles) {
            errors.add(
                    Lists.reverse(cycle).stream()
                            .map(Node::getName)
                            .collect(Collectors.joining(
                                    " -> ",
                                    "Jobs cycle found in " +
                                            jobsPointer + ": ",
                                    "."
                                    )
                            )
            );
        }

    }

    private void checkJob(
            Map<String, Node> validJobs,
            String validJobsPointer,
            Node sourceJob,
            String targetJobName
    ) {
        if (!validJobs.containsKey(targetJobName)) {
            errors.add(String.format(
                    "Job '%s' in %s not found in %s",
                    targetJobName,
                    sourceJob.getJobConfig().getParseInfo().getParsedPath("needs"),
                    validJobsPointer
            ));
        } else {
            sourceJob.getAdjacentNodes().add(validJobs.get(targetJobName));
        }
    }

    private Map<Node, List<List<Node>>> findCycles(Node currentNode, List<List<Node>> cyclesOutput) {
        if (currentNode.getStatus() == NodeStatus.VISITED) {
            return Map.of();
        }

        if (currentNode.getStatus() == NodeStatus.IN_PROCESSING) {
            List<Node> cycle = new ArrayList<>();
            cycle.add(currentNode);

            return Map.of(currentNode, List.of(cycle));
        }

        currentNode.setStatus(NodeStatus.IN_PROCESSING);
        Map<Node, List<List<Node>>> processingCycles = new HashMap<>();
        for (Node adjNode : currentNode.getAdjacentNodes()) {
            findCycles(adjNode, cyclesOutput).forEach((node, cycles) -> {
                if (currentNode.equals(node)) {
                    cyclesOutput.addAll(cycles.stream().peek(c -> c.add(currentNode)).collect(Collectors.toList()));
                } else {
                    processingCycles.computeIfAbsent(node, k -> new ArrayList<>()).addAll(cycles);
                }
            });
        }

        currentNode.setStatus(NodeStatus.VISITED);
        processingCycles.values().forEach(cycles -> cycles.forEach(c -> c.add(currentNode)));

        return processingCycles;
    }

    private enum NodeStatus {
        NOT_VISITED,
        IN_PROCESSING,
        VISITED
    }

    @Data
    private static class Node {
        private final String name;
        private final JobConfig jobConfig;
        @EqualsAndHashCode.Exclude
        private final Set<Node> adjacentNodes = new HashSet<>();

        private NodeStatus status = NodeStatus.NOT_VISITED;

        public void addAdjacentNode(Node node) {
            adjacentNodes.add(node);
        }
    }
}
