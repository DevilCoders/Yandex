package ru.yandex.ci.tms.metric.ci;

import java.util.HashSet;
import java.util.List;
import java.util.Set;

import one.util.streamex.StreamEx;
import org.apache.curator.framework.CuratorFramework;

import ru.yandex.ci.client.teamcity.TeamcityBuildType;
import ru.yandex.ci.client.teamcity.TeamcityClient;
import ru.yandex.ci.core.db.CiMainDb;

public class TeamcityUsageMetricTask extends AbstractUsageMetricTask {

    private final TeamcityClient teamcityClient;

    public TeamcityUsageMetricTask(CiMainDb db, TeamcityClient teamcityClient, CuratorFramework curator) {
        super(db, curator);
        this.teamcityClient = teamcityClient;
    }

    @Override
    public void computeMetric(MetricConsumer consumer) {

        List<TeamcityBuildType> arcBuildTypes = teamcityClient.getBuildTypesWithArcRoots();
        List<TeamcityBuildType> svnBuildTypes = teamcityClient.getBuildTypesWithArcadiaSvnRoots();

        Set<String> arcProjects = StreamEx.of(arcBuildTypes).map(TeamcityBuildType::getProjectId).toSet();
        Set<String> svnProjects = StreamEx.of(svnBuildTypes).map(TeamcityBuildType::getProjectId).toSet();

        Set<String> allProjects = new HashSet<>();
        allProjects.addAll(arcProjects);
        allProjects.addAll(svnProjects);

        int arcRoots = teamcityClient.getArcRoots().size();
        int svnRoots = teamcityClient.getArcadiaSvnRoots().size();

        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_ARC_BUILD_TYPES, arcBuildTypes.size());
        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_ARC_PROJECTS, arcProjects.size());

        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_SVN_BUILD_TYPES, svnBuildTypes.size());
        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_SVN_PROJECTS, svnProjects.size());

        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_TOTAL_PROCESSES, svnBuildTypes.size() + arcBuildTypes.size());
        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_PROJECTS, allProjects.size());

        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_ARC_ROOTS, arcRoots);
        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_SVN_ROOTS, svnRoots);
        consumer.addMetric(CiSystemsUsageMetrics.TEAMCITY_TOTAL_ARCADIA_ROOTS, arcRoots + svnRoots);
    }


}
