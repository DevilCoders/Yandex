package ru.yandex.ci.flow.engine.runtime.state;

import java.nio.file.Path;
import java.util.Map;
import java.util.stream.Collectors;

import com.google.gson.JsonElement;
import io.github.benas.randombeans.api.EnhancedRandom;
import io.github.benas.randombeans.api.Randomizer;
import org.assertj.core.api.recursive.comparison.RecursiveComparisonConfiguration;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.common.ydb.YdbExecutor;
import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.flow.db.CiFlowTables;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.definition.common.JobSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.definition.common.WeekSchedulerConstraintEntity;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.helpers.FlowTestUtils;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.random.TestRandomUtils;

import static org.assertj.core.api.Assertions.assertThat;

class FlowLaunchTableTest extends FlowEngineTestBase {
    private static final long SEED = -836825185L;

    private static final Map<String, Object> SOME_JOB_LAUNCH_VARIABLES = Map.of(
            "key1", Math.PI, "key2", "String", "key3", true
    );

    @Autowired
    private YdbExecutor executor;

    private final EnhancedRandom random = TestRandomUtils.enhancedRandomBuilder(SEED)
            .collectionSizeRange(2, 3)
            .randomize(
                    TestUtils.fieldsOfClass(JobLaunch.class, "variableMap"),
                    (Randomizer<Map<String, Object>>) () -> SOME_JOB_LAUNCH_VARIABLES
            )
            .randomize(
                    TestUtils.fieldsOfClass(FlowLaunchEntity.class, "launchId"),
                    () -> FlowTestUtils.LAUNCH_ID
            )
            .randomize(Path.class, (Randomizer<Path>) () -> Path.of("ci/a.yaml"))
            .randomize(ArcBranch.class, (Randomizer<ArcBranch>) () -> ArcBranch.ofPullRequest(42))
            .build();

    @Test
    public void saveTest() {
        FlowLaunchEntity saved = random.nextObject(FlowLaunchEntity.class);
        db.currentOrTx(() -> db.flowLaunch().save(saved));

        FlowLaunchEntity loaded = db.currentOrReadOnly(() -> db.flowLaunch().get(saved.getFlowLaunchId()));

        var comparisonConfiguration = RecursiveComparisonConfiguration.builder()
                .withIgnoreAllOverriddenEquals(false)
                .withIgnoredOverriddenEqualsForTypes(
                        JobState.class,
                        JobSchedulerConstraintEntity.class,
                        WeekSchedulerConstraintEntity.class
                )
                .withIgnoredFields(
                        "stateVersion",
                        "flowInfo.manualResources.parentField",
                        "flowInfo.manualResources.data.members.value.value",
                        "flowInfo.flowVars.parentField",
                        "flowInfo.flowVars.data.members.value.value",
                        "flowInfo.flowDescription.rollbackUsingLaunch"
                )
                .build();

        assertThat(loaded).usingRecursiveComparison(comparisonConfiguration).isEqualTo(saved);

        assertThat(loaded.getStateVersion()).isEqualTo(saved.getStateVersion() + 1);
    }

    @Test
    void backwardCompatibility() {
        var rows = TestUtils.forTable(db, CiFlowTables::flowLaunch)
                .upsertValues(executor, "ydb-data/flow-launch.json");

        var ids = rows.stream()
                .map(JsonElement::getAsJsonObject)
                .map(item -> item.getAsJsonPrimitive("id").getAsString())
                .map(FlowLaunchId::of)
                .collect(Collectors.toList());

        assertThat(ids).isNotEmpty();
        db.currentOrReadOnly(() -> {
            for (FlowLaunchId id : ids) {
                assertThat(db.flowLaunch().findOptional(id)).isNotEmpty();
            }
        });
    }
}
