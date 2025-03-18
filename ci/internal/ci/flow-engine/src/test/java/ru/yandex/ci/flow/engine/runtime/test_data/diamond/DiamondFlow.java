package ru.yandex.ci.flow.engine.runtime.test_data.diamond;

import ru.yandex.ci.flow.engine.definition.DummyJob;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;

public class DiamondFlow {
    public static final String FLOW_NAME = "diamond";
    public static final String START_JOB = "start";
    public static final String TOP_JOB = "top";
    public static final String BOTTOM_JOB = "bottom";
    public static final String END_JOB = "end";

    private DiamondFlow() {
        //
    }

    public static Flow flow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder start = builder.withJob(DummyJob.ID, START_JOB);

        JobBuilder top = builder.withJob(DummyJob.ID, TOP_JOB)
                .withUpstreams(start);

        JobBuilder bottom = builder.withJob(DummyJob.ID, BOTTOM_JOB)
                .withUpstreams(start);

        builder.withJob(DummyJob.ID, END_JOB)
                .withUpstreams(top, bottom);

        return builder.build();
    }
}
