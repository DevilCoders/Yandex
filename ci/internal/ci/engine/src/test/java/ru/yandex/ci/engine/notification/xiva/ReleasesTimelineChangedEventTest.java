package ru.yandex.ci.engine.notification.xiva;

import java.nio.file.Path;
import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.xiva.SendRequest;
import ru.yandex.ci.core.config.CiProcessId;

import static org.assertj.core.api.Assertions.assertThat;

class ReleasesTimelineChangedEventTest {

    @Test
    void getTopic() {
        var topic = new ReleasesTimelineChangedEvent(
                CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release-id"), List.of()
        ).getTopic();
        assertThat(topic).isEqualTo("releases-timeline@ci@release-id");
    }

    @Test
    void toXivaSendRequest() throws JsonProcessingException {
        var sendRequest = new ReleasesTimelineChangedEvent(
                CiProcessId.ofRelease(Path.of("ci/a.yaml"), "release-id"),
                List.of("trunk", "release/ci/1")
        ).toXivaSendRequest();
        assertThat(sendRequest).isEqualTo(new SendRequest(
                XivaBaseEvent.getMapper().readTree("""
                        {
                            "type": "releases_timeline_changed"
                        }
                        """),
                List.of("trunk", "release_2fci_2f1")
        ));
    }

}
