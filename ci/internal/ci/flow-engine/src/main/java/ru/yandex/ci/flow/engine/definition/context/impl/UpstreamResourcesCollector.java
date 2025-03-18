package ru.yandex.ci.flow.engine.definition.context.impl;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.gson.JsonArray;
import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;

import ru.yandex.ci.core.job.JobInstance;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.runtime.ExecutorType;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.ResourceRefContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobLaunch;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.engine.runtime.state.model.UpstreamLaunch;

@RequiredArgsConstructor
public class UpstreamResourcesCollector {

    @Nonnull
    private final CiDb db;
    @Nonnull
    private final ResourceService resourceService;

    /**
     * Returns upstream resources you could access in JMES Path expressions
     *
     * @param flowLaunchEntity flow launch
     * @param jobState         current job state
     * @return upstream resources
     */
    public Map<String, JsonObject> collectUpstreamResources(FlowLaunchEntity flowLaunchEntity, JobState jobState) {
        return new CollectorImpl(flowLaunchEntity).collect(jobState);
    }

    /**
     * Return single resource you could access in JMES Path expressions
     *
     * @param flowLaunchEntity flow launch
     * @param jobState         current job state
     * @param jobLaunch        current job launch
     * @return single resource (same format as collectUpstreamResources)
     */
    public JsonObject collectResource(FlowLaunchEntity flowLaunchEntity, JobState jobState, JobLaunch jobLaunch) {
        return new CollectorImpl(flowLaunchEntity).collectResource(jobState, jobLaunch);
    }

    @RequiredArgsConstructor
    private class CollectorImpl {
        private final FlowLaunchEntity flowLaunchEntity;
        private final Map<String, JsonObject> result = new HashMap<>();

        Map<String, JsonObject> collect(JobState jobState) {
            collectUpstreamResources(jobState);
            return result;
        }

        private void collectUpstreamResources(JobState jobState) {
            var jobs = flowLaunchEntity.getJobs();

            // #getUpstreamLaunches contains only direct upstreams
            // Have to traverse through the graph in order to collect resources from all upstreams, including root
            var lastLaunch = Objects.requireNonNull(jobState.getLastLaunch(), "Last launch cannot be null");
            for (UpstreamLaunch upstreamLaunch : lastLaunch.getUpstreamLaunches()) {
                var jobId = upstreamLaunch.getUpstreamJobId();
                if (result.containsKey(jobId)) {
                    continue; // ---
                }

                var job = jobs.get(jobId);
                Preconditions.checkState(job != null, "Unable to find upstream job %s", jobId);

                var upstreamLaunchLaunchNumber = upstreamLaunch.getLaunchNumber();
                var launch = job.getLaunchByNumber(upstreamLaunchLaunchNumber);

                result.put(jobId, collectResource(job, launch));
                collectUpstreamResources(job);
            }
        }

        private JsonObject collectResource(JobState job, JobLaunch launch) {
            // For 'Sandbox'
            // * Collect upstream 'producedResources'
            // * Collect upstream 'output'
            // * Merge into single object (resources = ..., output_params = ...)

            // For 'Tasklet'
            // Ignore upstream resources and load job output instead - use this output as a complete resource

            // For 'Class'
            // Collect upstream 'producedResources'

            var executorType = ExecutorType.selectFor(job.getExecutorContext());
            return switch (executorType) {
                case SANDBOX_TASK -> collectSandboxResource(job, launch);
                case TASKLET, TASKLET_V2 -> loadJobOutput(job, launch);
                case CLASS -> collectClassResource(job, launch);
            };
        }

        private JsonObject collectSandboxResource(JobState job, JobLaunch launch) {
            var ret = new JsonObject();
            var resources = loadResources(job, launch);
            ret.add("resources", collectProducedResourcesAsArray(resources));
            ret.add("output_params", loadJobOutput(job, launch));
            return ret;
        }

        private JsonObject collectClassResource(JobState job, JobLaunch launch) {
            var resources = loadResources(job, launch);
            var ret = collectProducedResourcesAsMap(resources);
            ret.add("resources", collectProducedResourcesAsArray(resources));
            return ret;
        }

        private JsonObject loadJobOutput(JobState job, JobLaunch launch) {
            var jobKey = job.getDelegatedOutputResources() != null
                    ? job.getDelegatedOutputResources().getJobOutput()
                    :
                    JobInstance.Id.of(
                            flowLaunchEntity.getFlowLaunchId().asString(),
                            job.getJobId(),
                            launch.getNumber()
                    );
            return db.currentOrReadOnly(() ->
                    db.jobInstance().find(jobKey)
                            .map(JobInstance::getOutput)
                            .orElseGet(JsonObject::new));
        }

        private List<Resource> loadResources(JobState job, JobLaunch launch) {
            var producedResources = job.getDelegatedOutputResources() != null
                    ? job.getDelegatedOutputResources().getResourceRef()
                    : launch.getProducedResources();
            var container = ResourceRefContainer.of(producedResources.getResources());
            return resourceService.loadResources(container).getAll();
        }

        private JsonArray collectProducedResourcesAsArray(List<Resource> resources) {
            var array = new JsonArray();
            resources.stream()
                    .map(Resource::getData)
                    .forEach(array::add);
            return array;
        }

        private JsonObject collectProducedResourcesAsMap(List<Resource> resources) {
            var object = new JsonObject();
            resources.stream()
                    .filter(resource -> resource.getParentField() != null)
                    .forEach(resource -> object.add(resource.getParentField(), resource.getData()));
            return object;
        }
    }

}
