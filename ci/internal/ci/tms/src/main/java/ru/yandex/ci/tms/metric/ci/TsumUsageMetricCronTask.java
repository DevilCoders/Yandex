package ru.yandex.ci.tms.metric.ci;

import java.util.List;
import java.util.Objects;

import org.apache.curator.framework.CuratorFramework;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.springframework.http.HttpStatus;

import ru.yandex.ci.client.base.http.HttpException;
import ru.yandex.ci.client.tsum.TsumClient;
import ru.yandex.ci.client.tsum.TsumDeliveryMachines;
import ru.yandex.ci.client.tsum.TsumJob;
import ru.yandex.ci.client.tsum.TsumListProject;
import ru.yandex.ci.client.tsum.TsumPipeline;
import ru.yandex.ci.client.tsum.TsumPipelineConfiguration;
import ru.yandex.ci.client.tsum.TsumProject;
import ru.yandex.ci.core.db.CiMainDb;

public class TsumUsageMetricCronTask extends AbstractUsageMetricTask {
    private static final Logger log = LoggerFactory.getLogger(TsumUsageMetricCronTask.class);

    private final TsumClient tsumClient;

    public TsumUsageMetricCronTask(CiMainDb db, TsumClient tsumClient, CuratorFramework curator) {
        super(db, curator);
        this.tsumClient = tsumClient;
    }

    @Override
    public void computeMetric(MetricConsumer consumer) {

        Counters counters = new Counters();
        List<TsumListProject> listProjects = tsumClient.getProjects();
        for (TsumListProject listProject : listProjects) {
            count(listProject, counters);
        }
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_PROJECTS, counters.projects);

        consumer.addMetric(CiSystemsUsageMetrics.TSUM_ARCADIA_DELIVERY_MACHINES, counters.arcadiaDeliveryMachines);
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_ARCADIA_RELEASE_PIPELINES, counters.arcadiaReleasePipelines);

        int totalArcadia = counters.arcadiaDeliveryMachines + counters.arcadiaReleasePipelines;
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_RELEASES, totalArcadia);
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_TOTAL_PROCESSES, totalArcadia);

        consumer.addMetric(CiSystemsUsageMetrics.TSUM_GITHUB_DELIVERY_MACHINES, counters.githubDeliveryMachines);
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_GITHUB_RELEASE_PIPELINES, counters.githubReleasePipelines);

        consumer.addMetric(CiSystemsUsageMetrics.TSUM_BITBUCKET_DELIVERY_MACHINES, counters.bitbucketDeliveryMachines);

        int totalWithGithub = totalArcadia + counters.githubDeliveryMachines + counters.githubReleasePipelines;
        consumer.addMetric(CiSystemsUsageMetrics.TSUM_TOTAL_RELEASES_WITH_GITHUB, totalWithGithub);
    }

    private void count(TsumListProject listProject, Counters counters) {
        log.info("Counting tsum project {}: {}", listProject.getId(), listProject.getTitle());
        TsumProject project = tsumClient.getProject(listProject.getId());
        countProject(project, counters);
        countDeliveryMachines(project, counters);
        countReleasePipelines(project, counters);
    }

    private void countProject(TsumProject project, Counters counters) {
        if (project.getDeliveryMachines().isEmpty() && project.getPipelines().isEmpty()) {
            return;
        }
        counters.projects++;
    }

    private void countDeliveryMachines(TsumProject project, Counters counters) {
        for (TsumDeliveryMachines deliveryMachine : project.getDeliveryMachines()) {
            switch (deliveryMachine.getVcsSettings().getType()) {
                case ARCADIA -> counters.arcadiaDeliveryMachines++;
                case GITHUB -> counters.githubDeliveryMachines++;
                case BITBUCKET -> counters.bitbucketDeliveryMachines++;
                default ->
                        //Значит ЦУМ поддержал ещё какую-то VCS и её стоит добавить в метрику.
                        throw new UnsupportedOperationException(
                                "Unknown TSUM vcs type " + deliveryMachine.getVcsSettings().getType()
                        );
            }
        }
    }

    private void countReleasePipelines(TsumProject project, Counters counters) {
        for (String pipelineId : project.getReleasePipelineIds()) {
            TsumPipeline pipeline = project.getPipelines().get(pipelineId);
            if (pipeline.isArchived()) {
                continue;
            }
            if (pipeline.getManualResources().stream().anyMatch(Objects::isNull)) {
                log.warn(
                        "Invalid pipeline: {} in project {}. One of manual resources is null",
                        pipeline, project.getId()
                );
                continue;
            }
            // Т.к. в пайплайнах можно воровать-убивать (не забывая про гусей) - нет нормального признака,
            // к какой VCS он относиться. Поэтому вытаскиваем список джобы и просто "угадываем" по названию
            final TsumPipelineConfiguration configuration;
            try {
                configuration = tsumClient.getPipelineConfiguration(
                        pipeline.getId(), pipeline.getConfigurationVersion()
                );
            } catch (HttpException e) {
                if (e.getHttpCode() == HttpStatus.NOT_FOUND.value()) {
                    log.warn(
                            "Invalid pipeline: {} in project {}. Failed to get configuration",
                            pipeline, project.getId(),
                            e
                    );
                    continue;
                }

                throw e;
            }
            var fullConfiguration = tsumClient.getPipelineFullConfiguration(configuration.getId());
            for (TsumJob job : fullConfiguration.getJobs()) {
                if (job.getExecutorClass().toLowerCase().contains("arcadia")) {
                    counters.arcadiaReleasePipelines++;
                }
                if (job.getExecutorClass().toLowerCase().contains("github")) {
                    counters.githubReleasePipelines++;
                }
            }
        }
    }

    private static class Counters {
        private int projects;
        private int arcadiaDeliveryMachines;
        private int githubDeliveryMachines;
        private int bitbucketDeliveryMachines;
        private int arcadiaReleasePipelines;
        private int githubReleasePipelines;
    }
}
