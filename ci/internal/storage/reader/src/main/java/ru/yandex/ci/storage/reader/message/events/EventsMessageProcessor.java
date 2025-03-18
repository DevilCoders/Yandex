package ru.yandex.ci.storage.reader.message.events;

import java.util.List;
import java.util.stream.Collectors;

import lombok.AllArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.common.proto.ProtoConverter;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.EventsStreamMessages.EventsStreamMessage.MessagesCase;
import ru.yandex.ci.storage.core.ShardOut;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;
import ru.yandex.ci.storage.core.exceptions.CheckNotFoundException;
import ru.yandex.ci.storage.core.message.shard_out.ShardOutMessageWriter;
import ru.yandex.ci.storage.core.proto.CheckProtoMappers;
import ru.yandex.ci.storage.core.registration.RegistrationProcessor;
import ru.yandex.ci.storage.reader.cache.ReaderCache;
import ru.yandex.ci.storage.reader.message.main.ReaderStatistics;

@Slf4j
@AllArgsConstructor
public class EventsMessageProcessor {

    private final ReaderCache readerCache;

    private final ShardOutMessageWriter writer;

    private final RegistrationProcessor registrationProcessor;

    private final ReaderStatistics statistics;

    public void process(List<EventMessage> messages) {
        var byCase = messages.stream().collect(Collectors.groupingBy(x -> x.getMessage().getMessagesCase()));

        processCancelMessages(byCase.getOrDefault(MessagesCase.CANCEL, List.of()));
        processRegistrationMessages(byCase.getOrDefault(MessagesCase.REGISTRATION, List.of()));

        messages.forEach(message -> message.getCommitCountdown().notifyMessageProcessed());
    }

    protected void processCancelMessages(List<EventMessage> messages) {
        if (messages.isEmpty()) {
            return;
        }

        log.info(
                "Sending forwarding cancel messages to shards: {}",
                messages.stream()
                        .map(x -> CheckProtoMappers.toIterationId(x.getMessage().getCancel().getIterationId()))
                        .map(CheckIterationEntity.Id::toString)
                        .collect(Collectors.joining(","))
        );

        var skipSettings = readerCache.settings().get().getReader().getSkip();
        messages = messages.stream().filter(
                message -> {
                    var checkId = CheckEntity.Id.of(message.getMessage().getCancel().getIterationId().getCheckId());
                    var checkOptional = this.readerCache.checks().get(checkId);

                    if (checkOptional.isEmpty()) {
                        this.statistics.getEvents().onMissingError();

                        if (skipSettings.isMissing()) {
                            log.warn("Skipping missing check: {}", checkId);
                            return false;
                        } else {
                            throw new CheckNotFoundException(checkId.toString());
                        }
                    }

                    return true;
                }
        ).collect(Collectors.toList());

        this.writer.writeForwarding(
                messages.stream().map(message ->
                        ShardOut.ShardForwardingMessage.newBuilder()
                                .setFullTaskId(
                                        CheckTaskOuterClass.FullTaskId.newBuilder()
                                                .setIterationId(message.getMessage().getCancel().getIterationId())
                                                .setTaskId("")
                                                .build()
                                )
                                .setCancel(message.getMessage().getCancel())
                                .build()
                ).collect(Collectors.toList())
        );
    }

    protected void processRegistrationMessages(List<EventMessage> messages) {
        for (var message : messages) {
            this.registrationProcessor.processMessage(
                    message.getMessage().getRegistration(),
                    ProtoConverter.convert(message.getMessage().getMeta().getTimestamp())
            );
        }
    }
}
