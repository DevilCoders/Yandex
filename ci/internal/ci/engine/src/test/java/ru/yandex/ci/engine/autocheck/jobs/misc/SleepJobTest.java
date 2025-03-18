package ru.yandex.ci.engine.autocheck.jobs.misc;

import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;

import ci.tasklets.misc.sleep.Sleep;
import org.junit.jupiter.api.Test;
import org.mockito.ArgumentCaptor;

import ru.yandex.ci.flow.engine.definition.context.JobProgressContext;
import ru.yandex.ci.flow.engine.definition.context.JobResourcesContext;
import ru.yandex.ci.flow.engine.definition.context.impl.JobActionsContextImpl;
import ru.yandex.ci.flow.engine.definition.context.impl.JobContextImpl;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.StatusChange;

import static org.assertj.core.api.AssertionsForClassTypes.assertThat;
import static org.assertj.core.api.AssertionsForClassTypes.fail;
import static org.junit.jupiter.api.Assertions.assertTrue;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.atLeastOnce;
import static org.mockito.Mockito.atMostOnce;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

class SleepJobTest {

    @Test
    public void shouldUpdateProgress() throws Exception {
        JobContextImpl context = mock(JobContextImpl.class);
        JobProgressContext progressContext = mock(JobProgressContext.class);
        when(context.progress()).thenReturn(progressContext);
        when(context.actions()).thenReturn(new JobActionsContextImpl(context));
        JobState jobState = mock(JobState.class);
        when(jobState.getLastLaunch()).thenReturn(new JobLaunch(1, "", List.of(), List.of(StatusChange.queued())));
        when(context.getJobState()).thenReturn(jobState);

        SleepJob job = new SleepJob();
        var config = Sleep.Config.newBuilder()
                .setSleepTime("5s")
                .build();

        var resources = mock(JobResourcesContext.class);
        when(resources.consume(Sleep.Config.class)).thenReturn(config);
        when(context.resources()).thenReturn(resources);

        job.execute(context);

        ArgumentCaptor<String> captor = ArgumentCaptor.forClass(String.class);
        // Progress is updated every full second
        verify(progressContext, atLeastOnce()).updateText(captor.capture());
        assertThat(captor.getValue()).startsWith("Remaining sleep");
    }

    @Test
    public void canBeInterrupted() throws Exception {
        JobContextImpl context = mock(JobContextImpl.class);
        JobState jobState = mock(JobState.class);
        JobLaunch lastLaunch = mock(JobLaunch.class);
        when(jobState.getLastLaunch()).thenReturn(lastLaunch);
        when(jobState.getLastLaunch()).thenReturn(new JobLaunch(1, "", List.of(), List.of(StatusChange.queued())));
        when(context.getJobState()).thenReturn(jobState);


        JobProgressContext progressContext = mock(JobProgressContext.class);
        when(context.progress()).thenReturn(progressContext);
        when(context.actions()).thenReturn(new JobActionsContextImpl(context));
        SleepJob job = new SleepJob();
        var config = Sleep.Config.newBuilder()
                .setSleepTime("100 s")
                .build();
        var resources = mock(JobResourcesContext.class);
        when(resources.consume(Sleep.Config.class)).thenReturn(config);
        when(context.resources()).thenReturn(resources);
        AtomicBoolean interrupted = new AtomicBoolean(false);
        Thread thread = new Thread(() -> {
            try {
                job.execute(context);
            } catch (InterruptedException x) {
                interrupted.set(true);
            } catch (Exception e) {
                fail(e.getMessage());
            }
        });

        thread.start();
        Thread.sleep(300);
        job.interrupt(context);
        thread.interrupt();
        thread.join();

        verify(progressContext, atMostOnce()).updateText(anyString());
        assertTrue(interrupted.get());
    }

}
