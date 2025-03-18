package ru.yandex.ci.core.taskletv2;

import java.time.Instant;
import java.util.Map;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.google.protobuf.Descriptors;
import lombok.Builder;
import lombok.ToString;
import lombok.Value;

import yandex.cloud.binding.schema.Column;
import yandex.cloud.binding.schema.Table;
import yandex.cloud.repository.db.Entity;
import yandex.cloud.repository.kikimr.DbType;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.tasklet.DescriptorsParser;
import ru.yandex.ci.core.tasklet.TaskletDescriptor;
import ru.yandex.ci.ydb.Persisted;

@Value
@Builder(toBuilder = true)
@Table(name = "flow/TaskletV2Metadata")
public class TaskletV2Metadata implements Entity<TaskletV2Metadata>, TaskletDescriptor {
    @Nonnull
    Id id;

    @Column(dbType = DbType.TIMESTAMP)
    Instant created;

    @Column
    long revision;

    @Column
    @ToString.Exclude
    byte[] descriptors;

    @ToString.Exclude
    transient Supplier<Map<String, Descriptors.Descriptor>> descriptorsCache =
            DescriptorsParser.getDescriptorsCache(this::getDescriptors);

    @Column
    String inputMessage;

    @Column
    String outputMessage;

    @Override
    public Id getId() {
        return id;
    }

    @Override
    public Map<String, Descriptors.Descriptor> getMessageDescriptors() {
        return descriptorsCache.get();
    }

    @Override
    public JobResourceType getInputType() {
        return JobResourceType.of(inputMessage);
    }

    @Override
    public JobResourceType getOutputType() {
        return JobResourceType.of(outputMessage);
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TaskletV2Metadata> {
        @Column(name = "id")
        @Nonnull
        String id;
    }

    @Persisted
    @Value(staticConstructor = "of")
    public static class Description {
        @Nonnull
        String namespace;

        @Nonnull
        String tasklet;

        @Nonnull
        String label;
    }

}
