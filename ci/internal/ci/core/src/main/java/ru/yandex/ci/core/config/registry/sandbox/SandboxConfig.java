package ru.yandex.ci.core.config.registry.sandbox;

import java.util.List;
import java.util.function.Function;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AccessLevel;
import lombok.AllArgsConstructor;
import lombok.EqualsAndHashCode;
import lombok.Getter;
import lombok.ToString;
import lombok.With;

import ru.yandex.ci.client.sandbox.api.TaskRequirementsDns;
import ru.yandex.ci.util.NestedBuilder;
import ru.yandex.ci.util.Overridable;
import ru.yandex.ci.util.Overrider;
import ru.yandex.ci.ydb.Persisted;

@SuppressWarnings("ReferenceEquality")
@Persisted
@EqualsAndHashCode
@ToString
@Getter
@AllArgsConstructor(access = AccessLevel.PRIVATE)
public class SandboxConfig implements Overridable<SandboxConfig> {

    @JsonProperty("tasks_resource")
    @JsonAlias("tasksResource")
    private String tasksResource;

    @JsonProperty("client_tags")
    @JsonAlias("clientTags")
    private String clientTags;

    @JsonProperty("container_resource")
    @JsonAlias("containerResource")
    private String containerResource;

    @JsonProperty("porto_layers")
    @JsonAlias("portoLayers")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    private List<Long> portoLayers;

    @With
    @JsonProperty("semaphores")
    private SandboxSemaphoresConfig semaphores;

    @JsonProperty("cpu_model")
    @JsonAlias("cpuModel")
    private String cpuModel;

    @JsonProperty
    private Dns dns;

    @JsonProperty
    private String host;

    @JsonProperty
    private String platform;

    @JsonProperty
    private Boolean privileged;

    @JsonProperty("tcpdump_args")
    @JsonAlias("tcpdumpArgs")
    private String tcpdumpArgs;

    // This field has no place in here, keep it until refactoring (see RuntimeSandboxConfig)
    @Deprecated
    @JsonProperty
    private TaskPriority priority;

    private SandboxConfig() {
    }

    private SandboxConfig(Builder<?> builder) {
        this.clientTags = builder.clientTags;
        this.containerResource = builder.containerResource;
        this.tasksResource = builder.tasksResource;
        this.portoLayers = builder.portoLayers;
        this.semaphores = builder.semaphores;
        this.cpuModel = builder.cpuModel;
        this.dns = builder.dns;
        this.host = builder.host;
        this.platform = builder.platform;
        this.privileged = builder.privileged;
        this.priority = builder.priority;
        this.tcpdumpArgs = builder.tcpdumpArgs;
    }

    public static Builder<?> builder() {
        return new Builder<>(Function.identity());
    }

    @Override
    public SandboxConfig override(SandboxConfig overrides) {
        Builder<?> builder = builder();
        Overrider<SandboxConfig> overrider = new Overrider<>(this, overrides);
        overrider.field(builder::clientTags, SandboxConfig::getClientTags);
        overrider.field(builder::containerResource, SandboxConfig::getContainerResource);
        overrider.field(builder::tasksResource, SandboxConfig::getTasksResource);
        overrider.field(builder::portoLayers, SandboxConfig::getPortoLayers);
        overrider.field(builder::semaphores, SandboxConfig::getSemaphores);
        overrider.field(builder::cpuModel, SandboxConfig::getCpuModel);
        overrider.field(builder::dns, SandboxConfig::getDns);
        overrider.field(builder::host, SandboxConfig::getHost);
        overrider.field(builder::platform, SandboxConfig::getPlatform);
        overrider.field(builder::privileged, SandboxConfig::getPrivileged);
        overrider.field(builder::priority, SandboxConfig::getPriority);
        overrider.field(builder::setTcpdumpArgs, SandboxConfig::getTcpdumpArgs);
        return builder.build();
    }

    @Persisted
    public enum Dns {
        @JsonProperty("default")
        @JsonAlias("DEFAULT")
        DEFAULT(TaskRequirementsDns.DEFAULT),
        @JsonProperty("local")
        @JsonAlias("LOCAL")
        LOCAL(TaskRequirementsDns.LOCAL),
        @JsonProperty("dns64")
        @JsonAlias("DNS64")
        DNS64(TaskRequirementsDns.DNS64);

        private final TaskRequirementsDns requestValue;

        Dns(TaskRequirementsDns requestValue) {
            this.requestValue = requestValue;
        }

        public TaskRequirementsDns getRequestValue() {
            return requestValue;
        }
    }

    public static class Builder<Parent> extends NestedBuilder<Parent, SandboxConfig> {
        private String clientTags;
        private String containerResource;
        private List<Long> portoLayers;
        private SandboxSemaphoresConfig semaphores;
        private String tasksResource;
        private String cpuModel;
        private Dns dns;
        private String host;
        private String platform;
        private Boolean privileged;
        private TaskPriority priority;
        private String tcpdumpArgs;

        public Builder(Function<SandboxConfig, Parent> toParent) {
            super(toParent);
        }

        public Builder<Parent> clientTags(String tags) {
            this.clientTags = tags;
            return this;
        }

        public Builder<Parent> tasksResource(String tasksResource) {
            this.tasksResource = tasksResource;
            return this;
        }

        public Builder<Parent> containerResource(String containerResource) {
            this.containerResource = containerResource;
            return this;
        }

        public Builder<Parent> portoLayers(List<Long> portoLayers) {
            this.portoLayers = portoLayers;
            return this;
        }

        public Builder<Parent> semaphores(SandboxSemaphoresConfig semaphores) {
            this.semaphores = semaphores;
            return this;
        }

        public Builder<Parent> cpuModel(String cpuModel) {
            this.cpuModel = cpuModel;
            return this;
        }

        public Builder<Parent> dns(Dns dns) {
            this.dns = dns;
            return this;
        }

        public Builder<Parent> host(String host) {
            this.host = host;
            return this;
        }

        public Builder<Parent> platform(String platform) {
            this.platform = platform;
            return this;
        }

        public Builder<Parent> privileged(Boolean privileged) {
            this.privileged = privileged;
            return this;
        }

        public Builder<Parent> priority(TaskPriority priority) {
            this.priority = priority;
            return this;
        }

        public Builder<Parent> setTcpdumpArgs(String tcpdumpArgs) {
            this.tcpdumpArgs = tcpdumpArgs;
            return this;
        }

        @Override
        public SandboxConfig build() {
            return new SandboxConfig(this);
        }
    }
}
