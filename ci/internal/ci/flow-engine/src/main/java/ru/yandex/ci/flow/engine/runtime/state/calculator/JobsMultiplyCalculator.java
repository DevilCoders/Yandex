package ru.yandex.ci.flow.engine.runtime.state.calculator;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.google.common.base.Preconditions;
import com.google.common.base.Suppliers;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import org.apache.commons.lang3.StringUtils;

import ru.yandex.ci.core.config.a.model.JobMultiplyConfig;
import ru.yandex.ci.core.job.JobResourceType;
import ru.yandex.ci.core.resolver.DocumentSource;
import ru.yandex.ci.core.resolver.PropertiesSubstitutor;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.flow.engine.definition.common.ManualTriggerPrompt;
import ru.yandex.ci.flow.engine.definition.common.UpstreamLink;
import ru.yandex.ci.flow.engine.definition.common.UpstreamStyle;
import ru.yandex.ci.flow.engine.definition.common.UpstreamType;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.runtime.ExecutorType;
import ru.yandex.ci.flow.engine.runtime.JobContextFactory;
import ru.yandex.ci.flow.engine.runtime.TaskletContextProcessor;
import ru.yandex.ci.flow.engine.runtime.di.ResourceEntity;
import ru.yandex.ci.flow.engine.runtime.di.ResourceService;
import ru.yandex.ci.flow.engine.runtime.di.model.AbstractResourceContainer;
import ru.yandex.ci.flow.engine.runtime.state.model.FlowLaunchEntity;
import ru.yandex.ci.flow.engine.runtime.state.model.JobState;
import ru.yandex.ci.flow.utils.FlowLayoutHelper;
import ru.yandex.ci.util.gson.CiGson;

@Slf4j
@RequiredArgsConstructor
public class JobsMultiplyCalculator {
    @Nonnull
    private final TaskletContextProcessor taskletContextProcessor;
    @Nonnull
    private final ResourceService resourceService;

    FlowLaunchEntity multiplyJobs(@Nonnull JobContextFactory jobContextFactory,
                                  @Nonnull FlowLaunchEntity flowLaunchEntity,
                                  @Nonnull String jobId) {

        var jobState = flowLaunchEntity.getJobState(jobId);
        var jobContext = jobContextFactory.createJobContext(flowLaunchEntity, jobState);

        log.info("Processing jobs multiplication for {}", jobId);

        Preconditions.checkState(jobState.getMultiply() != null, "Job %s must have multiplication config", jobId);
        try {
            return new Action(jobContext).multiply();
        } catch (Exception e) {
            throw new MultiplicationRuntimeException(e.getMessage(), e);
        }
    }

    private class Action {
        private final JobContext jobContext;
        private final FlowLaunchEntity flowLaunch;
        private final String jobId;
        private final JobState job;
        private final JobMultiplyConfig multiply;

        private final ExecutorType executorType;

        private final Supplier<AbstractResourceContainer> resources;
        private final Supplier<JobResourceType> taskletType;

        // We cannot save and load from the same table within moving to Cloud Framework
        private final List<Runnable> postProcess = new ArrayList<>();
        private final Map<String, JobState> newJobs = new LinkedHashMap<>();
        private final Set<String> updatedJobs = new HashSet<>();

        private Action(@Nonnull JobContext jobContext) {
            this.jobContext = jobContext;
            this.flowLaunch = jobContext.getFlowLaunch();
            this.job = jobContext.getJobState();
            this.jobId = job.getJobId();
            this.multiply = job.getMultiply();

            this.executorType = ExecutorType.selectFor(job.getExecutorContext());
            this.resources = Suppliers.memoize(() -> resourceService
                    .loadResources(job.getStaticResources()));

            this.taskletType = Suppliers.memoize(() -> {
                String field = multiply.getField();
                Preconditions.checkState(!StringUtils.isEmpty(field),
                        "Tasklet multiply field cannot be empty");
                return JobResourceType.of(field);
            });
        }

