package ru.yandex.ci.flow.engine.definition.context.impl;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.function.Consumer;

import com.google.common.collect.ListMultimap;
import com.google.common.collect.Multimaps;

import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.job.JobProgress;
import ru.yandex.ci.flow.engine.runtime.state.JobProgressService;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;

public class JobProgressContextImpl implements JobProgressContext {
    private final JobContextImpl context;
    private final JobProgressService progressService;
    private final JobProgress currentProgress = new JobProgress();

    public JobProgressContextImpl(JobContextImpl context, JobProgressService progressService) {
        this.context = context;
        this.progressService = progressService;
    }

    @Override
    public void update(Consumer<ProgressBuilder> callback) {
        callback.accept(new ProgressBuilder(currentProgress));
        context.updateFlowLaunch(
                () -> progressService.changeProgress(
                        context,
                        currentProgress.getText(),
                        currentProgress.getRatio(),
                        new ArrayList<>(currentProgress.getTaskStates().values())
                )
        );
    }

    @Override
    public TaskBadge getTaskState(String module) {
        return getTaskState(0, module);
    }

    @Override
    public TaskBadge getTaskState(long index, String module) {
        return currentProgress.getTaskStates().get(module + index);
    }

    @Override
    public Collection<TaskBadge> getTaskStates() {
        return currentProgress.getTaskStates().values();
    }

    @Override
    public void updateTaskState(TaskBadge taskBadge) {
        progressService.changeTaskBadge(context.getFullJobLaunchId(), taskBadge);
    }

    public static ProgressBuilder builder(JobProgress progress) {
        return new ProgressBuilder(progress);
    }

    public static class ProgressBuilder {
        private final JobProgress progress;

        private ProgressBuilder(JobProgress progress) {
            this.progress = progress;
        }

        public ProgressBuilder setText(String text) {
            this.progress.setText(text);
            return this;
        }

        public ProgressBuilder setRatio(Float ratio) {
            this.progress.setRatio(ratio);
            return this;
        }

        public ProgressBuilder clearTaskStates() {
            this.progress.getTaskStates().clear();
            return this;
        }

        public ProgressBuilder setTaskBadge(TaskBadge state) {
            setTaskBadge(0, state);
            return this;
        }

        public ProgressBuilder setTaskBadge(long index, TaskBadge state) {
            this.progress.getTaskStates().put(state.getModule() + index, state);
            return this;
        }

        public ProgressBuilder setTaskBadge(String id, TaskBadge state) {
            this.progress.getTaskStates().put(id, state);
            return this;
        }

        public ProgressBuilder setTaskStates(TaskBadge... states) {
            return setTaskStates(Arrays.asList(states));
        }

        public ProgressBuilder setTaskStates(List<TaskBadge> states) {
            ListMultimap<String, TaskBadge> moduleToStates = Multimaps.index(states, TaskBadge::getModule);
            for (String module : moduleToStates.keySet()) {
                List<TaskBadge> moduleStates = moduleToStates.get(module);
                for (int i = 0; i < moduleStates.size(); i++) {
                    setTaskBadge(i, moduleStates.get(i));
                }
            }
            return this;
        }
    }
}
