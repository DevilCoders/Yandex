package ru.yandex.ci.storage.tests.tester;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import lombok.Value;

import ru.yandex.ci.storage.api.StorageApi;
import ru.yandex.ci.storage.core.CheckIteration;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.db.model.common.StorageRevision;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.StorageTestsYdbTestBase;
import ru.yandex.ci.storage.tests.api.tester.StorageApiTester;

public class StorageTesterRegistrar {
    private final StorageApiTester storageTester;

    @Nullable
    private CheckEntity check;
    @Nullable
    private CheckIterationEntity iteration;

    private final List<CheckIterationEntity> iterations;
    private final List<CheckTaskEntity> tasks;

    public StorageTesterRegistrar(StorageApiTester storageTester) {
        this.storageTester = storageTester;
        this.iterations = new ArrayList<>();
        this.tasks = new ArrayList<>();
    }

    public StorageTesterRegistrar(StorageApiTester storageTester, RegistrationResult registration) {
        this.storageTester = storageTester;

        this.check = registration.getCheck();
        this.iterations = new ArrayList<>();
        this.tasks = new ArrayList<>();
    }

    public StorageTesterRegistrar check() {
        return check(check -> {
        });
    }

    public StorageTesterRegistrar check(StorageRevision left, StorageRevision right) {
        return check(
                check -> check
                        .setLeftRevision(CheckProtoMappers.toProtoRevision(left))
                        .setRightRevision(CheckProtoMappers.toProtoRevision(right))
        );
    }

    public StorageTesterRegistrar check(Consumer<StorageApi.RegisterCheckRequest.Builder> updateCheck) {
        Preconditions.checkState(check == null);

        this.check = CheckProtoMappers.toCheck(storageTester.registerCheck(updateCheck).getCheck());
        return this;
    }

    public StorageTesterRegistrar fullIteration() {
        return iteration(CheckIteration.IterationType.FULL);
    }

    public StorageTesterRegistrar fastIteration() {
        return iteration(CheckIteration.IterationType.FAST);
    }

    public StorageTesterRegistrar heavyIteration() {
        return iteration(CheckIteration.IterationType.HEAVY);
    }

    public StorageTesterRegistrar iteration(CheckIteration.IterationType iterationType) {
        return iteration(
                iterationType,
                0,
                iterationType == CheckIteration.IterationType.HEAVY
                        ? Common.CheckTaskType.CTT_LARGE_TEST
                        : Common.CheckTaskType.CTT_AUTOCHECK
        );
    }

    public StorageTesterRegistrar testenvIteration() {
        return iteration(CheckIteration.IterationType.HEAVY, 1, Common.CheckTaskType.CTT_TESTENV);
    }

    public StorageTesterRegistrar iteration(
            CheckIteration.IterationType iterationType, int number, Common.CheckTaskType tasksType
    ) {
        return iteration(iterationType, number, tasksType, Set.of());
    }

    public StorageTesterRegistrar iteration(
            CheckIteration.IterationType iterationType, int number, Common.CheckTaskType tasksType,
            Set<CheckIteration.ExpectedTask> expectedTasks
    ) {
        if (check == null) {
            this.check();
        }

        this.iteration = CheckProtoMappers.toCheckIteration(
                storageTester.registerIteration(
                        check,
                        iterationType,
                        number,
                        request -> request
                                .setTasksType(tasksType)
                                .setInfo(
                                        CheckIteration.IterationInfo.newBuilder()
                                                .setFlowProcessId(
                                                        ru.yandex.ci.api.proto.Common.FlowProcessId.newBuilder()
                                                                .setDir("dir")
                                                                .setId("id")
                                                                .build()
                                                )
                                                .setFlowLaunchNumber(1)
                                                .build()
                                )
                                .addAllExpectedTasks(expectedTasks)
                )
        );
        iterations.add(this.iteration);
        return this;
    }

    public StorageTesterRegistrar leftTask() {
        return task(StorageTestsYdbTestBase.TASK_ID_LEFT, false);
    }

    public StorageTesterRegistrar leftTask(String id) {
        return task(id, false);
    }

    public StorageTesterRegistrar leftTask(Common.CheckTaskType type) {
        return task(StorageTestsYdbTestBase.TASK_ID_LEFT, 1, false, type);
    }

    public StorageTesterRegistrar rightTask() {
        return task(StorageTestsYdbTestBase.TASK_ID_RIGHT, true);
    }

    public StorageTesterRegistrar rightTask(Common.CheckTaskType type) {
        return task(StorageTestsYdbTestBase.TASK_ID_RIGHT, 1, true, type);
    }

    public StorageTesterRegistrar rightTask(String id) {
        return task(id, true);
    }

    public StorageTesterRegistrar task(String id, boolean right) {
        Preconditions.checkNotNull(this.iteration);

        var task = storageTester.registerTask(
                CheckProtoMappers.toProtoIterationId(this.iteration.getId()),
                id,
                1,
                right,
                this.iteration.getTasksType()
        );
        this.tasks.add(CheckProtoMappers.toCheckTask(task));
        return this;
    }

    public StorageTesterRegistrar task(
            String id, int numberOfPartitions, boolean right, Common.CheckTaskType taskType
    ) {
        Preconditions.checkNotNull(this.iteration);

        var task = storageTester.registerTask(
                CheckProtoMappers.toProtoIterationId(iteration.getId()), id, numberOfPartitions, right, taskType
        );
        this.tasks.add(CheckProtoMappers.toCheckTask(task));
        return this;
    }

    public RegistrationResult complete() {
        return new RegistrationResult(
                check,
                new ArrayList<>(iterations),
                tasks.stream().collect(Collectors.toMap(task -> task.getId().getTaskId(), Function.identity()))
        );
    }

    @Value
    public static class RegistrationResult {
        CheckEntity check;
        List<CheckIterationEntity> iterations;
        Map<String, CheckTaskEntity> tasksMap;

        public CheckTaskEntity getFirstLeft() {
            return this.tasksMap.values().stream().filter(x -> !x.isRight()).findFirst().orElseThrow();
        }

        public CheckTaskEntity getFirstRight() {
            return this.tasksMap.values().stream().filter(CheckTaskEntity::isRight).findFirst().orElseThrow();
        }

        public CheckTaskEntity getTask(String id) {
            return tasksMap.get(id);
        }

        public CheckIterationEntity getIteration() {
            return iterations.get(0);
        }

        public Collection<CheckTaskEntity> getTasks() {
            return tasksMap.values();
        }
    }
}
