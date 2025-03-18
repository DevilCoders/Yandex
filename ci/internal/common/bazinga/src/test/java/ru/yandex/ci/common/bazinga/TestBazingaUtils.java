package ru.yandex.ci.common.bazinga;

import java.util.HashMap;
import java.util.HashSet;

import com.google.common.base.Preconditions;
import com.google.common.collect.Multimaps;
import com.google.common.collect.SetMultimap;
import org.junit.jupiter.api.Assertions;
import org.mockito.internal.util.MockUtil;

import ru.yandex.bolts.collection.Option;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.FullJobId;
import ru.yandex.commune.bazinga.impl.OnetimeJob;
import ru.yandex.commune.bazinga.scheduler.OnetimeTask;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doAnswer;

public final class TestBazingaUtils {
    private TestBazingaUtils() {
    }

    public static void waitOrFail(BazingaTaskManager taskManager, FullJobId jobId) throws InterruptedException {
        for (int i = 0; i < 1000; i++) {
            Thread.sleep(10);
            if (isCompleted(taskManager, jobId)) {
                return;
            }
        }
        Assertions.fail("Task did not executed");
    }

    public static boolean isCompleted(BazingaTaskManager taskManager, FullJobId jobId) {
        Option<OnetimeJob> onetimeJob = taskManager.getOnetimeJob(jobId);
        if (!onetimeJob.isPresent()) {
            return false;
        }
        return onetimeJob.get().getValue().getStatus().isCompleted();
    }

    public static SetMultimap<Class<? extends OnetimeTask>, FullJobId> spyScheduledJobs(
            BazingaTaskManager bazingaTaskManager
    ) {
        Preconditions.checkArgument(MockUtil.isSpy(bazingaTaskManager), "bazingaTaskManager is not a spy");

        SetMultimap<Class<? extends OnetimeTask>, FullJobId> bazingaJobIds = Multimaps
                .newSetMultimap(new HashMap<>(), HashSet::new);
        doAnswer(invocation -> {
            OnetimeTask task = invocation.getArgument(0);
            FullJobId fullJobId = ((FullJobId) invocation.callRealMethod());
            bazingaJobIds.put(task.getClass(), fullJobId);
            return fullJobId;
        }).when(bazingaTaskManager).schedule(any(OnetimeTask.class));

        return bazingaJobIds;
    }

    public static FullJobId getFirst(
            SetMultimap<Class<? extends OnetimeTask>, FullJobId> scheduledJobs,
            Class<? extends OnetimeTask> clazz
    ) {
        return scheduledJobs.get(clazz).stream().findFirst().orElseThrow();
    }
}
