package ru.yandex.ci.engine.autocheck.jobs.autocheck;

import java.time.Instant;
import java.util.Objects;
import java.util.UUID;

import javax.annotation.Nullable;

import com.google.common.base.Preconditions;
import com.google.gson.JsonObject;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.autocheck.Autocheck;
import ru.yandex.ci.core.arc.ArcCommit;
import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.autocheck.AutocheckConstants.PostCommits;
import ru.yandex.ci.core.autocheck.FlowVars;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.LaunchVcsInfo;
import ru.yandex.ci.core.resources.Resource;
import ru.yandex.ci.engine.autocheck.AutocheckBlacklistService;
import ru.yandex.ci.engine.autocheck.AutocheckService;
import ru.yandex.ci.engine.autocheck.model.CheckLaunchParams;
import ru.yandex.ci.flow.engine.definition.context.JobContext;
import ru.yandex.ci.flow.engine.definition.job.ExecutorInfo;
import ru.yandex.ci.flow.engine.definition.resources.Produces;
import ru.yandex.ci.flow.utils.UrlService;

@ExecutorInfo(
        title = "Start autocheck",
        description = "Internal job for starting autocheck task"
)
@Produces(single = Autocheck.AutocheckLaunch.class)
@Slf4j
public class StartAutocheckJob extends StartAutocheckBaseJob {

    public static final UUID ID = UUID.fromString("94d7ce5e-d418-4adf-9808-df8d2cb1fe4b");

    private final AutocheckService autocheckService;
    private final AutocheckBlacklistService autocheckBlacklistService;
    private final CiMainDb db;

    public StartAutocheckJob(
            AutocheckService autocheckService,
            AutocheckBlacklistService autocheckBlacklistService,
            CiMainDb db,
            UrlService urlService
    ) {
        super(urlService);
        this.autocheckService = autocheckService;
        this.autocheckBlacklistService = autocheckBlacklistService;
        this.db = db;
    }

    @Override
    public UUID getSourceCodeId() {
        return ID;
    }

    @Override
    public void execute(JobContext context) throws Exception {
        var flowVars = getFlowVars(context);
        var isPrecommit = FlowVars.getIsPrecommitOrThrow(flowVars);

        var vcsInfo = context.getFlowLaunch().getVcsInfo();
        var checkAuthor = getCheckAuthor(vcsInfo);
        var diffSetEventCreated = getDiffSetEventCreated(vcsInfo);
        var rightRevision = getRightRevision(vcsInfo);

        boolean skipCheck = getSkipCheck(isPrecommit, rightRevision);
        if (skipCheck) {
            log.info("Skipping autocheck. Check author: {}, rightRevision: {}", checkAuthor, rightRevision);
            var skippedLaunch = Autocheck.AutocheckLaunch.newBuilder()
                    .setSkip(true)
                    .build();
            context.resources().produce(Resource.of(skippedLaunch, "launch"));
            return;
        }

        var launchParamsBuilder = CheckLaunchParams.builder()
                .launchId(context.getFlowLaunch().getIdString())
                .ciProcessId(context.getFlowLaunch().getLaunchId().getProcessId())
                .launchNumber(context.getFlowLaunch().getLaunchNumber())
                .checkAuthor(checkAuthor)
                .rightRevision(rightRevision)
                .arcanumCheckId(getCheckId(vcsInfo))
                .precommitCheck(isPrecommit)
                .gsidBase(generateGsidBase(context))
                .stressTest(FlowVars.getIsStressTest(flowVars, false));

        OrderedArcRevision leftRevision;
        if (isPrecommit) {
            leftRevision = getLeftRevisionForAutocheckPullRequests(vcsInfo);
            launchParamsBuilder
                    .leftRevision(leftRevision)
                    .pullRequestId(getPullRequestId(vcsInfo))
                    .diffSetId(getDiffSetId(vcsInfo))
                    .diffSetEventCreated(diffSetEventCreated);
        } else {
            leftRevision = autocheckService.getLeftRevisionForAutocheckPostcommits(vcsInfo);
            launchParamsBuilder
                    .leftRevision(leftRevision)
                    .spRegistrationDisabled(FlowVars.getIsSpRegistrationDisabled(flowVars, false));
        }

        log.info(
                "Starting autocheck. Check author: {}, leftRevision: {}, rightRevision: {}",
                checkAuthor, leftRevision, rightRevision
        );

        var launch = autocheckService.createAutocheckLaunch(launchParamsBuilder.build());

        updateTaskBadge(context, launch.getCheckInfo().getCheckId());
        context.resources().produce(Resource.of(launch, "launch"));
    }

