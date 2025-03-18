package ru.yandex.ci.storage.api.proxy;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.core.logbroker.LogbrokerCredentialsProvider;
import ru.yandex.ci.core.logbroker.LogbrokerProxyBalancerHolder;
import ru.yandex.ci.storage.core.CheckOuterClass;
import ru.yandex.ci.storage.core.MainStreamMessages;
import ru.yandex.ci.storage.core.logbroker.LogbrokerTopic;
import ru.yandex.ci.storage.core.logbroker.LogbrokerTopics;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactoryImpl;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;
import ru.yandex.ci.storage.core.message.InternalPartitionGenerator;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.kikimr.persqueue.LogbrokerClientFactory;

public class StorageMessageProxyWriterImpl extends StorageMessageWriter implements StorageMessageProxyWriter {

    private final InternalPartitionGenerator partitionGenerator;

    private StorageMessageProxyWriterImpl(
            LogbrokerTopic topic,
            MeterRegistry meterRegistry,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, topic.getNumberOfPartitions(), logbrokerWriterFactory);
        this.partitionGenerator = new InternalPartitionGenerator(topic.getNumberOfPartitions());
    }

    @Override
    public void writeTasks(List<MainStreamMessages.MainStreamMessage> messages) {
        var messagesToWrite = new ArrayList<StorageMessageWriter.Message>(messages.size());
        for (var message : messages) {
            var taskId = CheckProtoMappers.toTaskId(message.getTaskMessage().getFullTaskId());
            var partition = partitionGenerator.generatePartition(taskId.getIterationId().getCheckId());
            messagesToWrite.add(new StorageMessageWriter.Message(
                    message.getMeta(),
                    partition,
                    message,
                    "Task message: " + taskId
            ));
        }
        this.writeWithRetries(messagesToWrite);
    }

    public static Map<CheckOuterClass.CheckType, StorageMessageProxyWriter> writersFor(
            LogbrokerProxyBalancerHolder proxyHolder,
            LogbrokerCredentialsProvider credentialsProvider,
            MeterRegistry meterRegistry
    ) {
        return Stream.of(
                        CheckOuterClass.CheckType.TRUNK_PRE_COMMIT,
                        CheckOuterClass.CheckType.TRUNK_POST_COMMIT,
                        CheckOuterClass.CheckType.BRANCH_PRE_COMMIT,
                        CheckOuterClass.CheckType.BRANCH_POST_COMMIT
                )
                .map(LogbrokerTopics::get)
                .collect(Collectors.toMap(
                        LogbrokerTopic::getType,
                        topic -> new StorageMessageProxyWriterImpl(
                                topic, meterRegistry,
                                new LogbrokerWriterFactoryImpl(
                                        topic.getPath(),
                                        topic.getType() + "-proxy",
                                        new LogbrokerClientFactory(proxyHolder.getProxyBalancer()),
                                        credentialsProvider
                                )
                        )));
    }
}
