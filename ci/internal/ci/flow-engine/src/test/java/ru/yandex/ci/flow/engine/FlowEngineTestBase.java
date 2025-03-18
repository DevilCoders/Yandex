package ru.yandex.ci.flow.engine;

import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import org.junit.jupiter.api.BeforeEach;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonYdbTestBase;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.definition.builder.FlowBuilder;
import ru.yandex.ci.flow.engine.definition.builder.JobBuilderImpl;
import ru.yandex.ci.flow.engine.definition.stage.Stage;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTester;
import ru.yandex.ci.flow.engine.runtime.state.StageGroupState;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobType;
import ru.yandex.ci.flow.engine.runtime.test_data.simple.SimpleFlow;
import ru.yandex.ci.flow.spring.FlowEngineTestConfig;
import ru.yandex.ci.flow.spring.FlowServicesConfig;

@ContextConfiguration(classes = {
        FlowServicesConfig.class,
        FlowEngineTestConfig.class
})
public class FlowEngineTestBase extends CommonYdbTestBase {
    @Autowired
    protected FlowTester flowTester;

    @SuppressWarnings("HidingField")
    @Autowired
    protected CiDb db;

    @BeforeEach
    void beforeEachFlowEngineTestBase() {
        flowTester.reset();
        flowTester.register(SimpleFlow.flow(), SimpleFlow.FLOW_ID);
    }

    protected void flowLaunchSave(FlowLaunchEntity entity) {
        db.currentOrTx(() ->
                db.flowLaunch().save(entity));
    }

    protected FlowLaunchEntity flowLaunchGet(FlowLaunchId id) {
        return db.currentOrReadOnly(() ->
                db.flowLaunch().get(id));
    }

    protected void stageGroupSave(StageGroupState state) {
        db.currentOrTx(() ->
                db.stageGroup().save(state));
    }

    protected StageGroupState stageGroupGet(String id) {
        return db.currentOrReadOnly(() ->
                db.stageGroup().get(id));
    }

    protected static FlowBuilder flowBuilder() {
        return FlowBuilder.create(FlowEngineTestBase::jobBuilder);
    }

    private static JobBuilderImpl jobBuilder(String id, JobType jobType) {
        return new JobBuilderImpl(id, jobType) {
            @Nullable
            @Override
            public Stage getStage() {
                var stage = super.getStage();
                if (stage != null) {
                    return stage;
                }

                List<Stage> possibleStages = getUpstreams().stream()
                        .map(u -> u.getEntity().getStage())
                        .filter(Objects::nonNull)
                        .distinct()
                        .collect(Collectors.toList());

                if (possibleStages.isEmpty()) {
                    return null;
                }

                Preconditions.checkState(
                        possibleStages.size() == 1, "Job %s belongs to several stages %s",
                        this, possibleStages
                );

                return possibleStages.get(0);
            }
        };
    }
}
