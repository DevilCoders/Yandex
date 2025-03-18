package ru.yandex.ci.flow.utils;

import java.nio.file.Path;

import org.junit.jupiter.api.Test;

import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.versioning.Version;

import static org.assertj.core.api.Assertions.assertThat;

class UrlServiceTest {

    private final UrlService urlService = new UrlService("http://localhost/");
    public static final CiProcessId RELEASE_PROCESS_ID = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "ci-release");
    public static final CiProcessId FLOW_PROCESS_ID = CiProcessId.ofFlow(Path.of("ci/a.yaml"), "ci-release");

    @Test
    void toFlowLaunch() {
        String resultUrl = urlService.toFlowLaunch("cidemo", LaunchId.of(FLOW_PROCESS_ID, 1));
        assertThat(resultUrl).isEqualTo(
                "http://localhost"
                        + "/projects/cidemo/ci/actions/flow"
                        + "?dir=ci"
                        + "&id=ci-release"
                        + "&number=1"
        );
    }

    @Test
    void toReleaseLaunch() {
        String resultUrl = urlService.toReleaseLaunch("cidemo", LaunchId.of(RELEASE_PROCESS_ID, 1),
                Version.fromAsString("2"));
        assertThat(resultUrl).isEqualTo(
                "http://localhost/projects/cidemo"
                        + "/ci/releases/flow"
                        + "?dir=ci"
                        + "&id=ci-release"
                        + "&version=2"
        );
    }

    @Test
    void toJobInReleaseLaunch() {
        String resultUrl = urlService.toJobInReleaseLaunch(
                "cidemo", LaunchId.of(RELEASE_PROCESS_ID, 1), Version.fromAsString("2.3"), "testing-api-deploy", 2
        );
        assertThat(resultUrl).isEqualTo(
                "http://localhost/projects/cidemo"
                        + "/ci/releases/flow"
                        + "?dir=ci"
                        + "&id=ci-release"
                        + "&version=2.3"
                        + "&selectedJob=testing-api-deploy"
                        + "&launchNumber=2"
        );
    }

    @Test
    void toJobInFlowLaunch() {
        var launchId = LaunchId.of(FLOW_PROCESS_ID, 1);
        String resultUrl = urlService.toJobInFlowLaunch("cidemo", launchId, "testing-api-deploy", 2);
        assertThat(resultUrl).isEqualTo(
                "http://localhost"
                        + "/projects/cidemo/ci/actions/flow"
                        + "?dir=ci"
                        + "&id=ci-release"
                        + "&number=1"
                        + "&selectedJob=testing-api-deploy"
                        + "&launchNumber=2"
        );
    }

    @Test
    void toReleaseTimelineUrl() {
        String resultUrl = urlService.toReleaseTimelineUrl("cidemo", RELEASE_PROCESS_ID);
        assertThat(resultUrl).isEqualTo(
                "http://localhost/projects/cidemo"
                        + "/ci/releases/timeline"
                        + "?dir=ci"
                        + "&id=ci-release"
        );
    }

}
