package ru.yandex.ci.common.temporal.ydb;

import java.nio.charset.Charset;

import com.google.common.hash.Hashing;
import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import ru.yandex.ci.common.temporal.BaseTemporalWorkflow;

@Value
@AllArgsConstructor
@Builder
@Table(name = "temporal/LaunchQueue")
public class TemporalLaunchQueueEntity implements Entity<TemporalLaunchQueueEntity> {

    Id id;
    String type;
    String runMethod;
    String payloadType;
    String payload;
    String queue;
    long enqueueTimeSeconds;

    @Override
    public Id getId() {
        return id;
    }

    @Value
    public static class Id implements Entity.Id<TemporalLaunchQueueEntity> {
        long hash; //for uniform distribution
        String id;

        public static <T extends BaseTemporalWorkflow<I>, I extends BaseTemporalWorkflow.Id> Id of(Class<T> type,
                                                                                                   I input,
                                                                                                   String id) {
            return new Id(
                    Hashing.sipHash24().newHasher()
                            .putString(type.getCanonicalName(), Charset.defaultCharset())
                            .putString(input.getTemporalWorkflowId(), Charset.defaultCharset())
                            .hash()
                            .asLong(),
                    id
            );
        }
    }

}
