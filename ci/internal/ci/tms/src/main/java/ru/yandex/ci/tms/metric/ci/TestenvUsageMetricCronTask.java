package ru.yandex.ci.tms.metric.ci;

import java.time.Duration;
import java.time.Instant;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.concurrent.Callable;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicInteger;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.micrometer.core.instrument.Tag;
import lombok.ToString;
import lombok.Value;
import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import ru.yandex.ci.client.testenv.TestenvClient;
import ru.yandex.ci.client.testenv.model.TestenvJob;
import ru.yandex.ci.client.testenv.model.TestenvListProject;
import ru.yandex.ci.client.testenv.model.TestenvProject;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.ydb.service.metric.MetricId;
import ru.yandex.commune.bazinga.scheduler.schedule.Schedule;
import ru.yandex.commune.bazinga.scheduler.schedule.SchedulePeriodic;
import ru.yandex.commune.bazinga.scheduler.schedule.ScheduleWithRetry;

public class TestenvUsageMetricCronTask extends AbstractUsageMetricTask {
    private static final Logger log = LoggerFactory.getLogger(TestenvUsageMetricCronTask.class);
    private static final int TASK_TIMEOUT_MINUTES = 60;
    private static final int THREADS_COUNT = 5;

    private static final Set<String> SKIP_PROJECTS = Set.of(
            "autocheck-trunk",
            "autocheck_recheck",
            "fizzzgen_testing_clone_2",
            "fizzzgen_testing",
            "fizzzgen_testing_2"
    );

    private static final List<String> DETAILED_PROJECTS = List.of(
            // https://st.yandex-team.ru/CI-2149
            "deploy-docs",
            // https://st.yandex-team.ru/CIWELCOME-466
            "rtyserver-trunk-builds",
            "rtyserver-trunk-cluster-linux",
            "rtyserver-trunk-func-distributor-linux",
            "rtyserver-trunk-func-release-linux",
            "rtyserver-trunk-load-dolbilo",
            "rtyserver-trunk-multi-linux",
            "rtyserver-trunk-saas-ferryman",
            "rtyserver-trunk-unit-linux"
    );


    private final TestenvClient testenvClient;

    public TestenvUsageMetricCronTask(CiMainDb db, TestenvClient testenvClient, @Nullable CuratorFramework curator) {
        super(db, curator);
        this.testenvClient = testenvClient;
    }

    @Override
    public Duration getTimeout() {
        return Duration.ofMinutes(TASK_TIMEOUT_MINUTES);
    }

    @Override
    public Schedule cronExpression() {
        return new ScheduleWithRetry(new SchedulePeriodic(3, TimeUnit.HOURS), 2);
    }

    @Override
    public void computeMetric(MetricConsumer consumer) throws Exception {

        Counters total = new Counters();
        var countersPerProject = countProjects();

        for (var projectCounter : countersPerProject.values()) {
            total.add(projectCounter);
        }

        log.info("All {} projects processed", countersPerProject.size());
        log.info("Counters: {}", total);

        consumer.addMetric(CiSystemsUsageMetrics.TE_PROJECTS, total.projects);
        consumer.addMetric(CiSystemsUsageMetrics.TE_TOTAL_PROCESSES, total.activeGroups);
        consumer.addMetric(CiSystemsUsageMetrics.TE_ACTIVE_GROUPS, total.activeGroups);

        consumer.addMetric(CiSystemsUsageMetrics.TE_PR_PROCESSES, total.activePreCommitJobs);
        consumer.addMetric(CiSystemsUsageMetrics.TE_TOTAL_JOBS, total.activeJobs);

        consumer.addMetric(CiSystemsUsageMetrics.TE_AUTOCHECK_BRANCHES, total.autocheckBranch);
        consumer.addMetric(CiSystemsUsageMetrics.TE_CUSTOM_BRANCHES, total.customBranch);

        for (var detailedProject : DETAILED_PROJECTS) {
            if (!countersPerProject.containsKey(detailedProject)) {
                continue;
            }
            addProjectCounters(consumer, detailedProject, countersPerProject.get(detailedProject));
        }
    }

    public Map<String, Counters> countProjects() throws Exception {
        List<TestenvListProject> projects = getNonReleaseMachineProjects();

        ExecutorService executorService = Executors.newFixedThreadPool(
                THREADS_COUNT,
                new ThreadFactoryBuilder().setNameFormat("testenv-usage-metrics-%d").build()
        );

        AtomicInteger processedCounter = new AtomicInteger();

        var futures = executorService.invokeAll(
                StreamEx.of(projects)
                        .map(listProject -> toCallable(listProject, processedCounter, projects.size()))
                        .toList(),
                TASK_TIMEOUT_MINUTES, TimeUnit.MINUTES
        );
        executorService.shutdown();

        var result = new HashMap<String, Counters>(futures.size());
        for (int i = 0; i < futures.size(); i++) {
            var future = futures.get(i);
            try {
                var projectCounters = future.get();
                result.put(projectCounters.getProject(), projectCounters.getCounters());
            } catch (Exception e) {
                log.error("Exception while processing project {}", projects.get(i).getName());
                throw e;
            }
        }

        return result;
    }

