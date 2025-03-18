package ru.yandex.ci.common.bazinga;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Set;
import java.util.function.Supplier;

import com.google.common.collect.Multimaps;
import com.google.common.collect.SetMultimap;
import lombok.AccessLevel;
import lombok.Getter;
import lombok.Value;
import org.joda.time.Instant;
import org.springframework.beans.BeansException;

import ru.yandex.bolts.collection.ListF;
import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.JobId;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.TaskId;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.commune.bazinga.scheduler.TaskCategory;
import ru.yandex.misc.db.q.SqlLimits;

@Value
public class TracingBazingaTaskManager implements BazingaTaskManager {

    @Getter(AccessLevel.PRIVATE)
    SetMultimap<Class<? extends OnetimeTask>, FullJobId> scheduledJobs = Multimaps
            .newSetMultimap(new HashMap<>(), HashSet::new);

    BazingaTaskManager taskManager;

    public Set<FullJobId> getScheduledJobs(Class<? extends OnetimeTask> clazz) {
        return scheduledJobs.get(clazz);
    }


    @Override
    public FullJobId schedule(OnetimeTask task) {
        return wrap(task, () -> taskManager.schedule(task));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, Instant date) {
        return wrap(task, () -> taskManager.schedule(task, date));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, TaskCategory category, Instant date) {
        return wrap(task, () -> taskManager.schedule(task, category, date));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, TaskCategory category, Instant date, int priority) {
        return wrap(task, () -> taskManager.schedule(task, category, date, priority));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, TaskCategory category, Instant date, int priority,
                              boolean forceRandomInId) {
        return wrap(task, () -> taskManager.schedule(task, category, date, priority, forceRandomInId));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, TaskCategory category, Instant date, int priority,
                              boolean forceRandomInId, Option<String> group) {
        return wrap(task, () -> taskManager.schedule(task, category, date, priority, forceRandomInId, group));
    }

    @Override
    public FullJobId schedule(OnetimeTask task, TaskCategory category, Instant date, int priority,
                              boolean forceRandomInId, Option<String> group, JobId jobId) {
        return wrap(task, () -> taskManager.schedule(task, category, date, priority, forceRandomInId, group,
                jobId));
    }

    @Override
    public Option<OnetimeJob> getOnetimeJob(FullJobId id) {
        return taskManager.getOnetimeJob(id);
    }

    @Override
    public boolean isJobActive(OnetimeTask task) {
        return taskManager.isJobActive(task);
    }

    @Override
    public ListF<? extends OnetimeTask> getActiveTasks(ListF<? extends OnetimeTask> tasks) {
        return taskManager.getActiveTasks(tasks);
    }

    @Override
    public ListF<OnetimeJob> getActiveJobs(TaskId taskId, SqlLimits limits) {
        return taskManager.getActiveJobs(taskId, limits);
    }

    @Override
    public ListF<OnetimeJob> getFailedJobs(TaskId taskId, SqlLimits limits) {
        return taskManager.getFailedJobs(taskId, limits);
    }

    @Override
    public ListF<OnetimeJob> getJobsByGroup(String group, SqlLimits limits) {
        return taskManager.getJobsByGroup(group, limits);
    }

    private FullJobId wrap(OnetimeTask task, Supplier<FullJobId> scheduler) {
        var fullJobId = scheduler.get();
        scheduledJobs.put(task.getClass(), fullJobId);
        return fullJobId;
    }


    public static class BeanPostProcessor implements org.springframework.beans.factory.config.BeanPostProcessor {
        @Override
        public Object postProcessAfterInitialization(Object bean, String beanName) throws BeansException {
            if (bean instanceof BazingaTaskManager) {
                return new TracingBazingaTaskManager((BazingaTaskManager) bean);
            }
            return bean;
        }
    }

}
