package ru.yandex.ci.flow.engine.runtime;

import java.util.UUID;

import ru.yandex.ci.core.test.resources.Res1;
import ru.yandex.ci.core.test.resources.Res2;
import ru.yandex.ci.core.test.resources.Res3;
import ru.yandex.ci.core.test.resources.StringResource;
import ru.yandex.ci.flow.engine.definition.Flow;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilder;
import ru.yandex.ci.flow.engine.definition.common.UpstreamType;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.definition.resources.Consume;
import ru.yandex.ci.flow.engine.definition.resources.Produces;

import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res1;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res2;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.res3;
import static ru.yandex.ci.flow.engine.runtime.test_data.Resources.stringResource;

public class TestUpstreamsFlows {

    public static final String CONSUME_RES1_ID = "consumeRes1";

    private TestUpstreamsFlows() {
    }

    public static Flow noResourceFlow() {
        FlowBuilder builder = FlowBuilder.create();

        builder.withJob(ProduceRes1.ID, "j1")
                .withResources(stringResource("Res1"));

        JobBuilder produceRes2 = builder.withJob(ProduceRes2.ID, "j2")
                .withResources(stringResource("Res2"));

        builder.withJob(ConsumeRes123.ID, CONSUME_RES1_ID)
                .withUpstreams(UpstreamType.NO_RESOURCES, produceRes2);

        return builder.build();
    }

    public static Flow directResourceSequenceFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder produceRes1 = builder.withJob(ProduceRes1.ID, "j1")
                .withResources(stringResource("Res1"));

        JobBuilder produceRes2 = builder.withJob(ProduceRes2.ID, "j2")
                .withResources(stringResource("Res2"))
                .withUpstreams(produceRes1);

        builder.withJob(ConsumeRes123.ID, CONSUME_RES1_ID)
                .withUpstreams(UpstreamType.DIRECT_RESOURCES, produceRes2);

        return builder.build();
    }

    public static Flow directResourceParallelFlow() {
        FlowBuilder builder = FlowBuilder.create();

        JobBuilder produceRes1 = builder.withJob(ProduceRes1.ID, "j1")
                .withResources(stringResource("Res1"));

        JobBuilder produceRes2 = builder.withJob(ProduceRes2.ID, "j2")
                .withUpstreams(produceRes1)
                .withResources(stringResource("Res2"));

        JobBuilder produceRes3 = builder.withJob(ProduceRes3.ID, "j3")
                .withUpstreams(produceRes1)
                .withResources(stringResource("Res3"));

        builder.withJob(ConsumeRes123.ID, CONSUME_RES1_ID)
                .withUpstreams(UpstreamType.NO_RESOURCES, produceRes2)
                .withUpstreams(UpstreamType.DIRECT_RESOURCES, produceRes3);

        return builder.build();
    }

    public static Flow directResourceSequenceWithDownstreamFlow() {
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder produceRes1 = builder.withJob(ProduceRes1.ID, "j1")
                .withResources(stringResource("Res1"));
        JobBuilder produceRes2 = builder.withJob(ProduceRes2.ID, "j2")
                .withResources(stringResource("Res2"))
                .withUpstreams(UpstreamType.DIRECT_RESOURCES, produceRes1);
        builder.withJob(ConsumeRes123.ID, CONSUME_RES1_ID)
                .withUpstreams(produceRes2);
        return builder.build();
    }

    public static Flow allResourceFlow() {
        FlowBuilder builder = FlowBuilder.create();
        JobBuilder produceRes1 = builder.withJob(ProduceRes1.ID, "j1")
                .withResources(stringResource("Res1"));
        JobBuilder produceRes2 = builder.withJob(ProduceRes2.ID, "j2")
                .withResources(stringResource("Res2"))
                .withUpstreams(produceRes1);
        builder.withJob(ConsumeRes123.ID, CONSUME_RES1_ID)
                .withUpstreams(UpstreamType.ALL_RESOURCES, produceRes2);
        return builder.build();
    }


    @Consume(name = "message", proto = StringResource.class)
    @Produces(single = Res1.class)
    public static class ProduceRes1 implements JobExecutor {
        public static final UUID ID = UUID.fromString("ad2dcc27-7eb1-46b7-9b04-4c1064f82f63");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            var message = context.resources().consume(StringResource.class);
            context.resources().produce(res1(message.getString()));
        }
    }

    @Consume(name = "message", proto = StringResource.class)
    @Produces(single = Res2.class)
    public static class ProduceRes2 implements JobExecutor {
        public static final UUID ID = UUID.fromString("199ef7b3-ba3b-4248-b3c6-ed7a42a161ee");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            var message = context.resources().consume(StringResource.class);
            context.resources().produce(res2(message.getString()));
        }
    }

    @Consume(name = "message", proto = StringResource.class)
    @Produces(single = Res3.class)
    public static class ProduceRes3 implements JobExecutor {
        public static final UUID ID = UUID.fromString("1830da61-01d2-4f44-bf9b-a61420eaba13");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
            var message = context.resources().consume(StringResource.class);
            context.resources().produce(res3(message.getString()));
        }
    }

    @Consume(name = "resource1", proto = Res1.class, list = true)
    @Consume(name = "resource2", proto = Res2.class, list = true)
    @Consume(name = "resource3", proto = Res3.class, list = true)
    public static class ConsumeRes123 implements JobExecutor {
        public static final UUID ID = UUID.fromString("66d5eb7d-40fc-4a02-a165-e4acdad71488");

        @Override
        public UUID getSourceCodeId() {
            return ID;
        }

        @Override
        public void execute(JobContext context) throws Exception {
        }
    }
}
