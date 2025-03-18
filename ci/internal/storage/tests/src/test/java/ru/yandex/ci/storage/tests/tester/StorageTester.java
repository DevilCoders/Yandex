package ru.yandex.ci.storage.tests.tester;

import java.time.Instant;
import java.util.Arrays;
import java.util.List;
import java.util.UUID;
import java.util.function.Consumer;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import lombok.AllArgsConstructor;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.core.Actions;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.MainStreamMessages;
import ru.yandex.ci.storage.core.TaskMessages;
import ru.yandex.ci.storage.core.bazinga.SingleThreadBazinga;
import ru.yandex.ci.storage.core.db.CiStorageDb;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_id_generator.CheckIdGenerator;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.MainStatistics;
import ru.yandex.ci.storage.core.db.model.check_task.CheckTaskEntity;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.tests.api.tester.CommonPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.RawDataPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.SearchPublicApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontHistoryApiTester;
import ru.yandex.ci.storage.tests.api.tester.StorageFrontTestsApiTester;
import ru.yandex.ci.storage.tests.logbroker.TestLogbrokerService;
import ru.yandex.ci.storage.tests.logbroker.TestsMainStreamWriter;
import ru.yandex.ci.util.Retryable;

import static org.assertj.core.api.Assertions.assertThat;

@AllArgsConstructor
public class StorageTester {

    private final CiStorageDb db;
    private final TestsMainStreamWriter mainStreamWriter;
    private final TestLogbrokerService testLogbrokerService;
    private final StorageApiTester apiTester;
    private final StorageFrontApiTester frontApiTester;
    private final StorageFrontHistoryApiTester historyApiTester;
    private final StorageFrontTestsApiTester testsApiTester;

    private final CommonPublicApiTester commonPublicApiTester;

    private final SearchPublicApiTester searchPublicApiTester;

    private final RawDataPublicApiTester rawDataPublicApiTester;
    private final SingleThreadBazinga bazinga;

    public void initialize() {
        Retryable.disable();
        CheckIdGenerator.fillDb(db, 10);
    }

    public StorageFrontHistoryApiTester historyApi() {
        return historyApiTester;
    }

    public StorageApiTester api() {
        return apiTester;
    }

    public StorageFrontApiTester frontApi() {
        return frontApiTester;
    }

    public StorageFrontTestsApiTester testsApi() {
        return this.testsApiTester;
    }

    public CommonPublicApiTester commonPublicApiTester() {
        return commonPublicApiTester;
    }

    public SearchPublicApiTester searchPublicApiTester() {
        return searchPublicApiTester;
    }

    public RawDataPublicApiTester rawDataPublicApiTester() {
        return rawDataPublicApiTester;
    }

    public StorageTesterRegistrar.RegistrationResult register(Consumer<StorageTesterRegistrar> consumer) {
        var registrar = new StorageTesterRegistrar(apiTester);
        consumer.accept(registrar);
        return registrar.complete();
    }

    public StorageTesterRegistrar.RegistrationResult register(
            StorageTesterRegistrar.RegistrationResult registration,
            Consumer<StorageTesterRegistrar> consumer
    ) {
        var registrar = new StorageTesterRegistrar(apiTester, registration);
        consumer.accept(registrar);
        return registrar.complete();
    }

    public TaskMessages.TaskMessage createTestTypeFinishedMessage(
            CheckTaskEntity.Id taskId,
            int partition,
            Actions.TestType testType,
            @Nullable Actions.TestTypeSizeFinished.Size testSize
    ) {
        var message = TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition);

