package ru.yandex.ci.observer.reader.proto;

import java.util.Set;
import java.util.stream.Collectors;

import com.google.protobuf.Timestamp;

import ru.yandex.ci.observer.core.Internal;
import ru.yandex.ci.observer.core.db.model.check_iterations.CheckIterationEntity;
import ru.yandex.ci.observer.core.db.model.check_tasks.CheckTaskEntity;
import ru.yandex.ci.storage.core.CheckTaskOuterClass;
import ru.yandex.ci.storage.core.Common;
import ru.yandex.ci.storage.core.EventsStreamMessages;
import ru.yandex.ci.storage.core.db.model.check_iteration.TechnicalStatistics;

public class ObserverInternalMessagesProtoMappers {
    private ObserverInternalMessagesProtoMappers() {
    }

    public static Internal.AggregateStages toProtoAggregateStages(
            CheckIterationEntity.Id iterationId,
            Set<CheckTaskEntity.Id> taskIds
    ) {
        return Internal.AggregateStages.newBuilder()
                .addAllTaskIds(
                        taskIds.stream()
                                .map(ObserverProtoMappers::toProtoTaskId)
                                .collect(Collectors.toList())
                )
                .setIterationId(ObserverProtoMappers.toProtoIterationId(iterationId))
                .build();
    }

    public static Internal.Cancel toProtoCancel(CheckIterationEntity.Id iterationId) {
        return Internal.Cancel.newBuilder()
                .setIterationId(ObserverProtoMappers.toProtoIterationId(iterationId))
                .build();
    }

    public static Internal.FinishPartition toProtoFinishPartition(
            CheckTaskOuterClass.FullTaskId taskId,
            int partition
    ) {
        return Internal.FinishPartition.newBuilder()
                .setTaskId(taskId)
                .setPartition(partition)
                .build();
    }

    public static Internal.Pessimize toProtoPessimize(CheckIterationEntity.Id iterationId) {
        return Internal.Pessimize.newBuilder()
                .setIterationId(ObserverProtoMappers.toProtoIterationId(iterationId))
                .build();
    }

    public static Internal.TechnicalStatisticsMessage toProtoTechnicalStatisticsMessage(
            CheckTaskOuterClass.FullTaskId taskId,
            int partition,
            TechnicalStatistics technicalStatistics
    ) {
        return Internal.TechnicalStatisticsMessage.newBuilder()
                .setTaskId(taskId)
                .setPartition(partition)
                .setTechnicalStatistics(toProtoTechnicalStatistics(technicalStatistics))
                .build();
    }

    public static Internal.Registered toProtoRegistered(EventsStreamMessages.RegistrationMessage message) {
        var registeredBuilder = Internal.Registered.newBuilder();
        if (message.hasIteration()) {
            registeredBuilder.setIterationId(message.getIteration().getId());
        }
        if (message.hasTask()) {
            registeredBuilder.setTaskId(message.getTask().getId());
        }

        return registeredBuilder.build();
    }

    public static Internal.FatalError toProtoFatalError(CheckTaskOuterClass.FullTaskId taskId) {
        return Internal.FatalError.newBuilder()
                .setTaskId(taskId)
                .build();
    }

    public static Internal.IterationFinish toProtoIterationFinish(
            CheckIterationEntity.Id iterationId,
            Common.CheckStatus status,
            Timestamp finishTimestamp
    ) {
        return Internal.IterationFinish.newBuilder()
                .setIterationId(ObserverProtoMappers.toProtoIterationId(iterationId))
                .setStatus(status)
                .setFinishTimestamp(finishTimestamp)
                .build();
    }

    public static Internal.TechnicalStatistics toProtoTechnicalStatistics(TechnicalStatistics statistics) {
        return Internal.TechnicalStatistics.newBuilder()
                .setMachineHours(statistics.getMachineHours())
                .setCacheHit(statistics.getCacheHit())
                .setNumberOfNodes(statistics.getTotalNumberOfNodes())
                .build();
    }
}
