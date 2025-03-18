package ru.yandex.ci.common.temporal;

import java.time.Duration;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicBoolean;

import com.cronutils.model.CronType;
import com.cronutils.model.definition.CronDefinitionBuilder;
import com.cronutils.parser.CronParser;
import io.temporal.workflow.Workflow;
import lombok.extern.slf4j.Slf4j;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.common.temporal.workflow.cron.CronTestActivity;
import ru.yandex.ci.common.temporal.workflow.cron.CronTestWorkflow;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestLongId;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestWorkflow2;

import static org.assertj.core.api.Assertions.assertThat;
import static org.awaitility.Awaitility.await;

@Slf4j
class TemporalServiceTest extends TemporalTestBase {

    private static final List<String> EXECUTED_IDS = Collections.synchronizedList(new ArrayList<>());
    private static final AtomicBoolean CRON_ENABLED = new AtomicBoolean();

    @BeforeEach
    @Override
    public void setUp() {
        super.setUp();
        CRON_ENABLED.set(false);
        EXECUTED_IDS.clear();
    }

    @Override
    protected List<Class<?>> workflowImplementationTypes() {
        return List.of(
                SimpleTestWorkflowImpl.class,
                SimpleTestWorkflow2Impl.class,
                CronTestWorkflowImpl.class
        );
    }

    @Override
    protected List<Object> activityImplementations() {
        return List.of();
    }

    @Override
    protected List<Class<?>> cronWorkflowImplementationTypes() {
        return List.of(
                CronTestWorkflowImpl.class
        );
    }

    @Override
    protected List<Object> cronActivityImplementations() {
        return List.of(new CronTestActivityImpl());
    }

    @Test
    void simpleStart() {
        String id = "id42";
        var execution = temporalService.startDeduplicated(
                SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.of(id)
        );

        assertThat(execution.getWorkflowId()).isEqualTo("SimpleTestWorkflow-" + id);

        temporalService.getWorkflowClient()
                .newUntypedWorkflowStub(execution.getWorkflowId()).getResult(Void.class);

        await().atMost(Duration.ofMinutes(1)).until(() -> EXECUTED_IDS.size() > 0);
        assertThat(EXECUTED_IDS).containsExactly(id);
    }

    @Test
    void deduplication() {
        String id = "id42";

        var execution1 = temporalService.startDeduplicated(
                SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.of(id)
        );

        var execution2 = temporalService.startDeduplicated(
                SimpleTestWorkflow.class, wf -> wf::run, SimpleTestId.of(id)
        );
        await().atMost(Duration.ofMinutes(1)).until(() -> EXECUTED_IDS.size() > 0);

        assertThat(execution1.getWorkflowId()).isEqualTo(execution2.getWorkflowId());
        assertThat(execution1.getRunId()).isEqualTo(execution2.getRunId());
    }

    @Test
    void startInTx() {

        String id = "id42";
        temporalDb.tx(() -> {
            temporalService.startInTx(
                    SimpleTestWorkflow.class, (SimpleTestWorkflow wf) -> wf::run, SimpleTestId.of(id)
            );
        });
        await().atMost(Duration.ofMinutes(1)).until(() -> EXECUTED_IDS.size() > 0);

        assertThat(EXECUTED_IDS).containsExactly(id);
    }

    @Test
    void startInTxWithTxCancel() throws Exception {
        String id = "startInTxWithTxCancel";
        try {
            temporalDb.tx(() -> {
                temporalService.startInTx(
                        SimpleTestWorkflow.class, (SimpleTestWorkflow wf) -> wf::run, SimpleTestId.of(id)
                );
                throw new RuntimeException("Don't commit me!");
            });
        } catch (Exception e) {
            log.error("Exception when starting task", e);
        }

        TimeUnit.SECONDS.sleep(5); //Just in case
        assertThat(EXECUTED_IDS).isEmpty();
    }

    @Test
    void scheduleCron() {
        var every5Seconds = new CronParser(CronDefinitionBuilder.instanceDefinitionFor(CronType.QUARTZ))
                .parse("*/5 * * * * ? *");
        log.info("Using cron: {}", every5Seconds);

        CRON_ENABLED.set(true);
        temporalService.registerCronTask(
                CronTestWorkflow.class,
                wf -> wf::run,
                every5Seconds,
                Duration.ofMinutes(1)
        );

        await().atMost(Duration.ofSeconds(30)).until(() -> EXECUTED_IDS.size() >= 2);

        assertThat(EXECUTED_IDS)
                .containsSequence(CronTestActivityImpl.ID, CronTestActivityImpl.ID);
    }

    @Test
    void workflowUrl() {
        String id = "FlowJobWorkflow-be8ad9af51dc112a1fbabfa6b45f74ee2884d0e5b1d0b04186012b612a91a725-sleep2-4-1";
        assertThat(temporalService.getWorkflowUrl(id)).isEqualTo(
                "http://ci-temporal-testing.yandex.net/namespaces/default/workflows?range=last-3-months&status=ALL" +
                        "&workflowId=FlowJobWorkflow-be8ad9af51dc112a1fbabfa6b45f74ee2884d0e5b1d0b04186012b612a91a725" +
                        "-sleep2-4-1"
        );
    }

    public static class SimpleTestWorkflowImpl implements SimpleTestWorkflow {

        @Override
        public void run(SimpleTestId input) {
            EXECUTED_IDS.add(input.getId());
            Workflow.getLogger(SimpleTestWorkflowImpl.class).info("Task {} executed.", input.getId());
        }
    }

    public static class SimpleTestWorkflow2Impl implements SimpleTestWorkflow2 {

        @Override
        public void run(SimpleTestLongId input) {
            Workflow.getLogger(SimpleTestWorkflowImpl.class).info("Task {} executed.", input.getId());
        }
    }

    public static class CronTestWorkflowImpl implements CronTestWorkflow {
        private final CronTestActivity testActivity = TemporalService.createActivity(
                CronTestActivity.class,
                Duration.ofMinutes(5)
        );

        @Override
        public void run() {
            Workflow.getLogger(CronTestWorkflowImpl.class).info("Run");
            testActivity.run();
        }
    }

    public static class CronTestActivityImpl implements CronTestActivity {
        static final String ID = "CronTestWorkflowImpl-cron";

        @Override
        public void run() {
            Workflow.getLogger(CronTestWorkflowImpl.class).info("Run");
            if (CRON_ENABLED.get()) {
                EXECUTED_IDS.add(ID);
            }
        }
    }
}
