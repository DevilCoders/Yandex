package ru.yandex.ci.flow.engine.runtime;

import java.time.Clock;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import java.util.stream.Collectors;
import java.util.stream.IntStream;

import com.google.gson.JsonObject;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.flow.engine.FlowEngineTestBase;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceId;
import ru.yandex.misc.random.Random2;

import static org.assertj.core.api.Assertions.assertThat;
import static org.assertj.core.api.Assertions.assertThatThrownBy;

public class JobResourcesValidatorTest extends FlowEngineTestBase {
    private static final String CI_PROJECT = "ci";
    private static final String ANY_PROJECT = "any";

    private static final FlowLaunchId LAUNCH_1 = FlowLaunchId.of("l-1");
    private static final FlowLaunchId LAUNCH_2 = FlowLaunchId.of("l-2");

    @Autowired
    private JobResourcesValidator validator;

    @Autowired
    private Clock clock;

    @Test
    void testSaveResourcesNormal() {
        validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, singleResource(LAUNCH_1));
    }

    @Test
    void testSaveResourcesMulti() {
        validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, singleResource(LAUNCH_1));
        validator.validateAndSaveResources(CI_PROJECT, LAUNCH_2, singleResource(LAUNCH_2));

        var entities = db.currentOrReadOnly(() -> db.resources().findAll());
        assertThat(entities)
                .hasSize(2);

        var expect = entities.stream()
                .map(ResourceEntity::getResourceType)
                .collect(Collectors.toSet());
        assertThat(expect)
                .isEqualTo(Set.of("message-type-1"));
    }

    @Test
    void testSaveResourcesWithLargeNumberOfDifferentResources() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "type-2", 210, 100));
        db.currentOrTx(() ->
                db.resources().saveResources(staticResources, ResourceEntity.ResourceClass.STATIC, clock));

        validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, singleResource(LAUNCH_1));

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(211);
    }


    @Test
    void testSaveResourcesExceedArtifactSize() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "artifact-too-large", 10, 10241));
        assertThatThrownBy(() -> validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, staticResources))
                .hasMessage("Unable to save resources. Max artifact size is 10240 bytes, " +
                        "found JobResourceType(messageName=artifact-too-large) of 10263 bytes");

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(0);
    }

    @Test
    void testSaveResourcesAcceptArtifactCountPerTypeCI() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "too-much-per-type", 50, 1));
        validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, staticResources);

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(50);
    }

    @Test
    void testSaveResourcesAcceptArtifactCountPerTypeAny() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "too-much-per-type", 250, 1));
        validator.validateAndSaveResources(ANY_PROJECT, LAUNCH_1, staticResources);

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(250);
    }

    @Test
    void testSaveResourcesExceedArtifactCountPerTypeCI() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "too-much-per-type", 51, 1));
        assertThatThrownBy(() -> validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, staticResources))
                .hasMessage("Unable to save resources. " +
                        "Artifacts per resource limit is 50, found too-much-per-type=51");

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(0);
    }

    @Test
    void testSaveResourcesExceedArtifactCountPerTypeAny() {
        var staticResources = new StoredResourceContainer(resources(LAUNCH_1, "too-much-per-type", 251, 1));
        assertThatThrownBy(() -> validator.validateAndSaveResources(ANY_PROJECT, LAUNCH_1, staticResources))
                .hasMessage("Unable to save resources. " +
                        "Artifacts per resource limit is 250, found too-much-per-type=251");

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(0);
    }

    @Test
    void testSaveResourcesExceedTotalArtifactsCount() {
        var list = new ArrayList<StoredResource>();
        list.addAll(resources(LAUNCH_1, "t-1", 50, 1));
        list.addAll(resources(LAUNCH_1, "t-2", 50, 1));
        list.addAll(resources(LAUNCH_1, "t-3", 50, 1));
        list.addAll(resources(LAUNCH_1, "t-4", 50, 1));
        list.addAll(resources(LAUNCH_1, "t-5", 1, 1));

        var staticResources = new StoredResourceContainer(list);
        assertThatThrownBy(() -> validator.validateAndSaveResources(CI_PROJECT, LAUNCH_1, staticResources))
                .hasMessage("Unable to save resources. Total artifact count limit is 200, stored 201");

        assertThat(db.currentOrReadOnly(() -> db.resources().countAll()))
                .isEqualTo(0);
    }


    public static StoredResourceContainer singleResource(FlowLaunchId flowLaunchId) {
        return new StoredResourceContainer(resources(flowLaunchId, "message-type-1", 1, 100));
    }


    public static Collection<StoredResource> resources(
            FlowLaunchId flowLaunchId,
            String resourceType,
            int count,
            int size) {
        return IntStream.range(0, count)
                .mapToObj(i -> {
                    var object = new JsonObject();
                    object.addProperty("index", i);
                    object.addProperty("value", Random2.R.nextString(size));
                    return new StoredResource(
                            StoredResourceId.generate(),
                            "flow-id",
                            flowLaunchId,
                            UUID.randomUUID(),
                            object,
                            JobResourceType.of(resourceType),
                            List.of());
                })
                .collect(Collectors.toList());
    }

}
