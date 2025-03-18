package ru.yandex.ci.tms.job;

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.Set;
import java.util.stream.Collectors;

import io.github.benas.randombeans.api.EnhancedRandom;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.job.JobStatus;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.ydb.YdbCiTestBase;
import ru.yandex.ci.test.random.TestRandomUtils;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.test.TestUtils.fieldsOfClass;

class JobInstanceTableTest extends YdbCiTestBase {

    private static final long SEED = 1730166036L;

    private static final EnhancedRandom RANDOM = TestRandomUtils.enhancedRandomBuilder(SEED)
            .excludeField(fieldsOfClass(JobInstance.class, "number"))
            .build();

    @Test
    public void mapping() {
        JobInstance original = RANDOM.nextObject(JobInstance.class);

        save(original);
        JobInstance loaded = findOptional(original.getId()).orElse(null);

        assertThat(original).isEqualTo(loaded);
    }

    @Test
    public void loadWithMissedShouldReturnNull() {
        JobInstance.Id notExistsKey = JobInstance.Id.of("not-exists-flow-launch", "job", 0);
        Optional<JobInstance> loaded = findOptional(notExistsKey);
        assertThat(loaded).isEmpty();
    }

    @Test
    public void update() {
        List<JobInstance> original = RANDOM.objects(JobInstance.class, 4).collect(Collectors.toList());
        List<JobInstance.Id> keys = original.stream()
                .map(JobInstance::getId)
                .collect(Collectors.toList());

        saveAll(original);

        List<JobInstance> updated = new ArrayList<>(original.size());
        for (JobInstance originalInstance : original) {
            JobInstance updatedInstance = RANDOM.nextObject(JobInstance.class)
                    .toBuilder()
                    .id(originalInstance.getId())
                    .build();
            assertThat(updatedInstance).usingRecursiveComparison().isNotEqualTo(originalInstance);
            updated.add(updatedInstance);
        }

        saveAll(updated);

        List<JobInstance> loaded = db.currentOrReadOnly(() ->
                db.jobInstance().find(Set.copyOf(keys)));

        assertThat(loaded).usingFieldByFieldElementComparator()
                .containsExactlyInAnyOrderElementsOf(updated);
    }

    @Test
    public void findRunning() {
        db.currentOrTx(() -> {
            save(RANDOM.nextObject(JobInstance.class).withStatus(JobStatus.CREATED));
            save(RANDOM.nextObject(JobInstance.class).withStatus(JobStatus.RUNNING));
            save(RANDOM.nextObject(JobInstance.class).withStatus(JobStatus.FAILED));
            save(RANDOM.nextObject(JobInstance.class).withStatus(JobStatus.SUCCESS));
        });

        List<JobInstance> running = db.currentOrReadOnly(() ->
                db.jobInstance().findRunning());
        assertThat(running).isNotEmpty();
        assertThat(running).extracting(JobInstance::getStatus)
                .extracting(JobStatus::isFinished)
                .containsOnly(false);
    }

    @Test
    public void findInstances() {
        FlowLaunchId blueLaunch = RANDOM.nextObject(FlowLaunchId.class);
        FlowLaunchId grayLaunch = RANDOM.nextObject(FlowLaunchId.class);

        List<JobInstance> blueInstances = RANDOM.objects(JobInstance.class, 10)
                .map(inst -> updateFlowLaunchId(inst, blueLaunch))
                .collect(Collectors.toList());

        List<JobInstance> grayInstances = RANDOM.objects(JobInstance.class, 10)
                .map(inst -> updateFlowLaunchId(inst, grayLaunch))
                .collect(Collectors.toList());

        db.currentOrTx(() -> blueInstances.forEach(this::save));

        JobInstance blueInstance = db.currentOrReadOnly(() ->
                db.jobInstance().get(blueInstances.get(0).getId()));
        assertThat(blueInstance)
                .isEqualTo(blueInstances.get(0));

        JobInstance grayInstance = db.currentOrReadOnly(() ->
                db.jobInstance().find(grayInstances.get(0).getId()).orElse(null));
        //noinspection ConstantConditions
        assertThat(grayInstance)
                .isNull();

    }

    private JobInstance updateFlowLaunchId(JobInstance instance, FlowLaunchId newFlowLaunchId) {
        var id = instance.getId();
        return instance.toBuilder()
                .id(JobInstance.Id.of(newFlowLaunchId.asString(), id.getJobId(), id.getNumber()))
                .build();
    }

    private Optional<JobInstance> findOptional(JobInstance.Id key) {
        return db.currentOrReadOnly(() -> db.jobInstance().find(key));
    }

    private void saveAll(List<JobInstance> jobInstances) {
        db.currentOrTx(() -> db.jobInstance().save(jobInstances));
    }

    private void save(JobInstance jobInstance) {
        db.currentOrTx(() -> db.jobInstance().save(jobInstance));
    }
}
