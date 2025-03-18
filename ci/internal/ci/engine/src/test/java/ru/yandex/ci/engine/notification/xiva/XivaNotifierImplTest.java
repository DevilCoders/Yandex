package ru.yandex.ci.engine.notification.xiva;

import java.nio.file.Path;
import java.util.List;

import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.mockserver.integration.ClientAndServer;
import org.mockserver.model.HttpStatusCode;
import org.springframework.beans.factory.annotation.Autowired;
import org.springframework.http.HttpMethod;
import org.springframework.test.context.ContextConfiguration;

import ru.yandex.ci.CommonTestBase;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.launch.LaunchId;
import ru.yandex.ci.core.launch.LaunchState;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.engine.proto.ProtoMappers;
import ru.yandex.ci.engine.spring.clients.XivaClientTestConfig;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;
import static org.mockserver.model.JsonBody.json;
import static ru.yandex.ci.engine.notification.xiva.XivaBaseEvent.Type.PROJECT_STATISTICS_CHANGED;

@ContextConfiguration(classes = {
        XivaClientTestConfig.class,
        XivaNotifierImpl.class
})
class XivaNotifierImplTest extends CommonTestBase {

    private static final CiProcessId PROCESS_ID = CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release-id");

    @Autowired
    ClientAndServer xivaServer;

    @Autowired
    XivaNotifierImpl xivaNotifier;

    @BeforeEach
    void setUp() {
        xivaServer.reset();
        xivaServer.when(request("/v2/send").withMethod(HttpMethod.POST.name()))
                .respond(response().withStatusCode(HttpStatusCode.OK_200.code()));
    }

    @Test
    void onLaunchStateChanged() {
        var oldLaunch = TestData.launchBuilder()
                .project("ci")
                .launchId(LaunchId.of(PROCESS_ID, 1))
                .status(LaunchState.Status.RUNNING)
                .build();
        var updatedLaunch = oldLaunch.toBuilder()
                .status(LaunchState.Status.SUCCESS)
                .build();
        xivaNotifier.sendOnLaunchStateChanged(updatedLaunch, oldLaunch).join();

        var projectStatisticsChangedRequest = request("/v2/send")
                .withMethod(HttpMethod.POST.name())
                .withQueryStringParameter("service", "ci-unit-test")
                .withQueryStringParameter("topic", "project@ci")
                .withQueryStringParameter("event", "project_statistics_changed")
                .withBody(json("""
                        {
                            "payload": {
                                "type": "project_statistics_changed"
                            },
                            "tags": []
                        }
                        """
                ));
        xivaServer.verify(projectStatisticsChangedRequest);
        xivaServer.clear(projectStatisticsChangedRequest);

        var timelineChangedRequest = request("/v2/send")
                .withMethod(HttpMethod.POST.name())
                .withQueryStringParameter("service", "ci-unit-test")
                .withQueryStringParameter("topic", "releases-timeline@ci@release-id")
                .withQueryStringParameter("event", "releases_timeline_changed")
                .withBody(json("""
                        {
                            "payload": {
                                "type": "releases_timeline_changed"
                            },
                            "tags": ["trunk"]
                        }
                        """
                ));
        xivaServer.verify(timelineChangedRequest);
        xivaServer.clear(timelineChangedRequest);

        xivaServer.verifyZeroInteractions();
    }

    @Test
    void toXivaSubscription() {
        assertThat(xivaNotifier.toXivaSubscription(new XivaTestEvent("topic")))
                .isEqualTo(ProtoMappers.toXivaSubscription("ci-unit-test", "topic"));

        assertThat(xivaNotifier.toXivaSubscription(new XivaTestEvent("topic", List.of("tag1"))))
                .isEqualTo(ProtoMappers.toXivaSubscription("ci-unit-test:tag1", "topic"));

        assertThat(xivaNotifier.toXivaSubscription(new XivaTestEvent("topic", List.of("tag1", "tag2"))))
                .isEqualTo(ProtoMappers.toXivaSubscription("ci-unit-test:tag1+tag2", "topic"));
    }

    private static class XivaTestEvent extends XivaBaseEvent {

        XivaTestEvent(String topic) {
            super(topic);
        }

        XivaTestEvent(String topic, List<String> tags) {
            super(topic, tags);
        }

        @Override
        public Type getType() {
            return PROJECT_STATISTICS_CHANGED;
        }
    }


}
