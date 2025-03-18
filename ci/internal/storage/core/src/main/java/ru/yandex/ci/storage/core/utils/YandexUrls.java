package ru.yandex.ci.storage.core.utils;

import javax.annotation.Nonnull;

import ru.yandex.ci.core.config.FlowFullId;
import ru.yandex.ci.core.flow.FlowUrls;
import ru.yandex.ci.storage.core.db.model.check.CheckEntity;
import ru.yandex.ci.storage.core.db.model.check_iteration.CheckIterationEntity;

public class YandexUrls {

    private static final FlowUrls FLOW_URLS = new FlowUrls("https://a.yandex-team.ru");

    private YandexUrls() {

    }

    public static String revisionUrl(String revision) {
        return "https://a.yandex-team.ru/arc_vcs/commit/" + revision;
    }

    public static String distPoolUrl(String pool) {
        return "https://datalens.yandex-team.ru/alc90mpai3u04-distbuild-pools?pool=" + pool;
    }

    public static String reviewUrl(long reviewId) {
        return "https://a.yandex-team.ru/review/%d/details".formatted(reviewId);
    }

    public static String ciBadgeUrl(CheckEntity.Id checkId) {
        return "https://a.yandex-team.ru/ci-card-preview/" + checkId.getId();
    }

    public static String ciBadgeUrl(@Nonnull Long checkId) {
        return "https://a.yandex-team.ru/ci-card-preview/" + checkId;
    }

    public static String ciExecutionProfiler(CheckIterationEntity.Id iterationId) {
        return ("https://datalens.yandex-team.ru/preview/7hom905swaes0-ci-autocheck-iteration-tasks-stages-new" +
                "?checkId=%d&iterationType=%s&iterationNumber=%d")
                .formatted(
                        iterationId.getCheckId().getId(),
                        iterationId.getIterationType().toString(),
                        iterationId.getNumber()
                );
    }

    public static String autocheckSandboxTasksUrl(CheckIterationEntity.Id iterationId) {
        return "https://sandbox.yandex-team.ru/tasks?limit=100&hints=" + iterationId.getCheckId().toString() +
                "%2F" + iterationId.getIterationType().name() + "&tags=AUTOCHECK";
    }

    public static String testenvSandboxTasksUrl(String testenvId) {
        var tags = "TESTENV-AUTOCHECK-JOB";
        return "https://sandbox.yandex-team.ru/tasks?limit=100&children=true&hidden=true&hints=%s&tags=%s"
                .formatted(testenvId.toUpperCase(), tags);
    }

    public static String testenvJobsSandboxTasksUrl(String testenvId) {
        var tags = "TESTENV-TE-JOB";
        return "https://sandbox.yandex-team.ru/tasks?limit=100&children=true&hidden=true&hints=%s&tags=%s"
                .formatted(testenvId.toUpperCase(), tags);
    }

    public static String testenvTimelineUrl(String testenvId) {
        return "https://testenv.yandex-team.ru/?database=" + testenvId + "&screen=timeline";
    }

    public static String largeSandboxTasksUrl(CheckIterationEntity.Id iterationId) {
        return "https://sandbox.yandex-team.ru/tasks?limit=100&children=true&hidden=true&hints=%d/%s&tags=%s"
                .formatted(iterationId.getCheckId().getId(), iterationId.getIterationType().name(), "AUTOCHECK-LARGE");
    }

    public static String nativeSandboxTasksUrl(CheckIterationEntity.Id iterationId) {
        return "https://sandbox.yandex-team.ru/tasks?limit=100&children=true&hidden=true&hints=%d/%s&tags=%s"
                .formatted(iterationId.getCheckId().getId(), iterationId.getIterationType().name(), "AUTOCHECK-NATIVE");
    }

    public static String sandboxTaskUrl(long sandboxTaskId) {
        return "https://sandbox.yandex-team.ru/task/%d/view".formatted(sandboxTaskId);
    }

    public static String distbuildProfilerUrl(long reviewId) {
        return "https://datalens.yandex-team.ru/0e0iocvqdgsuv-distbuildprofiler?review=" + reviewId;
    }

    public static String distbuildViewerUrl(@Nonnull String queryParam) {
        return "https://viewer-distbuild.n.yandex-team.ru/search?query=" + queryParam;
    }

    public static String autocheckFlow(FlowFullId flowFullId, int launchNumber) {
        return FLOW_URLS.toFlowLaunch("autocheck", flowFullId.getDir(), flowFullId.getId(), launchNumber);
    }

}
