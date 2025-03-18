package ru.yandex.ci.core.tasklet;

import java.util.Map;
import java.util.Objects;
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

/**
 * Информация о тасклете, извлекаемая из самого бинаря.
 */
@Value
@Builder(toBuilder = true)
@Table(name = "flow/TaskletMetadata")
public class TaskletMetadata implements Entity<TaskletMetadata>, TaskletDescriptor {
    Id id;

    @Column(dbType = DbType.UTF8)
    String name;

    @Column(dbType = DbType.UTF8)
    String sandboxTask;

    @Column
    @ToString.Exclude
    byte[] descriptors;

    @ToString.Exclude
    transient Supplier<Map<String, Descriptors.Descriptor>> descriptorsCache =
            DescriptorsParser.getDescriptorsCache(this::getDescriptors);

    // TODO возможно, стоит выделить отдельный тип
    //      потому что в данном месте это еще не тип ресурса, а только ссылка на имя корневого месседжа
    //      и его нельзя напрямую использовать как тип ресурса, потому что чаще всего будут вложенные месседжи
    //      с другими JobResourceType
    //      см ru.yandex.ci.core.tasklet.SchemaService.extractSchema
    @Column(name = "inputType", dbType = DbType.UTF8)
    String inputTypeString;

    @Column(name = "outputType", dbType = DbType.UTF8)
    String outputTypeString;

    @Column(name = "features", flatten = false)
    Features features;

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
        return JobResourceType.of(inputTypeString);
    }

    @Override
    public JobResourceType getOutputType() {
        return JobResourceType.of(outputTypeString);
    }

    @Nonnull
    public Features getFeatures() {
        return Objects.requireNonNullElseGet(features, Features::empty);
    }

    public static class Builder {

        Builder inputType(JobResourceType type) {
            return this.inputTypeString(type.getMessageName());
        }

        Builder outputType(JobResourceType type) {
            return this.outputTypeString(type.getMessageName());
        }
    }

    @Value(staticConstructor = "of")
    public static class Id implements Entity.Id<TaskletMetadata> {

        @Column(name = "implementation", dbType = DbType.UTF8)
        @Nonnull
        String implementation;

        // UTF8 - for compatibility reasons
        @Column(name = "runtime", dbType = DbType.UTF8)
        @Nonnull
        TaskletRuntime runtime;

        // Nulls in YDB keys are bad thing - and ORM cannot even save key with null value (will be 0 anyway)
        // 0 means 'no value'
        @Column(name = "sandboxResourceId")
        long sandboxResourceId;
    }


}
