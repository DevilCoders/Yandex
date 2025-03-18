package ru.yandex.ci.tms.metric.ci;

import java.util.ArrayList;
import java.util.List;

import io.micrometer.core.instrument.Tag;

import ru.yandex.ci.ydb.service.metric.MetricId;

public class CiSystemsUsageMetrics {
    private static final List<MetricId> ALL_METRICS = new ArrayList<>();

    private static final String CI_SYSTEMS_USAGE = "ci_system_usage";

    public static final String SYSTEM_TAG_NAME = "system";
    private static final Tag CI_TAG = Tag.of(SYSTEM_TAG_NAME, "CI");
    private static final Tag RM_TAG = Tag.of(SYSTEM_TAG_NAME, "RM");
    private static final Tag TSUM_TAG = Tag.of(SYSTEM_TAG_NAME, "Tsum");
    private static final Tag TE_TAG = Tag.of(SYSTEM_TAG_NAME, "TE");
    private static final Tag TEAMCITY_TAG = Tag.of(SYSTEM_TAG_NAME, "Teamcity");
    private static final Tag POTATO_TAG = Tag.of(SYSTEM_TAG_NAME, "Potato");


    private static final String TYPE_TAG_NAME = "type";
    private static final Tag PROJECTS_TAG = Tag.of(TYPE_TAG_NAME, "projects");
    private static final Tag RELEASES_TAG = Tag.of(TYPE_TAG_NAME, "releases");
    private static final Tag PR_PROCESSES_TAG = Tag.of(TYPE_TAG_NAME, "prProcesses");
    private static final Tag COMMIT_PROCESSES_TAG = Tag.of(TYPE_TAG_NAME, "commitProcesses");
    public static final Tag FLOWS_TAG = Tag.of(TYPE_TAG_NAME, "flows");
    private static final Tag TOTAL_PROCESSES_TAG = Tag.of(TYPE_TAG_NAME, "totalProcesses");
    private static final Tag CI_PROCESSES_TAG = Tag.of(TYPE_TAG_NAME, "ciProcesses");

    public static final MetricId CI_PROJECTS = metric(CI_TAG, PROJECTS_TAG);
    public static final MetricId CI_FLOWS = metric(CI_TAG, FLOWS_TAG);
    public static final MetricId CI_RELEASES = metric(CI_TAG, RELEASES_TAG);
    public static final MetricId CI_PR_PROCESSES = metric(CI_TAG, PR_PROCESSES_TAG);
    public static final MetricId CI_COMMIT_PROCESSES = metric(CI_TAG, COMMIT_PROCESSES_TAG);
    public static final MetricId CI_CI_PROCESSES_TAG = metric(CI_TAG, CI_PROCESSES_TAG);
    public static final MetricId CI_TOTAL_PROCESSES = metric(CI_TAG, TOTAL_PROCESSES_TAG);

    public static final MetricId RM_RELEASES = metric(RM_TAG, RELEASES_TAG);
    public static final MetricId RM_RELEASES_OVER_CI = metricWithType(RM_TAG, "releasesOverCi");
    public static final MetricId RM_RELEASES_OVER_TE = metricWithType(RM_TAG, "releasesOverTe");
    public static final MetricId RM_TOTAL_PROCESSES = metric(RM_TAG, TOTAL_PROCESSES_TAG);

    public static final MetricId TSUM_PROJECTS = metric(TSUM_TAG, PROJECTS_TAG);
    public static final MetricId TSUM_RELEASES = metric(TSUM_TAG, RELEASES_TAG);
    public static final MetricId TSUM_ARCADIA_DELIVERY_MACHINES = metricWithType(TSUM_TAG, "arcadiaDeliveryMachines");
    public static final MetricId TSUM_GITHUB_DELIVERY_MACHINES = metricWithType(TSUM_TAG, "githubDeliveryMachines");
    public static final MetricId TSUM_BITBUCKET_DELIVERY_MACHINES = metricWithType(
            TSUM_TAG, "bitbucketDeliveryMachines"
    );

    public static final MetricId TSUM_ARCADIA_RELEASE_PIPELINES = metricWithType(TSUM_TAG, "arcadiaReleasePipelines");
    public static final MetricId TSUM_GITHUB_RELEASE_PIPELINES = metricWithType(TSUM_TAG, "githubReleasePipelines");

    public static final MetricId TSUM_TOTAL_PROCESSES = metric(TSUM_TAG, TOTAL_PROCESSES_TAG);
    public static final MetricId TSUM_TOTAL_RELEASES_WITH_GITHUB = metricWithType(
            TSUM_TAG, "totalReleasesWithGithub"
    );

