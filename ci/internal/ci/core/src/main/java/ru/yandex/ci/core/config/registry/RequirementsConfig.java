package ru.yandex.ci.core.config.registry;

import java.util.function.Function;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;
import lombok.ToString;
import lombok.With;
import org.springframework.util.unit.DataSize;

import ru.yandex.ci.core.config.registry.sandbox.SandboxConfig;
import ru.yandex.ci.util.NestedBuilder;
import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.util.jackson.JacksonDataSizeDeserializer;
import ru.yandex.ci.util.jackson.JacksonDataSizeSerializer;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@ToString
@EqualsAndHashCode
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class RequirementsConfig implements Overridable<RequirementsConfig> {
    @JsonProperty("disk")
    @JsonSerialize(using = JacksonDataSizeSerializer.class)
    @JsonDeserialize(using = JacksonDataSizeDeserializer.class)
    private DataSize disk;

    @JsonProperty("ram")
    @JsonSerialize(using = JacksonDataSizeSerializer.class)
    @JsonDeserialize(using = JacksonDataSizeDeserializer.class)
    private DataSize ram;

    @JsonProperty
    private Integer cores;

    @JsonProperty("tmpfs")
    @JsonAlias("tmp")
    @JsonSerialize(using = JacksonDataSizeSerializer.class)
    @JsonDeserialize(using = JacksonDataSizeDeserializer.class)
    private DataSize tmp;

    @With
    @JsonProperty
    private SandboxConfig sandbox;

    private RequirementsConfig() {
    }

    private RequirementsConfig(Builder<?> builder) {
        this.disk = builder.disk;
        this.ram = builder.ram;
        this.cores = builder.cores;
        this.tmp = builder.tmp;
        this.sandbox = builder.sandbox;
    }

    public static Builder<?> builder() {
        return new Builder<>(Function.identity());
    }

    public DataSize getDisk() {
        return disk;
    }

    public DataSize getRam() {
        return ram;
    }

    public Integer getCores() {
        return cores;
    }

    public DataSize getTmp() {
        return tmp;
    }

    public SandboxConfig getSandbox() {
        return sandbox;
    }

    @Override
    public RequirementsConfig override(RequirementsConfig override) {
        Builder<?> builder = builder();
        Overrider<RequirementsConfig> overrider = new Overrider<>(this, override);
        overrider.field(builder::withCores, RequirementsConfig::getCores);
        overrider.field(builder::withDisk, RequirementsConfig::getDisk);
        overrider.field(builder::withRam, RequirementsConfig::getRam);
        overrider.field(builder::withTmp, RequirementsConfig::getTmp);
        overrider.fieldDeep(builder::withSandbox, RequirementsConfig::getSandbox);
        return builder.build();
    }

    public static class Builder<Parent> extends NestedBuilder<Parent, RequirementsConfig> {
        private DataSize disk;
        private DataSize ram;
        private Integer cores;
        private DataSize tmp;
        private SandboxConfig sandbox;

        public Builder(Function<RequirementsConfig, Parent> toParent) {
            super(toParent);
        }

        public Builder<Parent> withDiskBytes(long diskBytes) {
            this.disk = DataSize.ofBytes(diskBytes);
            return this;
        }

        public Builder<Parent> withDisk(DataSize disk) {
            this.disk = disk;
            return this;
        }

        public Builder<Parent> withRamBytes(long ramBytes) {
            this.ram = DataSize.ofBytes(ramBytes);
            return this;
        }

        public Builder<Parent> withRam(DataSize ram) {
            this.ram = ram;
            return this;
        }

        public Builder<Parent> withCores(Integer cores) {
            this.cores = cores;
            return this;
        }

        public Builder<Parent> withTmpBytes(long tmpBytes) {
            this.tmp = DataSize.ofBytes(tmpBytes);
            return this;
        }

        public Builder<Parent> withTmp(DataSize tmp) {
            this.tmp = tmp;
            return this;
        }

        public Builder<Parent> withSandbox(SandboxConfig sandbox) {
            this.sandbox = sandbox;
            return this;
        }

        public SandboxConfig.Builder<Builder<Parent>> startSandbox() {
            return new SandboxConfig.Builder<>(this::withSandbox);
        }

        @Override
        public RequirementsConfig build() {
            return new RequirementsConfig(this);
        }

    }
}
