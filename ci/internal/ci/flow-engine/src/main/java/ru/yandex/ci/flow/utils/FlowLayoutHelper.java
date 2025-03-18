package ru.yandex.ci.flow.utils;

import java.util.ArrayDeque;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Queue;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import com.google.common.collect.HashMultimap;
import com.google.common.collect.Multimap;
import lombok.Value;

import ru.yandex.ci.flow.engine.definition.Position;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;

public class FlowLayoutHelper {
    public static final int DEFAULT_JOB_SHIFT_X = 300;
    public static final int DEFAULT_JOB_SHIFT_Y = 200;

    private FlowLayoutHelper() {
    }

    @Value
    public static class JobNode {
        String id;
        Set<String> upstreams;
    }

    @Value
    private static class Column {
        int number;
        JobNode job;
    }

    public static Map<String, Integer> getJobIdToColumnMap(Map<String, JobNode> jobs) {
        Multimap<JobNode, JobNode> downstreamMap = HashMultimap.create();

        for (JobNode jobState : jobs.values()) {
            for (String upstreamLink : jobState.upstreams) {
                downstreamMap.put(jobs.get(upstreamLink), jobState);
            }
        }

        List<JobNode> jobsWithoutUpstreams = jobs
                .values()
                .stream()
                .filter(j -> j.upstreams.isEmpty())
                .collect(Collectors.toList());

        Map<String, Integer> stageMap = new HashMap<>();

        Queue<Column> bfsQueue = jobsWithoutUpstreams.stream()
                .map(j -> new Column(0, j))
                .collect(Collectors.toCollection(ArrayDeque::new));

        while (!bfsQueue.isEmpty()) {
            Column column = bfsQueue.poll();
            stageMap.put(column.job.id, column.number);

            bfsQueue.addAll(
                    downstreamMap.get(column.job)
                            .stream()
                            .map(j -> new Column(column.number + 1, j))
                            .collect(Collectors.toList())
            );
        }

        return stageMap;
    }

    public static int repositionJobs(List<JobBuilder> jobs, int topDelta) {
        return repositionJobsImpl(jobs,
                JobBuilder::getId,
                JobBuilder::getUpstreams,
                JobBuilder::getId,
                JobBuilder::withPosition,
                topDelta);
    }

    public static void repositionJobs(FlowBuilder builder) {
        repositionJobs(builder.getJobBuilders(), 0);
    }

    public static void repositionJobs(FlowLaunchEntity entity) {
        repositionJobsImpl(entity.getJobs().values(),
                JobState::getJobId,
                JobState::getUpstreams,
                Function.identity(),
                JobState::setPosition,
                0);
    }

    private static <T, L> int repositionJobsImpl(
            Collection<T> jobs,
            Function<T, String> getJobId,
            Function<T, ? extends Collection<UpstreamLink<L>>> getUpstreams,
            Function<L, String> getUpstreamJobId,
            BiConsumer<T, Position> setPosition,
            int topDelta) {
        Map<String, Integer> jobIdToColumnMap = FlowLayoutHelper.getJobIdToColumnMap(
                jobs.stream()
                        .map(job -> new FlowLayoutHelper.JobNode(
                                getJobId.apply(job),
                                getUpstreams.apply(job).stream().map(u -> getUpstreamJobId.apply(u.getEntity()))
                                        .collect(Collectors.toSet())
                        ))
                        .collect(Collectors.toMap(FlowLayoutHelper.JobNode::getId, Function.identity()))
        );

        Map<Integer, Integer> columnToJobCount = jobIdToColumnMap.values().stream().distinct()
                .collect(Collectors.toMap(Function.identity(), column -> 0));

        int maxTop = 0;
        for (var job : jobs) {
            int column = jobIdToColumnMap.get(getJobId.apply(job));
            int jobsInColumn = columnToJobCount.get(column);
            var pos = new Position(
                    (column + 1) * DEFAULT_JOB_SHIFT_X,
                    (topDelta > 0 ? topDelta + DEFAULT_JOB_SHIFT_Y : 0) + (jobsInColumn + 1) * DEFAULT_JOB_SHIFT_Y);
            setPosition.accept(job, pos);
            columnToJobCount.put(column, jobsInColumn + 1);

            maxTop = Math.max(maxTop, pos.getY());
        }

        return maxTop;
    }
}