    private static void addProjectCounters(MetricConsumer consumer, String project, Counters counter) {
        consumer.addMetric(withTEProject(CiSystemsUsageMetrics.TE_ACTIVE_GROUPS, project), counter.activeGroups);
        consumer.addMetric(withTEProject(CiSystemsUsageMetrics.TE_TOTAL_JOBS, project), counter.activeJobs);
        consumer.addMetric(withTEProject(CiSystemsUsageMetrics.TE_PR_PROCESSES, project), counter.activePreCommitJobs);
    }

    private static MetricId withTEProject(MetricId metricId, String project) {
        var tags = new ArrayList<>(metricId.getTags());
        tags.add(Tag.of("teProject", project));
        return new MetricId(metricId.getName(), tags);
    }

    public static List<MetricId> getMetricIds() {
        var metricIds = new ArrayList<MetricId>();
        for (var project : DETAILED_PROJECTS) {
            addProjectCounters(new MetricConsumer() {
                @Override
                public void addMetric(MetricId metricId, double value) {
                    addMetric(metricId, value, now());
                }

                @Override
                public void addMetric(MetricId metricId, double value, Instant time) {
                    metricIds.add(metricId);
                }

                @Override
                public Instant now() {
                    return Instant.now();
                }
            }, project, new Counters());
        }
        return metricIds;
    }

    private List<TestenvListProject> getNonReleaseMachineProjects() {
        List<TestenvListProject> projects = testenvClient.getProjects();
        int allProjectsCount = projects.size();
        log.info("Got {} projects", allProjectsCount);
        projects = StreamEx.of(projects).filter(p -> !p.getShard().equals("release_machine")).toList();
        log.info("Filtered {} RM projects, {} remaining", allProjectsCount - projects.size(), projects.size());
        return projects;
    }

    private Callable<ProjectCounters> toCallable(TestenvListProject listProject,
                                                 AtomicInteger processedCounter,
                                                 int total) {
        return () -> {
            Counters counters = countProject(listProject);
            int processed = processedCounter.incrementAndGet();
            if (processed % 100 == 0) {
                log.info("Processed {}/{} projects.", processed, total);
            }
            return ProjectCounters.of(listProject.getName(), counters);
        };
    }

    private Counters countProject(TestenvListProject listProject) {
        String projectName = listProject.getName();
        log.debug("Processing project: {}", projectName);
        Counters counters = new Counters();

        if (SKIP_PROJECTS.contains(projectName)) {
            log.debug("Skipping project {}", projectName);
            return counters;
        }

        TestenvProject project = testenvClient.getProject(projectName);

        Preconditions.checkState(project.getStatus() != null, "Unknown status for project %s", projectName);
        if (listProject.getStatus() != project.getStatus()) {
            log.warn(
                    "List projects status {} inconsistent with project status {} fro project {}",
                    listProject.getStatus(),
                    project.getStatus(),
                    projectName
            );
        }

        if (project.getStatus() == TestenvProject.Status.STOPPED) {
            log.debug("Skipping stopped project {}", projectName);
            return counters;
        }

        if (project.getSvnServer().equals("arcadia.yandex.ru/robots")) {
            log.info("Skipping arcadia.yandex.ru/robots project {}", project);
            return counters;
        }

        if (!project.getSvnPath().startsWith("trunk")) {
            if (projectName.startsWith("autocheck-branch") && project.getTaskOwner().equals("AUTOCHECK")) {
                counters.autocheckBranch++;
                return counters;
            }
            if (project.getSvnPath().contains("branches/")) {
                counters.customBranch++;
                return counters;
            }
            log.info("TODO getSvnPath {}", project);
        }
        countJobs(project, counters);


        if (counters.activeGroups > 0) {
            counters.projects++;
        }

        if (counters.activeGroups > 10) {
            log.info("Large project {}. {} active graphs", projectName, counters.activeGroups);
        }

        if (counters.releaseMachineJobs > 0) {
            log.warn(
                    "Skipping project {} cause {} jobs is from release machine. Maybe RM project in custom shard.",
                    project.getName(), counters.releaseMachineJobs
            );
            var newCounters = new Counters();
            newCounters.releaseMachineJobs = counters.releaseMachineJobs;
            return newCounters;
        }
        return counters;
    }

