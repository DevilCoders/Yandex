package ru.yandex.ci.client.sandbox.api;

import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nonnull;

import lombok.AllArgsConstructor;
import lombok.Builder;
import lombok.Value;

@Value
@Builder(toBuilder = true)
@AllArgsConstructor
public class ResourceInfo {
    long id;
    String type;
    String description;
    ResourceState state;
    Map<String, String> attributes;
    Task task;
    String fileName;
    Http http;
    Mds mds;
    Long size;
    String arch;

    @Nonnull
    public Map<String, String> getAttributes() {
        return Objects.requireNonNullElse(attributes, Map.of());
    }

    @Value
    public static class Task {
        long id;
        SandboxTaskStatus status;
        String url;
    }

    @Value
    public static class Mds {
        String url;
    }

    @Value
    public static class Http {
        String proxy;
        List<String> links;
    }
}