        FlowLaunchEntity multiply() {
            log.info("Multiply jobs based on {} using configuration {}", jobId, multiply);

            var byDocumentSource = taskletContextProcessor.getDocumentSource(jobContext);
            JsonArray by = calcBy(multiply.getBy(), byDocumentSource);

            // Generate as many jobs as we have to, based on provided expression
            // All JMESPath expressions will be resolved right away
            var templateDoc = new JsonObject();
            templateDoc.addProperty("job_id", jobId);

            int size = by.size();
            log.info("About to multiply job {} to {} new task(s)", jobId, size);

            var max = multiply.getMaxJobs();
            Preconditions.checkState(size <= max, "Job %s size cannot exceed %s, got %s",
                    jobId, max, size);
            templateDoc.addProperty("size", size);

            // Prevent from multiplying jobs during restart
            var jobJson = CiGson.instance().toJson(job);
            for (int i = 0; i < size; i++) {
                var index = JobMultiplyConfig.index(i);

                var objectElement = by.get(i);
                templateDoc.addProperty("index", index);
                templateDoc.add("by", objectElement);

                var newJobId = JobMultiplyConfig.getId(jobId, index);

                JobState newJob;
                var existingJob = flowLaunch.getJobs().get(newJobId);
                if (existingJob != null) { // Existing job is OK, need to update it though
                    log.info("Found existing job {}", newJobId);
                    if (existingJob.getJobTemplateId() == null) {
                        throw new IllegalStateException(String.format(
                                "Invalid multiply/by job configuration. Found existing job %s " +
                                        "created without template",
                                newJobId));
                    } else if (!Objects.equals(existingJob.getJobTemplateId(), jobId)) {
                        throw new IllegalStateException(String.format(
                                "Invalid multiply/by job configuration. Found existing job %s " +
                                        "from different template. Template must be: %s, found: %s",
                                newJobId, jobId, existingJob.getJobTemplateId()));
                    }
                    newJob = existingJob;
                    updatedJobs.add(newJobId);
                } else {
                    log.info("Registering new job: {}", newJobId);
                    newJob = CiGson.instance().fromJson(jobJson, JobState.class);
                    newJob.setMultiply(null);
                    newJob.clearLaunches();
                    newJob.setJobId(newJobId);
                    newJob.setJobTemplateId(jobId);
                    newJobs.put(newJobId, newJob);
                }
                newJob.setConditionalRunExpression(job.getConditionalRunExpression());

                // Make sure new jobs depends on template
                var upstreams = newJob.getUpstreams();
                upstreams.clear();
                upstreams.add(new UpstreamLink<>(jobId, UpstreamType.ALL_RESOURCES, UpstreamStyle.SPLINE));

                var source = DocumentSource.merge(byDocumentSource, templateDoc);
                if (StringUtils.isEmpty(multiply.getTitle())) {
                    newJob.setTitle(job.getTitle());
                } else {
                    newJob.setTitle(substituteString(multiply.getTitle(), source, "title"));
                }
                if (StringUtils.isEmpty(multiply.getDescription())) {
                    newJob.setDescription(job.getDescription());
                } else {
                    newJob.setDescription(substituteString(multiply.getDescription(), source, "description"));
                }

                var manualTrigger = job.getManualTriggerPrompt();
                if (manualTrigger != null) {
                    if (manualTrigger.getQuestion() != null) {
                        var resolvedQuestion = substituteString(manualTrigger.getQuestion(), source, "prompt");
                        newJob.setManualTriggerPrompt(new ManualTriggerPrompt(resolvedQuestion));
                    }
                }

                var withoutContext = DocumentSource.skipContext(source);

                this.updateRuntimeConfig(newJob, withoutContext);
                this.updateRequirementsConfig(newJob, withoutContext);
                this.resolveResources(newJob, objectElement, withoutContext);

                log.info("Job prepared: {}", newJobId);
            }

            this.closeOldJobs();
            var jobs = this.prepareJobs();
            postProcess.forEach(Runnable::run);

            var entity = flowLaunch.toBuilder()
                    .jobs(jobs)
                    .build();

            FlowLayoutHelper.repositionJobs(entity);

            return entity;
        }

        private void updateRuntimeConfig(JobState newJob, DocumentSource documentSource) {
            var executorContext = newJob.getExecutorContext();

            var runtimeConfig = executorContext.getJobRuntimeConfig();
            if (runtimeConfig == null) {
                return; // ---
            }

            var sandboxConfig = runtimeConfig.getSandbox();

            var priority = sandboxConfig.getPriority();
            if (priority != null && priority.getExpression() != null) {
                priority = priority.withExpression(substituteString(
                        priority.getExpression(), documentSource, "priority"));
            }

            var hints = substituteStrings(sandboxConfig.getHints(), documentSource, "hints");
            var tags = substituteStrings(sandboxConfig.getTags(), documentSource, "tags");

            newJob.setExecutorContext(
                    executorContext.withJobRuntimeConfig(
                            runtimeConfig.withSandbox(
                                    sandboxConfig.toBuilder()
                                            .clearTags()
                                            .tags(tags)
                                            .clearHints()
                                            .hints(hints)
                                            .priority(priority)
                                            .build()
                            )
                    )
            );
        }

