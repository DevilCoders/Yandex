package ru.yandex.ci.client.sandbox.api;

import java.util.List;

import com.fasterxml.jackson.annotation.JsonProperty;
import org.apache.commons.lang3.builder.ReflectionToStringBuilder;
import org.apache.commons.lang3.builder.ToStringStyle;

// todo add other fields
public class SandboxTaskRequirements {

    private String clientTags;
    private Long containerResource;
    private Long cores;
    private String cpuModel;
    private Long diskSpace;
    private TaskRequirementsDns dns;
    private String host;
    private String platform;
    private Boolean privileged;
    private Long ram;
    private TaskRequirementsRamdisk ramdrive;
    private Long tasksResource;
    private List<Long> portoLayers;
    private TaskSemaphores semaphores;
    private String tcpdumpArgs;

    public SandboxTaskRequirements() {
        // no-arg constructor for jackson
    }

    @JsonProperty("client_tags")
    public String getClientTags() {
        return clientTags;
    }

    public SandboxTaskRequirements setClientTags(String clientTags) {
        this.clientTags = clientTags;
        return this;
    }

    @JsonProperty("container_resource")
    public Long getContainerResource() {
        return containerResource;
    }

    public SandboxTaskRequirements setContainerResource(Long containerResource) {
        this.containerResource = containerResource;
        return this;
    }

    public Long getCores() {
        return cores;
    }

    public SandboxTaskRequirements setCores(Long cores) {
        this.cores = cores;
        return this;
    }

    @JsonProperty("cpu_model")
    public String getCpuModel() {
        return cpuModel;
    }

    public SandboxTaskRequirements setCpuModel(String cpuModel) {
        this.cpuModel = cpuModel;
        return this;
    }

    public Long getDiskSpace() {
        return diskSpace;
    }

    @JsonProperty("disk_space")
    public SandboxTaskRequirements setDiskSpace(Long diskSpace) {
        this.diskSpace = diskSpace;
        return this;
    }

    @JsonProperty("dns")
    public TaskRequirementsDns getDns() {
        return dns;
    }

    public SandboxTaskRequirements setDns(TaskRequirementsDns dns) {
        this.dns = dns;
        return this;
    }

    public String getHost() {
        return host;
    }

    public SandboxTaskRequirements setHost(String host) {
        this.host = host;
        return this;
    }

    public String getPlatform() {
        return platform;
    }

    public SandboxTaskRequirements setPlatform(String platform) {
        this.platform = platform;
        return this;
    }

    public Boolean getPrivileged() {
        return privileged;
    }

    public SandboxTaskRequirements setPrivileged(Boolean privileged) {
        this.privileged = privileged;
        return this;
    }

    public Long getRam() {
        return ram;
    }

    public SandboxTaskRequirements setRam(Long ram) {
        this.ram = ram;
        return this;
    }

    @JsonProperty("ramdrive")
    public TaskRequirementsRamdisk getRamdrive() {
        return ramdrive;
    }

    public SandboxTaskRequirements setRamdrive(TaskRequirementsRamdisk ramdrive) {
        this.ramdrive = ramdrive;
        return this;
    }

    @JsonProperty("tasks_resource")
    public Long getTasksResource() {
        return tasksResource;
    }

    public SandboxTaskRequirements setTasksResource(Long tasksResource) {
        this.tasksResource = tasksResource;
        return this;
    }

    @JsonProperty("porto_layers")
    public List<Long> getPortoLayers() {
        return portoLayers;
    }

    public SandboxTaskRequirements setPortoLayers(List<Long> portoLayers) {
        this.portoLayers = portoLayers;
        return this;
    }

    public TaskSemaphores getSemaphores() {
        return semaphores;
    }

    public SandboxTaskRequirements setSemaphores(TaskSemaphores semaphores) {
        this.semaphores = semaphores;
        return this;
    }

    @JsonProperty("tcpdump_args")
    public String getTcpdumpArgs() {
        return tcpdumpArgs;
    }

    public void setTcpdumpArgs(String tcpdumpArgs) {
        this.tcpdumpArgs = tcpdumpArgs;
    }

    @Override
    public String toString() {
        return ReflectionToStringBuilder.toString(this, ToStringStyle.SHORT_PREFIX_STYLE);
    }
}
