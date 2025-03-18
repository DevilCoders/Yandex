package ru.yandex.ci.core.config.a;

import java.util.Collection;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;
import java.util.stream.Stream;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import lombok.AllArgsConstructor;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.config.a.model.CiConfig;
import ru.yandex.ci.core.config.a.model.FlowWithFlowVars;
import ru.yandex.ci.core.config.a.model.JobConfig;
import ru.yandex.ci.core.config.a.model.ReleaseConfig;

@Slf4j
public class AYamlPostProcessor {

    private AYamlPostProcessor() {
        //
    }

    public static CiConfig postProcessCi(@Nonnull CiConfig ci) {
        return updateImplicitStages(ci);
    }

    private static CiConfig updateImplicitStages(CiConfig ci) {
        var holder = new CiHolder(ci);

        for (var release : ci.getReleases().values()) {
            new ImplicitStageHandler(holder, release).update();
        }

        return holder.builder != null
                ? holder.builder.build() // Stages were updated
                : ci; // No changes
    }


    @AllArgsConstructor
    private static class ImplicitStageHandler {

        @Nonnull
        private final CiHolder holder;

        @Nonnull
        private final ReleaseConfig release;

        private final Set<String> checkedFlows = new HashSet<>();

        void update() {
            maybeUpdateJobStages(release.getFlow());
            Stream.of(release.getHotfixFlows(), release.getRollbackFlows())
                    .flatMap(Collection::stream)
                    .map(FlowWithFlowVars::getFlow)
                    .forEach(this::maybeUpdateJobStages);
        }

        private void maybeUpdateJobStages(String flowId) {
            var flowOptional = holder.ci.findFlow(flowId);
            if (flowOptional.isEmpty()) {
                log.warn("Flow {} not found, unable to update job stages", flowId);
                return; //
            }
            var flow = flowOptional.get();

            if (!checkedFlows.add(flowId)) {
                return; // already checked this flow
            }

            // Similar to AYamlStageValidator, but simpler and without validation
            // В cleanupJobs стадий нет, поэтому их проверять не нужно
            var allJobs = flow.getJobs();

            var jobs = allJobs.values().stream()
                    .map(JobConfig::getId)
                    .collect(Collectors.toList());

            var singleStageId = release.getStages().size() == 1
                    ? release.getStages().get(0).getId()
                    : null;

            var updater = new StageUpdater(new HashSet<>(), new LinkedHashMap<>(allJobs), singleStageId);
            updater.update(jobs);

            if (updater.modified) {
                holder.toBuilder().flow(flow.withJobs(updater.allJobs));
            }
        }
    }

    @RequiredArgsConstructor
    private static class StageUpdater {
        private final Set<String> checkedJobs;
        private final LinkedHashMap<String, JobConfig> allJobs;
        private final @Nullable
        String singleStageId;
        private boolean modified;

        void update(List<String> jobsToUpdate) {
            for (var jobId : jobsToUpdate) {
                if (!checkedJobs.add(jobId)) {
                    continue; // ---
                }
                var job = allJobs.get(jobId);
                if (job == null) {
                    continue; // ---
                }
                if (job.getStage() != null) {
                    continue; // ---
                }

                var needs = job.getNeeds();
                if (needs.isEmpty() && singleStageId != null) {
                    allJobs.put(jobId, job.toBuilder()
                            .stage(singleStageId)
                            .build());
                    modified = true;
                    continue; // ---
                }

                this.update(needs);

                var possibleStages = needs.stream()
                        .map(allJobs::get)
                        .filter(Objects::nonNull)
                        .map(JobConfig::getStage)
                        .filter(Objects::nonNull)
                        .distinct()
                        .toList();

                if (possibleStages.size() == 1) {
                    allJobs.put(jobId, job.toBuilder()
                            .stage(possibleStages.get(0))
                            .build());
                    modified = true;
                }
            }
        }
    }

    @RequiredArgsConstructor
    private static class CiHolder {
        private final CiConfig ci;

        @Nullable
        private CiConfig.Builder builder;

        CiConfig.Builder toBuilder() {
            if (builder == null) {
                builder = ci.toBuilder();
            }
            return builder;
        }

    }

}
