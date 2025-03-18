package ru.yandex.ci.common.temporal.monitoring;

import java.time.Duration;

import io.temporal.workflow.WorkflowInterface;
import io.temporal.workflow.WorkflowMethod;

import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;
import ru.yandex.ci.common.temporal.TemporalService;
import ru.yandex.ci.common.temporal.workflow.simple.SimpleTestLongId;

@WorkflowInterface
public interface WorkflowWithFailingActivity extends BaseTemporalWorkflow<SimpleTestLongId> {


    @WorkflowMethod
    void run(SimpleTestLongId id);

    class Impl implements WorkflowWithFailingActivity {

        private final FailingActivity activity = TemporalService.createActivity(
                FailingActivity.class,
                Duration.ofSeconds(10)
        );

        @Override
        public void run(SimpleTestLongId id) {
            activity.run((int) id.getId());
        }
    }

}
