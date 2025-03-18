package ru.yandex.ci.tms.bazinga;

import org.joda.time.Duration;
import org.joda.time.Instant;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Disabled;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.boot.test.mock.mockito.SpyBean;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.common.bazinga.TestBazingaUtils;
import ru.yandex.ci.common.bazinga.monitoring.HelloService;
import ru.yandex.ci.common.bazinga.monitoring.TestOneTimeTask;
import ru.yandex.ci.common.bazinga.monitoring.TestOneTimeWithContextTask;
import ru.yandex.ci.common.bazinga.spring.BazingaCoreConfig;
import ru.yandex.ci.common.bazinga.spring.S3BazingaLoggerTestConfig;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.tms.spring.bazinga.BazingaServiceConfig;
import ru.yandex.commune.bazinga.BazingaTaskManager;
import ru.yandex.commune.bazinga.impl.FullJobId;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.verify;

@Disabled("Disabled until we figure out how to deal with flaky")
@ContextConfiguration(classes = {
        TestOneTimeTask.class,
        TestOneTimeWithContextTask.class,
        BazingaServiceConfig.class,
        S3BazingaLoggerTestConfig.class,
        BazingaCoreConfig.class,
        HelloService.class
})
class BazingaTest extends YdbCiTestBase {

    @Autowired
    private BazingaTaskManager bazingaTaskManager;

    @Autowired
    private TestOneTimeTask oneTimeTask;

    @SpyBean
    private HelloService helloService;

    @BeforeEach
    public void setUp() {
        oneTimeTask.reset();
    }

    @Test
    void testTask() throws Exception {
        var task = new TestOneTimeTask("Billy Jean");
        var jobId = bazingaTaskManager.schedule(task);
        wait(jobId);
        assertThat(oneTimeTask.getCalls()).containsExactly("Billy Jean");
    }

    @Test
    void testScheduledInPastTask() throws InterruptedException {
        var task = new TestOneTimeTask("scheduled Billy Jean");
        var jobId = bazingaTaskManager.schedule(task, Instant.now().minus(Duration.standardMinutes(4)));
        wait(jobId);
        assertThat(oneTimeTask.getCalls()).containsExactly("scheduled Billy Jean");
    }

    @Test
    void reschedule() throws InterruptedException {
        var task = new TestOneTimeTask("scheduled task");
        var now = Instant.now();
        var jobId = bazingaTaskManager.schedule(task, now.plus(Duration.standardMinutes(99)));
        var jobId2 = bazingaTaskManager.schedule(task, now.plus(Duration.standardSeconds(2)));
        assertThat(jobId2).isEqualTo(jobId);
        wait(jobId);
        assertThat(oneTimeTask.getCalls()).containsExactly("scheduled task");
    }

    @Test
    void testTaskWithSpringContext() throws Exception {
        var task = new TestOneTimeWithContextTask("Billy");
        var jobId = bazingaTaskManager.schedule(task);
        wait(jobId);
        verify(helloService).makeHello("Billy");
    }

    private void wait(FullJobId jobId) throws InterruptedException {
        TestBazingaUtils.waitOrFail(bazingaTaskManager, jobId);
    }
}