    private void countJobs(TestenvProject project, Counters counters) {
        List<TestenvJob> allJobs = testenvClient.getJobs(project.getName());

        List<TestenvJob> jobs = StreamEx.of(allJobs).distinct().toList();
        if (allJobs.size() != jobs.size()) {
            log.info("Duplicate jobs in project {}. Got {}, uniq {}.", project.getName(), allJobs.size(), jobs.size());
        }

        for (List<TestenvJob> jobsGroup : buildGroups(jobs)) {
            counters.add(countJobsGroup(jobsGroup));
        }

    }

    private Counters countJobsGroup(List<TestenvJob> jobs) {
        Counters groupCounters = new Counters();
        for (TestenvJob job : jobs) {
            countJob(job, groupCounters);
        }
        if (groupCounters.activeJobs > 0) {
            groupCounters.activeGroups++;
        }
        return groupCounters;
    }

    private void countJob(TestenvJob job, Counters counters) {

        if (job.getTestModulePath().startsWith("testenv/jobs/release_machine/")) {
            counters.releaseMachineJobs++;
            return;
        }

        Preconditions.checkState(job.getStatus() != null, "Unknown status for job %s", job.getName());
        if (job.getStatus() == TestenvJob.Status.STOPPED) {
            counters.disabledJobs++;
            return;
        }

        counters.activeJobs++;
        if (job.getTags().contains("LIGHT_PRECOMMIT_CHECKS") || job.getTags().contains("FULL_PRECOMMIT_CHECKS")) {
            counters.activePreCommitJobs++;
        }
    }

    private List<List<TestenvJob>> buildGroups(List<TestenvJob> jobs) {

        Set<String> remainingJobs = StreamEx.of(jobs).map(TestenvJob::getName).toSet();
        Map<String, TestenvJob> nameToJob = Maps.uniqueIndex(jobs, TestenvJob::getName);

        List<List<TestenvJob>> groups = new ArrayList<>();

        Multimap<String, TestenvJob> metaJobGraphs = ArrayListMultimap.create();

        List<TestenvJob> singleYmlJobs = new ArrayList<>();

        for (TestenvJob job : jobs) {
            if (job.getJobType().equals("meta_job")) {
                metaJobGraphs.put(job.getTestModuleName(), job);
                continue;
            }
            if (!remainingJobs.contains(job.getName())) {
                continue;
            }
            Set<String> jobsInGraph = new HashSet<>();
            processDependencies(job.getName(), nameToJob, jobsInGraph, remainingJobs);
            if (jobsInGraph.size() == 1 && job.getTestModulePath().endsWith(".yaml")) {
                singleYmlJobs.add(job);
                continue;
            }
            groups.add(StreamEx.of(jobsInGraph).map(nameToJob::get).toList());
        }


        for (String moduleName : metaJobGraphs.keySet()) {
            groups.add(List.copyOf(metaJobGraphs.get(moduleName)));
        }

        for (List<TestenvJob> group : StreamEx.of(singleYmlJobs).groupingBy(TestenvJob::getTestModulePath).values()) {
            groups.addAll(Set.of(group));
        }

        return groups;
    }

    private void processDependencies(String jobName,
                                     Map<String, TestenvJob> nameToJob,
                                     Set<String> jobsInGraph,
                                     Set<String> remainingJobs) {
        if (!remainingJobs.contains(jobName)) {
            return;
        }
        jobsInGraph.add(jobName);
        remainingJobs.remove(jobName);
        TestenvJob job = nameToJob.get(jobName);

        for (String dependencyJobName : getAllDependencies(job)) {
            processDependencies(dependencyJobName, nameToJob, jobsInGraph, remainingJobs);
        }
    }

    private Set<String> getAllDependencies(TestenvJob job) {
        Set<String> deps = new HashSet<>();
        addDependencyJobs(job.getChildJobs(), deps);
        addDependencyJobs(job.getParentJobs(), deps);
        return deps;
    }


    private void addDependencyJobs(List<TestenvJob.DependencyJob> dependencyJobs, Set<String> target) {
        for (TestenvJob.DependencyJob dependencyJob : dependencyJobs) {
            target.add(dependencyJob.getName());
            addDependencyJobs(dependencyJob.getItems(), target);
        }
    }

    @Value(staticConstructor = "of")
    private static class ProjectCounters {
        String project;
        Counters counters;
    }

    @ToString
    @SuppressWarnings("VisibilityModifier")
    public static class Counters {
        public int projects;
        public int activeGroups;
        public int activeJobs;
        public int activePreCommitJobs;
        public int disabledJobs;
        public int autocheckBranch;
        public int customBranch;
        public int releaseMachineJobs;

        void add(Counters counters) {
            projects += counters.projects;
            activeGroups += counters.activeGroups;
            activeJobs += counters.activeJobs;
            activePreCommitJobs += counters.activePreCommitJobs;
            disabledJobs += counters.disabledJobs;
            autocheckBranch += counters.autocheckBranch;
            customBranch += counters.customBranch;
            releaseMachineJobs += counters.releaseMachineJobs;
        }
    }
}
