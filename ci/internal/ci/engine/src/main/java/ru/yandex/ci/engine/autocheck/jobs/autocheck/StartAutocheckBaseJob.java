package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.util.ArrayList;

import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.JobExecutor;
import ru.yandex.ci.flow.engine.runtime.state.model.TaskBadge;
import ru.yandex.ci.flow.utils.UrlService;

public abstract class StartAutocheckBaseJob implements JobExecutor {
    private final UrlService urlService;

    protected StartAutocheckBaseJob(UrlService urlService) {
        this.urlService = urlService;
    }

    public static String generateGsidBase(JobContext context) {
        var gsidParts = new ArrayList<String>();

        var vcsInfo = context.getFlowLaunch().getVcsInfo();
        var pullRequestInfo = vcsInfo.getPullRequestInfo();

        if (pullRequestInfo != null) {
            gsidParts.add("ARCANUM:" + pullRequestInfo.getPullRequestId());
            gsidParts.add("ARCANUM_DIFF_SET:" + pullRequestInfo.getDiffSetId());
            gsidParts.add("ARC_MERGE:" + vcsInfo.getRevision().getCommitId());
            gsidParts.add("CI_CHECK_OWNER:" + pullRequestInfo.getAuthor());
        } else {
            var commit = vcsInfo.getCommit();
            if (commit != null) {
                gsidParts.add("USER:" + commit.getAuthor());
            }
            gsidParts.add("ARC_COMMIT:r" + vcsInfo.getRevision().getNumber());
            if (commit != null) {
                gsidParts.add("CI_CHECK_OWNER:" + commit.getAuthor());
            }
        }
        gsidParts.add(getCIGsidPart(context.getFlowLaunch().getLaunchId()));
        gsidParts.add("CI_FLOW:" + context.getFullJobLaunchId().getFlowLaunchId().asString());

        return String.join(" ", gsidParts);
    }

    private static String getCIGsidPart(LaunchId launchId) {
        return "CI:" + launchId.getProcessId().getPath() +
                ":ACTION:" + launchId.getProcessId().getSubId() +
                ":" + launchId.getNumber();
    }

    protected void updateTaskBadge(JobContext context, String checkId) {
        String url = urlService.toCiCardPreview(checkId);
        var taskBadge = TaskBadge.of("ci-bage", "CI_BADGE", url, TaskBadge.TaskStatus.SUCCESSFUL);
        context.progress().updateTaskState(taskBadge);
    }
}