        if (testSize == null) {
            message.setTestTypeFinished(
                    Actions.TestTypeFinished.newBuilder()
                            .setTestType(testType)
                            .build()
            );
        } else {
            message.setTestTypeSizeFinished(
                    Actions.TestTypeSizeFinished.newBuilder()
                            .setTestType(testType)
                            .setSize(testSize)
                            .build()
            );
        }
        return message.build();
    }

    // Sends all messages in one main stream message, useful for testing simultaneous processing of different messages.
    public void writeMainStreamMessages(TaskMessages.TaskMessage... messages) {
        writeMainStreamMessages(List.of(messages));
    }

    public void writeMainStreamMessages(List<TaskMessages.TaskMessage> listOfMessages) {
        mainStreamWriter.write(wrapToMainStreamMessage(listOfMessages));
    }

    public CheckEntity getCheck(CheckEntity.Id id) {
        return this.db.currentOrReadOnly(() -> this.db.checks().get(id));
    }

    public CheckIterationEntity getIteration(CheckIterationEntity.Id id) {
        return this.db.currentOrReadOnly(() -> this.db.checkIterations().get(id));
    }

    public void assertIterationStatus(CheckIterationEntity.Id id, Common.CheckStatus status) {
        assertThat(getIteration(id).getStatus()).isEqualTo(status);
    }

    public void assertIterationStatus(CheckIterationEntity iteration, Common.CheckStatus status) {
        assertThat(getIteration(iteration.getId()).getStatus()).isEqualTo(status);
    }

    public void assertIterationStatistics(CheckIterationEntity.Id id, MainStatistics statistics) {
        assertThat(getIteration(id).getStatistics().getAllToolchain().getMain()).isEqualTo(statistics);
    }


    public CheckTaskEntity getTask(CheckTaskEntity.Id id) {
        return this.db.currentOrReadOnly(() -> this.db.checkTasks().get(id));
    }

    public void ensureAllReadsCommited() {
        assertThat(testLogbrokerService.getNotCommitedCookies()).isEmpty();
    }

    private List<MainStreamMessages.MainStreamMessage> wrapToMainStreamMessage(
            List<TaskMessages.TaskMessage> taskMessage
    ) {
        return taskMessage.stream()
                .map(
                        message -> MainStreamMessages.MainStreamMessage.newBuilder()
                                .setTaskMessage(message)
                                .setMeta(createMeta())
                                .build()
                )
                .collect(Collectors.toList());
    }

    private Common.MessageMeta createMeta() {
        return Common.MessageMeta.newBuilder()
                .setId(UUID.randomUUID().toString())
                .setTimestamp(ProtoConverter.convert(Instant.now()))
                .build();
    }

    @Deprecated
    public void writeFinish(CheckTaskEntity.Id... taskIds) {
        writeFinish(0, taskIds);
    }

    @Deprecated
    public void writeFinish(int partition, CheckTaskEntity.Id... taskIds) {
        var taskMessages = Arrays.stream(taskIds).map(taskId -> TaskMessages.TaskMessage.newBuilder()
                .setFullTaskId(CheckProtoMappers.toProtoCheckTaskId(taskId))
                .setPartition(partition)
                .setFinished(
                        Actions.Finished.newBuilder()
                                .setTimestamp(ProtoConverter.convert(Instant.now()))
                                .build()
                )
                .build()
        ).collect(Collectors.toList());

        writeMainStreamMessages(taskMessages);
    }

    public void executeAllOnetimeTasks() {
        bazinga.executeAll();
    }


    public void write(Consumer<StorageTesterWriter> consumer) {
        write(null, consumer);
    }

    public void write(
            @Nullable StorageTesterRegistrar.RegistrationResult registration,
            Consumer<StorageTesterWriter> consumer
    ) {
        consumer.accept(new StorageTesterWriter(this, registration));
    }

    public void writeAndDeliver(Consumer<StorageTesterWriter> consumer) {
        writeAndDeliver(null, consumer);
    }

    public void writeAndDeliver(
            @Nullable StorageTesterRegistrar.RegistrationResult registration,
            Consumer<StorageTesterWriter> consumer
    ) {
        write(registration, consumer);
        testLogbrokerService.deliverAllMessages();
    }
}