    public static final MetricId TE_ACTIVE_GROUPS = metricWithType(TE_TAG, "activeGroups");

    public static final MetricId TE_ACTIVE_GROUPS_DOCS = metric(TE_TAG,
            Tag.of(TYPE_TAG_NAME, "activeGroups"), Tag.of("teProject", "deploy-docs")
    );

    public static final MetricId TE_TOTAL_PROCESSES = metric(TE_TAG, TOTAL_PROCESSES_TAG);
    public static final MetricId TE_PROJECTS = metric(TE_TAG, PROJECTS_TAG);
    public static final MetricId TE_AUTOCHECK_BRANCHES = metricWithType(TE_TAG, "autocheckBranches");
    public static final MetricId TE_CUSTOM_BRANCHES = metricWithType(TE_TAG, "customBranches");
    public static final MetricId TE_PR_PROCESSES = metric(TE_TAG, PR_PROCESSES_TAG);
    public static final MetricId TE_TOTAL_JOBS = metricWithType(TE_TAG, "totalJobs");

    public static final MetricId TEAMCITY_PROJECTS = metric(TEAMCITY_TAG, PROJECTS_TAG);
    public static final MetricId TEAMCITY_ARC_PROJECTS = metricWithType(TEAMCITY_TAG, "arcProjects");
    public static final MetricId TEAMCITY_SVN_PROJECTS = metricWithType(TEAMCITY_TAG, "svnProjects");
    public static final MetricId TEAMCITY_TOTAL_PROCESSES = metric(TEAMCITY_TAG, TOTAL_PROCESSES_TAG);
    public static final MetricId TEAMCITY_ARC_BUILD_TYPES = metricWithType(TEAMCITY_TAG, "arcBuildTypes");
    public static final MetricId TEAMCITY_SVN_BUILD_TYPES = metricWithType(TEAMCITY_TAG, "svnBuildTypes");

    public static final MetricId TEAMCITY_ARC_ROOTS = metricWithType(TEAMCITY_TAG, "arcRoots");
    public static final MetricId TEAMCITY_SVN_ROOTS = metricWithType(TEAMCITY_TAG, "svnRoots");
    public static final MetricId TEAMCITY_TOTAL_ARCADIA_ROOTS = metricWithType(TEAMCITY_TAG, "totalArcadiaVscRoots");


    public static final MetricId POTATO_PROJECTS = metric(POTATO_TAG, PROJECTS_TAG);
    public static final MetricId POTATO_ARCADIA_PROJECTS = metricWithType(POTATO_TAG, "arcadiaProjects");
    public static final MetricId POTATO_GITHUB_PROJECTS = metricWithType(POTATO_TAG, "githubProjects");
    public static final MetricId POTATO_BITBUCKET_PROJECTS = metricWithType(POTATO_TAG, "bitbucketProjects");
    public static final MetricId POTATO_GITHUB_PUBLIC_PROJECTS = metricWithType(POTATO_TAG, "githubPublicProjects");
    public static final MetricId POTATO_GITHUB_ENTERPRISE_PROJECTS =
            metricWithType(POTATO_TAG, "githubEnterpriseProjects");
    public static final MetricId POTATO_BITBUCKET_PUBLIC_PROJECTS =
            metricWithType(POTATO_TAG, "bitbucketPublicProjects");
    public static final MetricId POTATO_BITBUCKET_ENTERPRISE_PROJECTS =
            metricWithType(POTATO_TAG, "bitbucketEnterpriseProjects");
    public static final MetricId POTATO_BITBUCKET_BROWSER_PROJECTS =
            metricWithType(POTATO_TAG, "bitbucketBrowserProjects");

    private CiSystemsUsageMetrics() {
    }

    public static List<MetricId> allMetrics() {
        List<MetricId> allMetrics = new ArrayList<>();
        allMetrics.addAll(ALL_METRICS);
        allMetrics.addAll(TrendboxUsageMetricCronTask.getMetricIds());
        allMetrics.addAll(TestenvUsageMetricCronTask.getMetricIds());
        return allMetrics;
    }

    private static MetricId metric(Tag... tags) {
        MetricId metricId = MetricId.of(CI_SYSTEMS_USAGE, tags);
        ALL_METRICS.add(metricId);
        return metricId;
    }

    private static MetricId metricWithType(Tag systemTag, String type) {
        return metric(systemTag, Tag.of(TYPE_TAG_NAME, type));
    }
}
