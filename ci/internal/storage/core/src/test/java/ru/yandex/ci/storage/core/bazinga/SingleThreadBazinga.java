package ru.yandex.ci.storage.core.bazinga;

import java.util.List;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.joda.time.Instant;

import ru.yandex.bolts.collection.Option;
import ru.yandex.bolts.collection.impl.ArrayListF;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.impl.OnetimeUtils;
import ru.yandex.commune.bazinga.impl.storage.BazingaStorage;
import ru.yandex.commune.bazinga.impl.worker.WorkerTask;
import ru.yandex.commune.bazinga.impl.worker.WorkerTaskRegistry;
import ru.yandex.commune.bazinga.scheduler.ExecutionContext;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;
import ru.yandex.misc.db.q.SqlLimits;

@AllArgsConstructor
@Slf4j
public class SingleThreadBazinga {
    private final BazingaStorage bazingaStorage;
    private final WorkerTaskRegistry workerTaskRegistry;

    public void executeAll() {
        while (true) {
            var jobsToExecute = bazingaStorage.findOnetimeJobsToExecute(
                    Instant.now(),
                    SqlLimits.all(),
                    Option.empty()
            );

            if (jobsToExecute.isEmpty()) {
                return;
            }

            log.info("{} tasks to execute", jobsToExecute.size());

            jobsToExecute.forEach(this::execute);
        }
    }

    private void execute(OnetimeJob job) {
        runTask(workerTaskRegistry.getOnetimeTask(job.getTaskId()), job);
    }

    private void runTask(final OnetimeTask task, OnetimeJob job) {
        task.setParameters(OnetimeUtils.parseParameters(task, job.getParameters()));
        var workerTask = WorkerTask.onetime(task, job);

        var context = new ExecutionContext(
                bazingaStorage,
                workerTask.isOnetime(),
                workerTask.getFullJobId(),
                workerTask.getPartition(),
                workerTask.getExecutionInfo()
        );

        try {
            task.execute(context);
        } catch (Exception e) {
            throw new RuntimeException("Failed to execute", e);
        }

        bazingaStorage.deleteOnetimeJobs(new ArrayListF<>(List.of(job.getId())));
    }
}
