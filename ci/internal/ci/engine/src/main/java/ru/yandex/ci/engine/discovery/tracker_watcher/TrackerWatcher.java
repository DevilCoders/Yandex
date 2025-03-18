package ru.yandex.ci.engine.discovery.tracker_watcher;

import java.util.List;
import java.util.Set;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import com.google.common.base.Preconditions;
import com.google.common.base.Suppliers;
import com.google.gson.JsonObject;
import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;
import yav_service.YavOuterClass;

import ru.yandex.ci.core.arc.ArcBranch;
import ru.yandex.ci.core.arc.ArcRevision;
import ru.yandex.ci.core.arc.ArcService;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.config.a.model.TrackerWatchConfig;
import ru.yandex.ci.core.db.model.ConfigState;
import ru.yandex.ci.core.db.model.TrackerFlow;
import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.engine.config.ConfigBundle;
import ru.yandex.ci.engine.config.ConfigurationService;
import ru.yandex.ci.engine.launch.FlowLaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService;
import ru.yandex.ci.engine.launch.OnCommitLaunchService.StartFlowParameters;
import ru.yandex.ci.engine.tasks.tracker.TrackerTicketCollector;
import ru.yandex.ci.flow.db.CiDb;
import ru.yandex.ci.flow.engine.model.FlowLaunchId;
import ru.yandex.ci.flow.engine.runtime.state.calculator.FlowLaunchMutexManager;

@Slf4j
@RequiredArgsConstructor
public class TrackerWatcher {

    private final TrackerTicketCollector trackerTicketCollector;
    private final OnCommitLaunchService onCommitLaunchService;
    private final ConfigurationService configurationService;
    private final FlowLaunchService flowLaunchService;
    private final FlowLaunchMutexManager flowLaunchMutexManager;
    private final ArcService arcService;
    private final CiDb db;
    private final Set<String> queueWhitelist = Set.of("TOARCADIA");

    public void process() {
        var active = db.scan().run(() -> db.configStates().findAllVisible(true));

        var getHead = getHead();

        int processed = 0;
        int checked = 0;
        for (var state : active) {
            if (state.getLastValidRevision() == null) {
                continue; // ---
            }

            var getConfig = getConfig(state);
            for (var action : state.getActions()) {
                checked++;
                if (action.getTrackerWatchConfig() != null) {
                    processed++;
                    var ciProcessId = CiProcessId.ofFlow(state.getConfigPath(), action.getFlowId());
                    new SingleAction(getHead, getConfig, ciProcessId, action.getTrackerWatchConfig())
                            .process();
                }
            }
        }

        log.info("Processed {} tracker actions out of {}, total {} states", processed, checked, active.size());

    }


    public void processSingle(ConfigState configState, CiProcessId ciProcessId, TrackerWatchConfig trackerCfg) {
        new SingleAction(getHead(), getConfig(configState), ciProcessId, trackerCfg).process();
    }

    private Supplier<ArcRevision> getHead() {
        return Suppliers.memoize(() ->
                arcService.getLastRevisionInBranch(ArcBranch.trunk()));
    }

    private Supplier<ConfigBundle> getConfig(ConfigState configState) {
        Preconditions.checkState(configState.getLastValidRevision() != null,
                "Expect config with last valid revision only");
        return Suppliers.memoize(() ->
                configurationService.getConfig(configState.getConfigPath(), configState.getLastValidRevision()));
    }

    @RequiredArgsConstructor
    private class SingleAction {
        final Supplier<ArcRevision> getHead;
        final Supplier<ConfigBundle> getConfig;
        final CiProcessId ciProcessId;
        final TrackerWatchConfig trackerCfg;
        final Supplier<YavOuterClass.YavSecretSpec> spec = () ->
                YavOuterClass.YavSecretSpec.newBuilder()
                        .setKey(getTrackerCfg().getSecret().getKey())
                        .build();
        final Supplier<YavToken.Id> token = () ->
                getConfig().getConfigEntity().getSecurityState().getYavTokenUuid();

        private TrackerWatchConfig getTrackerCfg() {
            return trackerCfg;
        }

        private ConfigBundle getConfig() {
            return getConfig.get();
        }

        public void process() {
            log.info("Processing {} with {}", ciProcessId, trackerCfg);

            var queue = trackerCfg.getQueue();
            if (!queueWhitelist.contains(queue)) {
                log.warn("Queue {} is not in whitelist: {}", queue, queueWhitelist);
                return; // ---
            }

            closeFlows();
            startFlows();
        }

