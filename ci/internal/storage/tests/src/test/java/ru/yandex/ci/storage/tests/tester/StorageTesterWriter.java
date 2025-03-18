package ru.yandex.ci.storage.tests.tester;

import java.time.Instant;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;

public class StorageTesterWriter {
    private final StorageTester storageTester;
    private final @Nullable
    StorageTesterRegistrar.RegistrationResult registration;

    private Set<CheckTaskEntity.Id> taskIds;
    private int partition;

    public StorageTesterWriter(
            StorageTester storageTester, @Nullable StorageTesterRegistrar.RegistrationResult registration
    ) {
        this.storageTester = storageTester;
        this.registration = registration;
    }

    public StorageTesterWriter partition(int partition) {
        this.partition = partition;
        return this;
    }

    public StorageTesterWriter toAll() {
        Preconditions.checkNotNull(registration, "Registration required for this method");
        this.taskIds = registration.getTasks().stream().map(CheckTaskEntity::getId).collect(Collectors.toSet());
        return this;
    }

    public StorageTesterWriter to(String taskId) {
        Preconditions.checkNotNull(registration, "Registration required for this method");
        this.taskIds = Set.of(registration.getTask(taskId).getId());
        return this;
    }

    public StorageTesterWriter to(String... taskIds) {
        Preconditions.checkNotNull(registration, "Registration required for this method");
        this.taskIds = Arrays.stream(taskIds)
                .map(registration::getTask)
                .map(CheckTaskEntity::getId)
                .collect(Collectors.toSet());
        return this;
    }

    public StorageTesterWriter to(CheckTaskEntity.Id taskId) {
        this.taskIds = Set.of(taskId);
        return this;
    }

    public StorageTesterWriter to(Set<CheckTaskEntity.Id> taskIds) {
        this.taskIds = Set.copyOf(taskIds);
        return this;
    }

    public StorageTesterWriter to(CheckTaskEntity... tasks) {
        this.taskIds = Arrays.stream(tasks).map(CheckTaskEntity::getId).collect(Collectors.toSet());
        return this;
    }

    public StorageTesterWriter to(Collection<CheckTaskEntity> tasks) {
        this.taskIds = tasks.stream().map(CheckTaskEntity::getId).collect(Collectors.toSet());
        return this;
    }

    public StorageTesterWriter to(CheckTaskEntity task) {
        this.taskIds = Set.of(task.getId());
        return this;
    }

    public StorageTesterWriter trace(String trace) {
        var messages = this.taskIds.stream()
                .map(
                        taskId -> TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                                .setPartition(this.partition)
                                .setTraceStage(
                                        Common.TraceStage.newBuilder()
                                                .setType(trace)
                                                .build()
                                )
                                .build()
                ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter pessimize(String description) {
        var messages = this.taskIds.stream()
                .map(
                        taskId -> TaskMessages.TaskMessage.newBuilder()
                                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                                .setPartition(this.partition)
                                .setPessimize(
                                        Actions.Pessimize.newBuilder()
                                                .setTimestamp(ProtoConverter.convert(Instant.now()))
                                                .setInfo(description)
                                                .build()
                                )
                                .build()
                ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter results(Collection<TaskMessages.AutocheckTestResult> results) {
        return this.results(results.toArray(TaskMessages.AutocheckTestResult[]::new));
    }

    public StorageTesterWriter results(TaskMessages.AutocheckTestResult... results) {
        var messages = this.taskIds.stream()
                .map(taskId -> TaskMessages.TaskMessage.newBuilder()
                        .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                        .setPartition(this.partition)
                        .setAutocheckTestResults(
                                TaskMessages.AutocheckTestResults.newBuilder()
                                        .addAllResults(List.of(results))
                                        .build()
                        )
                        .build()
                ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter metric(Common.Metric metric) {
        var messages = this.taskIds.stream()
                .map(taskId -> TaskMessages.TaskMessage.newBuilder()
                        .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                        .setPartition(this.partition)
                        .setMetric(metric)
                        .build()
                ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter testTypeFinish(Actions.TestType testType) {
        var messages = taskIds.stream().map(taskId -> TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition)
                .setTestTypeFinished(
                        Actions.TestTypeFinished.newBuilder()
                                .setTimestamp(ProtoConverter.convert(Instant.now()))
                                .setTestType(testType)
                )
                .build()
        ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter testTypeSizeFinish(Actions.TestTypeSizeFinished.Size size) {
        var messages = taskIds.stream().map(taskId -> TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition)
                .setTestTypeSizeFinished(
                        Actions.TestTypeSizeFinished.newBuilder()
                                .setTimestamp(ProtoConverter.convert(Instant.now()))
                                .setTestType(Actions.TestType.TEST)
                                .setSize(size)
                )
                .build()
        ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter allTestTypeFinish() {

        for (var testType : Actions.TestType.values()) {
            if (testType == Actions.TestType.UNRECOGNIZED) {
                continue;
            }

            testTypeFinish(testType);
        }

        return this;
    }

    public StorageTesterWriter pureFinish() {
        var messages = taskIds.stream().map(taskId -> TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition)
                .setFinished(
                        Actions.Finished.newBuilder()
                                .setTimestamp(ProtoConverter.convert(Instant.now()))
                                .build()
                )
                .build()
        ).collect(Collectors.toList());

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter finish() {
        allTestTypeFinish();
        pureFinish();
        return this;
    }

    public StorageTesterWriter autocheckFatalError(Actions.AutocheckFatalError fatalError) {
        var messages = taskIds.stream().map(taskId -> TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition)
                .setAutocheckFatalError(fatalError)
                .build()
        ).toList();

        this.storageTester.writeMainStreamMessages(messages);

        return this;
    }

    public StorageTesterWriter toLeft() {
        Preconditions.checkNotNull(registration, "Registration required for this method");

        this.taskIds = this.registration.getTasks().stream()
                .filter(CheckTaskEntity::isLeft)
                .map(CheckTaskEntity::getId)
                .collect(Collectors.toSet());
        return this;
    }

    public StorageTesterWriter toRight() {
        Preconditions.checkNotNull(registration, "Registration required for this method");

        this.taskIds = this.registration.getTasks().stream()
                .filter(CheckTaskEntity::isRight)
                .map(CheckTaskEntity::getId)
                .collect(Collectors.toSet());

        return this;
    }
}