        private void updateRequirementsConfig(JobState newJob, DocumentSource documentSource) {
            var executorContext = newJob.getExecutorContext();

            var requirements = executorContext.getRequirements();
            if (requirements == null) {
                return; // ---
            }

            var sandbox = requirements.getSandbox();
            if (sandbox == null) {
                return; // ---
            }

            var semaphores = sandbox.getSemaphores();
            if (semaphores == null || semaphores.getAcquires().isEmpty()) {
                return; // ---
            }

            var acquires = semaphores.getAcquires().stream()
                    .map(acquire -> acquire.withName(substituteString(acquire.getName(), documentSource, "acquire")))
                    .collect(Collectors.toList());

            newJob.setExecutorContext(
                    executorContext.withRequirements(
                            requirements.withSandbox(
                                    sandbox.withSemaphores(
                                            semaphores.withAcquires(acquires)
                                    )
                            )
                    )
            );
        }

        private String substituteString(String value, DocumentSource documentSource, String title) {
            return PropertiesSubstitutor.substituteToString(value, documentSource, title);
        }

        private List<String> substituteStrings(List<String> values, DocumentSource documentSource, String title) {
            return values.stream()
                    .map(value -> substituteString(value, documentSource, title))
                    .toList();
        }

        private Map<String, JobState> prepareJobs() {
            var jobs = new LinkedHashMap<>(flowLaunch.getJobs());
            for (var otherJob : jobs.values()) {
                var upstreams = otherJob.getUpstreams();
                for (var upstream : new HashSet<>(upstreams)) {
                    if (jobId.equals(upstream.getEntity())) {
                        newJobs.keySet().forEach(newJobId -> {
                            var newUpstream = new UpstreamLink<>(newJobId, upstream.getType(), upstream.getStyle());
                            log.info("Add job {} upstream: {}", otherJob.getJobId(), newUpstream);
                            upstreams.add(newUpstream);
                        });
                    }
                }
            }
            jobs.putAll(newJobs);
            return jobs;
        }

        private void closeOldJobs() {
            var removed = "Removed: ";
            var disableExpression = "${`false`}";

            for (JobState otherJob : flowLaunch.getJobs().values()) {
                if (jobId.equals(otherJob.getJobTemplateId())) {
                    var otherJobId = otherJob.getJobId();
                    if (!updatedJobs.contains(otherJobId)
                            && !otherJob.getTitle().startsWith(removed)
                            && !disableExpression.equals(otherJob.getConditionalRunExpression())) {
                        log.info("Obsolete job found after multiply-by operation: {}. Skipping", otherJobId);
                        otherJob.setTitle("Removed: " + otherJob.getTitle());
                        otherJob.setConditionalRunExpression(disableExpression);
                    }
                }
            }
        }

        private void resolveResources(JobState newJob, JsonElement by, DocumentSource source) {
            boolean hasAsField = (executorType == ExecutorType.TASKLET || executorType == ExecutorType.TASKLET_V2)
                    && !StringUtils.isEmpty(multiply.getField());
            List<Resource> sourceRes;
            if (hasAsField && by.isJsonObject()) { // Legacy
                var taskletResourceType = taskletType.get();
                sourceRes = new ArrayList<>(resources.get().getAll());
                sourceRes.add(new Resource(taskletResourceType, by.getAsJsonObject(), null));

                newJob.setSkipUpstreamResources(Set.of(taskletResourceType));
            } else {
                sourceRes = resources.get().getAll();
            }
            var targetRes = JobsMultiplyCalculator.resolveResources(sourceRes, source);
            saveResources(newJob, targetRes);
        }

        private void saveResources(JobState newJob, List<Resource> resources) {
            postProcess.add(() -> {
                log.info("Saving resources for new job: {}", newJob.getJobId());
                var result = resourceService.saveResources(resources, flowLaunch,
                        ResourceEntity.ResourceClass.STATIC);
                newJob.setStaticResources(result.toRefs());
            });
        }
    }

    private static List<Resource> resolveResources(List<Resource> resources, DocumentSource source) {
        return resources.stream()
                .map(res -> {
                    // There is one exception - do not substitute ${context} expressions
                    // Context will be available later, during the new job execution
                    var resolved = PropertiesSubstitutor.substitute(res.getData(), source);
                    Preconditions.checkState(resolved.isJsonObject(),
                            "Expect resolved as JSON object, got %s",
                            resolved);

                    return new Resource(res.getResourceType(), resolved.getAsJsonObject(), res.getParentField());
                })
                .collect(Collectors.toList());

    }

    private static JsonArray calcBy(String by, DocumentSource source) {
        var result = PropertiesSubstitutor.substitute(new JsonPrimitive(by), source);
        if (result.isJsonObject()) {
            var array = new JsonArray(1);
            array.add(result);
            return array;
        } else if (result.isJsonArray()) {
            return result.getAsJsonArray();
        }

        throw new IllegalStateException("Unable to calculate by expression: " + by +
                ", must array or object, got " + result);
    }

}