    private JsonObject getFlowVars(JobContext context) {
        return Objects.requireNonNull(context.getFlowLaunch().getFlowInfo().getFlowVars()).getData();
    }

    private boolean getSkipCheck(boolean isPrecommit, OrderedArcRevision rightRevision) {
        if (isPrecommit) {
            return false;
        }
        // autocheck is disabled in testing/prestable environments
        var autocheckEnabled = db.currentOrReadOnly(() ->
                db.keyValue().getBoolean(PostCommits.NAMESPACE, PostCommits.ENABLED, false)
        );
        log.info("autocheckEnabled={}", autocheckEnabled);
        return !autocheckEnabled || autocheckBlacklistService.skipPostCommit(rightRevision.toRevision());
    }

    public static OrderedArcRevision getLeftRevisionForAutocheckPullRequests(LaunchVcsInfo vcsInfo) {
        // previous revision is not null only in pull requests (ArcBranch.isPr)
        var leftRevision = vcsInfo.getPreviousRevision();
        Preconditions.checkState(leftRevision != null, "No left revision.");
        if (!leftRevision.getBranch().isTrunk()) {
            return leftRevision;
        }

        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        if (pullRequestInfo == null) {
            return leftRevision;
        }

        var prUpstreamBranch = pullRequestInfo.getVcsInfo().getUpstreamBranch();
        if (prUpstreamBranch.isTrunk()) {
            return leftRevision;
        }

        var branchLeftRevision = OrderedArcRevision.fromHash(
                leftRevision.getCommitId(),
                prUpstreamBranch,
                0,
                leftRevision.getPullRequestId()
        );

        log.info(
                "Upstream branch is {}, but left revision is in trunk {}. Overriding left revision: {}",
                prUpstreamBranch, leftRevision, branchLeftRevision
        );

        return branchLeftRevision;
    }

    public static OrderedArcRevision getRightRevision(LaunchVcsInfo vcsInfo) {
        return vcsInfo.getRevision();
    }

    public static String getCheckId(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        if (pullRequestInfo != null) {
            return "PR:" + pullRequestInfo.getPullRequestId() + ":" + pullRequestInfo.getDiffSetId();
        }
        ArcCommit commit = vcsInfo.getCommit();
        Preconditions.checkState(commit != null, "Commit not present");
        return "COMMIT:" + commit.getCommitId();
    }

    private String getCheckAuthor(LaunchVcsInfo vcsInfo) {
        if (vcsInfo.getPullRequestInfo() != null) {
            return vcsInfo.getPullRequestInfo().getAuthor();
        }
        ArcCommit commit = vcsInfo.getCommit();
        Preconditions.checkState(commit != null, "Commit not present");
        return commit.getAuthor();
    }

    @Nullable
    private Long getPullRequestId(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        return pullRequestInfo != null ? pullRequestInfo.getPullRequestId() : null;
    }

    @Nullable
    private Long getDiffSetId(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        return pullRequestInfo != null ? pullRequestInfo.getDiffSetId() : null;
    }

    @Nullable
    private Instant getDiffSetEventCreated(LaunchVcsInfo vcsInfo) {
        var pullRequestInfo = vcsInfo.getPullRequestInfo();
        return pullRequestInfo != null ? pullRequestInfo.getDiffSetEventCreated() : null;
    }
}
