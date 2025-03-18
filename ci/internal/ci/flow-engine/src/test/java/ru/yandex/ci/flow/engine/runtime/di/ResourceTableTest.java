package ru.yandex.ci.flow.engine.runtime.di;

import java.time.Clock;
import java.util.List;
import java.util.UUID;
import java.util.stream.Collectors;

import io.github.benas.randombeans.api.EnhancedRandom;
import org.assertj.core.api.recursive.comparison.RecursiveComparisonConfiguration;
import org.junit.jupiter.api.Test;
import org.springframework.beans.factory.annotation.Autowired;

import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.core.test.schema.Person;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRef;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResource;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredResourceContainer;
import ru.yandex.ci.flow.engine.runtime.di.model.StoredSourceCodeObject;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.TestUtils;
import ru.yandex.ci.test.random.TestRandomUtils;

import static org.assertj.core.api.Assertions.assertThat;

class ResourceTableTest extends YdbCiTestBase {

    private static final long SEED = -363747220L;
    private static final String TYPE = "some-protobuf-message";

    private final EnhancedRandom random = TestRandomUtils
            .enhancedRandomBuilder(SEED)
            // random-beans не вызывает конструктор, однако нем достигается согласованность className и resourceType
            .randomize(
                    TestUtils.fieldsOfClass(StoredSourceCodeObject.class, "className"),
                    Resource.class::getName
            )
            .randomize(
                    TestUtils.fieldsOfClass(StoredResource.class, "resourceType"),
                    () -> JobResourceType.of(TYPE)
            )
            .build();

    @SuppressWarnings("HidingField")
    @Autowired
    private CiDb db;

    @Test
    public void saveSingle() {
        StoredResource saved = random.nextObject(StoredResource.class, "instantiated", "instance");

        var jobType = JobResourceType.ofDescriptor(Person.getDescriptor());
        ResourceRef id = new ResourceRef(saved.getId(), jobType, UUID.randomUUID());

        saveResources(new StoredResourceContainer(List.of(saved)));

        StoredResourceContainer loadContainer = loadResources(ResourceRefContainer.of(id));
        assertThat(loadContainer).isNotNull();
        assertThat(loadContainer.getResources()).hasSize(1);

        assertThat(loadContainer.getResources().iterator().next())
                .usingRecursiveComparison()
                .usingOverriddenEquals()
                .isEqualTo(saved);
    }

    @Test
    public void saveAndLoad() {
        List<StoredResource> saved = random.objects(StoredResource.class, 5, "instantiated", "instance")
                .collect(Collectors.toList());

        var jobType = JobResourceType.ofDescriptor(Person.getDescriptor());

        List<ResourceRef> ids = saved.stream()
                .map(StoredResource::getId)
                .map(id -> new ResourceRef(id, jobType, UUID.randomUUID()))
                .collect(Collectors.toList());

        saveResources(new StoredResourceContainer(saved));

        StoredResourceContainer loadContainer = loadResources(ResourceRefContainer.of(ids));
        assertThat(loadContainer).isNotNull();

        assertThat(loadContainer.getResources())
                .usingRecursiveFieldByFieldElementComparator(
                        RecursiveComparisonConfiguration.builder().withIgnoreAllOverriddenEquals(false).build()
                )
                .containsExactlyInAnyOrderElementsOf(saved);
    }

    private void saveResources(StoredResourceContainer ref) {
        db.currentOrTx(() ->
                db.resources().saveResources(ref, ResourceEntity.ResourceClass.DYNAMIC, Clock.systemUTC()));
    }

    private StoredResourceContainer loadResources(ResourceRefContainer ref) {
        return db.currentOrReadOnly(() ->
                db.resources().loadResources(ref));
    }

}
