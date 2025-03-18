package ru.yandex.ci.storage.shard.message.writer;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

import com.google.common.collect.Lists;
import io.micrometer.core.instrument.MeterRegistry;

import ru.yandex.ci.storage.core.PostProcessor;
import ru.yandex.ci.storage.core.db.model.test.TestEntity;
import ru.yandex.ci.storage.core.logbroker.LogbrokerWriterFactory;
import ru.yandex.ci.storage.core.logbroker.StorageMessageWriter;

public class PostProcessorWriterImpl extends StorageMessageWriter implements PostProcessorWriter {

    public PostProcessorWriterImpl(
            int numberOfPartitions,
            MeterRegistry meterRegistry,
            LogbrokerWriterFactory logbrokerWriterFactory
    ) {
        super(meterRegistry, numberOfPartitions, logbrokerWriterFactory);
    }

    @Override
    public void writeResults(List<PostProcessor.ResultForPostProcessor> results) {
        var byPartition = results.stream().collect(Collectors.groupingBy(this::getPartition));

        var messagesToWrite = new ArrayList<Message>();

        for (var partitionGroup : byPartition.entrySet()) {
            var meta = createMeta();
            for (var batch : Lists.partition(partitionGroup.getValue(), 1000)) {
                messagesToWrite.add(new Message(
                        meta,
                        partitionGroup.getKey(),
                        PostProcessor.PostProcessorInMessage.newBuilder()
                                .setMeta(meta)
                                .setTestResults(
                                        PostProcessor.ResultsForPostProcessor.newBuilder()
                                                .addAllResults(batch)
                                                .build()
                                )
                                .build(),
                        "Post processor results"
                ));
            }
        }

        this.writeWithRetries(messagesToWrite);
    }


    private int getPartition(PostProcessor.ResultForPostProcessor x) {
        return TestEntity.Id.getPostProcessorPartition(x.getId().getHid(), this.numberOfPartitions);
    }
}
