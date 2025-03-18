package ru.yandex.ci.tms.metric.ci;

import java.util.HashSet;
import java.util.Set;

import lombok.ToString;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.core.config.a.model.TriggerConfig;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.project.ActionConfigState;

public class CiSelfUsageMetricCronTask extends AbstractUsageMetricTask {
    private final CiMainDb db;

    public CiSelfUsageMetricCronTask(CiMainDb db, CuratorFramework curator) {
        super(db, curator);
        this.db = db;
    }

    @Override
    public void computeMetric(MetricConsumer consumer) {
        Counter counter = db.currentOrReadOnly(() -> {
            Counter localCounter = new Counter();
            db.configStates().streamAll(1000).forEach(localCounter::collect);
            return localCounter;
        });

        consumer.addMetric(CiSystemsUsageMetrics.CI_PROJECTS, counter.projects.size());
        consumer.addMetric(CiSystemsUsageMetrics.CI_FLOWS, counter.flows);

        consumer.addMetric(CiSystemsUsageMetrics.CI_RELEASES, counter.releases);
        consumer.addMetric(CiSystemsUsageMetrics.CI_PR_PROCESSES, counter.prProcesses);
        consumer.addMetric(CiSystemsUsageMetrics.CI_COMMIT_PROCESSES, counter.commitProcesses);
        consumer.addMetric(CiSystemsUsageMetrics.CI_CI_PROCESSES_TAG, counter.ciProcesses);
        consumer.addMetric(CiSystemsUsageMetrics.CI_TOTAL_PROCESSES, counter.releases + counter.ciProcesses);
    }

    @ToString
    public static class Counter {
        final Set<String> projects = new HashSet<>();
        int releases = 0;
        int prProcesses = 0;
        int commitProcesses = 0;
        int ciProcesses = 0;
        int flows = 0;

        public void collect(ConfigState state) {
            var status = state.getStatus();
            if (status == ConfigState.Status.INVALID || status.isHidden()) {
                return;
            }
            if (state.getReleases().isEmpty() && state.getActions().isEmpty()) {
                return;
            }

            projects.add(state.getProject());
            releases += state.getReleases().size();
            flows += state.getActions().size();

            boolean isCiProcess = false;
            for (ActionConfigState flow : state.getActions()) {

                boolean foundCiProcess = false;
                boolean foundCommitProcess = false;
                for (TriggerConfig trigger : flow.getTriggers()) {
                    var on = trigger.getOn();
                    if (on == TriggerConfig.On.PR) {
                        if (!foundCiProcess) {
                            prProcesses++;
                            isCiProcess = true;
                            foundCiProcess = true;
                        }
                    }
                    if (on == TriggerConfig.On.COMMIT) {
                        if (!foundCommitProcess) {
                            commitProcesses++;
                            isCiProcess = true;
                            foundCommitProcess = true;
                        }
                    }
                }

                if (isCiProcess) {
                    ciProcesses++;
                }
            }
        }
    }
}