        private void closeFlows() {
            log.info("Checking issues to stop...");
            int processed = 0;
            for (var closeStatus : trackerCfg.getCloseStatuses()) {
                var issues = getIssues(closeStatus);

                var trackerIds = issues.stream()
                        .map(TrackerTicketCollector.TrackerIssue::getKey)
                        .filter(issueKey -> !skipIssue(issueKey, trackerCfg))
                        .map(issueKey -> TrackerFlow.Id.of(ciProcessId, issueKey))
                        .collect(Collectors.toSet());
                log.info("Total {} tracker flows to check", trackerIds.size());

                var trackerFlows = db.currentOrReadOnly(() -> db.trackerFlowTable().find(trackerIds));
                for (var trackerFlow : trackerFlows) {
                    closeOldTrackerFlow(trackerFlow);
                    processed++;
                }
            }

            log.info("All issues processed for stop: {}", processed);
        }

        private void startFlows() {
            log.info("Checking issues to start...");
            int processed = 0;
            var issues = getIssues(trackerCfg.getStatus());
            for (var issue : issues) {
                if (issue.getAssignee() == null || issue.getStatusUpdated() == null) {
                    log.info("Skip issue without assignee or status updated: {}", issue);
                    continue;
                }

                var issueKey = issue.getKey();
                if (skipIssue(issueKey, trackerCfg)) {
                    log.info("Skip issue not in whitelist: {}", issue);
                    continue;
                }

                log.info("Checking issue: {}", issue);
                var trackerFlowId = TrackerFlow.Id.of(ciProcessId, issueKey);

                var flowOptional = db.currentOrReadOnly(() -> db.trackerFlowTable().find(trackerFlowId));
                if (flowOptional.isPresent()) {
                    var flow = flowOptional.get();
                    if (flow.getStatusUpdated() == null ||
                            flow.getStatusUpdated().isBefore(issue.getStatusUpdated())) {
                        log.info("Status updated: {} -> {}", flow.getStatusUpdated(), issue.getStatusUpdated());
                        closeOldTrackerFlow(flow);
                    } else {
                        log.info("Flow already exists: {}", flow.toLaunchId());
                        continue; // ---
                    }
                }
                startNewTrackerFlow(issue, trackerFlowId);
                processed++;
            }

            log.info("All issues processed for start: {}", processed);
        }

        private void closeOldTrackerFlow(TrackerFlow trackerFlow) {
            var launchId = trackerFlow.toLaunchId();
            log.info("Closing flow for {}", launchId);
            flowLaunchMutexManager.acquireAndRun(launchId,
                    () -> db.currentOrTx(() -> {
                        var launch = db.launches().find(launchId.toKey());
                        if (launch.isPresent() && !launch.get().getStatus().isTerminal()) {
                            flowLaunchService.cancelFlow(FlowLaunchId.of(launchId));
                        }
                        db.trackerFlowTable().delete(trackerFlow.getId());
                    }));
        }

        private void startNewTrackerFlow(TrackerTicketCollector.TrackerIssue issue, TrackerFlow.Id trackerFlowId) {
            Preconditions.checkState(issue.getAssignee() != null);
            Preconditions.checkState(issue.getStatusUpdated() != null);

            var flowVars = new JsonObject();
            flowVars.addProperty(trackerCfg.getFlowVar(), issue.getKey());

            var headRevision = getHead.get();
            var params = StartFlowParameters.builder()
                    .processId(ciProcessId)
                    .branch(ArcBranch.trunk())
                    .revision(headRevision)
                    .configOrderedRevision(getConfig().getRevision())
                    .triggeredBy(issue.getAssignee())
                    .flowVars(flowVars)
                    .skipUpdatingDiscoveredCommit(true)
                    .build();
            db.currentOrTx(() -> {
                var launch = onCommitLaunchService.startFlow(params);
                log.info("Registered new flow: {}", launch.getLaunchId());
                db.trackerFlowTable().save(TrackerFlow.of(
                        trackerFlowId,
                        launch.getId().getLaunchNumber(),
                        issue.getStatusUpdated()));
            });
        }

        private List<TrackerTicketCollector.TrackerIssue> getIssues(String status) {
            return trackerTicketCollector.getIssues(spec.get(), token.get(), trackerCfg.getQueue(), status);
        }

        private boolean skipIssue(String issue, TrackerWatchConfig trackerCfg) {
            if (!trackerCfg.getIssues().isEmpty()) {
                return !trackerCfg.getIssues().contains(issue);
            }
            return false;
        }
    }

}
