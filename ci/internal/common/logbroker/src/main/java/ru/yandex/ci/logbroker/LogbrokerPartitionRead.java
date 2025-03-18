package ru.yandex.ci.logbroker;

import java.util.List;

import lombok.Value;

import ru.yandex.kikimr.persqueue.consumer.transport.message.inbound.data.MessageBatch;

@Value
public class LogbrokerPartitionRead {
    int partition;
    LbCommitCountdown commitCountdown;
    List<MessageBatch> batches;
    int numberOfMessages;

    public LogbrokerPartitionRead(int partition, LbCommitCountdown commitCountdown, List<MessageBatch> batches) {
        this.partition = partition;
        this.commitCountdown = commitCountdown;
        this.batches = batches;
        // only part of messages from commit countdown
        this.numberOfMessages = batches.stream().mapToInt(x -> x.getMessageData().size()).sum();
    }

    public void notifyProcessed() {
        commitCountdown.notifyMessageProcessed(numberOfMessages);
    }
}
