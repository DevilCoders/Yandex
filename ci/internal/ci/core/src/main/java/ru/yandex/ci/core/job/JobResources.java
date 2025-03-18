package ru.yandex.ci.core.job;

import java.util.List;
import java.util.Map;
import java.util.function.Supplier;

import javax.annotation.Nonnull;

import com.google.gson.JsonObject;
import lombok.Value;
import lombok.With;

/**
 * Store everything we need to pass input parameters into Job Launch
 */
@Value(staticConstructor = "of")
public class JobResources {
    // Resources to be passed into job input
    @With
    @Nonnull
    List<JobResource> resources;

    // Source for all resources from all upstream tasks.
    // This operation is not cheap; load only if required, i.e. need "resolution" (we have "${tasks.*} expression}
    @Nonnull
    Supplier<Map<String, JsonObject>> upstreamResources;

    public static JobResources of(List<JobResource> resources) {
        return new JobResources(resources, Map::of);
    }

    public static JobResources of(JobResource... resources) {
        return JobResources.of(List.of(resources));
    }
}
