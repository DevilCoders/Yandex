package ru.yandex.ci.engine.notification.xiva;

import java.util.List;

import com.fasterxml.jackson.core.JsonProcessingException;
import org.junit.jupiter.api.Test;

import ru.yandex.ci.client.xiva.SendRequest;

import static org.assertj.core.api.Assertions.assertThat;

class ProjectStatisticsChangedEventTest {

    @Test
    void getTopic() {
        var topic = new ProjectStatisticsChangedEvent("ci").getTopic();
        assertThat(topic).isEqualTo("project@ci");
    }

    @Test
    void toXivaSendRequest() throws JsonProcessingException {
        var sendRequest = new ProjectStatisticsChangedEvent("ci").toXivaSendRequest();
        assertThat(sendRequest).isEqualTo(new SendRequest(
                XivaBaseEvent.getMapper().readTree("""
                        {
                            "type": "project_statistics_changed"
                        }
                        """),
                List.of()
        ));
    }

}
