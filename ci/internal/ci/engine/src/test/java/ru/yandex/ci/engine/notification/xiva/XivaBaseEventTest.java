package ru.yandex.ci.engine.notification.xiva;

import org.junit.jupiter.api.Test;

import static org.assertj.core.api.Assertions.assertThat;
import static ru.yandex.ci.engine.notification.xiva.XivaBaseEvent.FORBIDDEN_TAG_CHAR_PATTERN;
import static ru.yandex.ci.engine.notification.xiva.XivaBaseEvent.FORBIDDEN_TOPIC_CHAR_PATTERN;
import static ru.yandex.ci.engine.notification.xiva.XivaBaseEvent.TOPIC_MAX_LENGTH;

class XivaBaseEventTest {

    @Test
    void trimLongTopic_shouldTrimAndAddHash() {
        assertThat(XivaBaseEvent.trimLongTopic("a", "a")).isEqualTo("a");

        var originalTopic = "a".repeat(TOPIC_MAX_LENGTH);
        assertThat(XivaBaseEvent.trimLongTopic(originalTopic, originalTopic))
                .isEqualTo(originalTopic);

        originalTopic = "a".repeat(TOPIC_MAX_LENGTH + 1);
        assertThat(XivaBaseEvent.trimLongTopic(originalTopic, originalTopic))
                .isEqualTo("a".repeat(TOPIC_MAX_LENGTH - 9) + "@bff658f1");
    }

    @Test
    void trimLongTopic_shouldHashOriginalTopic() {
        var originalTopic = "a".repeat(2000);
        assertThat(2000).isGreaterThan(TOPIC_MAX_LENGTH);

        assertThat(XivaBaseEvent.trimLongTopic(originalTopic, originalTopic))
                .isEqualTo("a".repeat(TOPIC_MAX_LENGTH - 9) + "@2a59a7c2")
                .hasSize(TOPIC_MAX_LENGTH);

        assertThat(XivaBaseEvent.trimLongTopic("x".repeat(2000), originalTopic))
                .isEqualTo("x".repeat(TOPIC_MAX_LENGTH - 9) + "@2a59a7c2")
                .hasSize(TOPIC_MAX_LENGTH);
    }

    @Test
    void constructor_shouldEncodeTopic() {
        var originalTopic = "topic@ci/a.yaml@" + "x".repeat(TOPIC_MAX_LENGTH + 1);
        var encodedTopic = new XivaBaseEvent(originalTopic) {
            @Override
            public Type getType() {
                return Type.PROJECT_STATISTICS_CHANGED;
            }
        }.getTopic();
        assertThat(encodedTopic)
                .isEqualTo("topic@ci.2fa.yaml@".strip() + "x".repeat(173) + "@a2f3923f")
                .hasSize(TOPIC_MAX_LENGTH);
    }

    @Test
    void encodeTopic() {
        var topicBuilder = new StringBuilder();
        for (char i = 0; i < 255; i++) {
            topicBuilder.append(i);
        }
        var topic = topicBuilder.toString();
        assertThat(FORBIDDEN_TOPIC_CHAR_PATTERN.matcher(topic).find()).isTrue();
        assertThat(FORBIDDEN_TOPIC_CHAR_PATTERN.matcher(XivaBaseEvent.encodeTopic(topic)).find()).isFalse();
    }

    @Test
    void encodeTag() {
        var tagBuilder = new StringBuilder();
        for (char i = 0; i < 255; i++) {
            tagBuilder.append(i);
        }
        var tag = tagBuilder.toString();
        assertThat(FORBIDDEN_TAG_CHAR_PATTERN.matcher(tag).find()).isTrue();
        assertThat(FORBIDDEN_TAG_CHAR_PATTERN.matcher(XivaBaseEvent.encodeTag(tag)).find()).isFalse();
    }

}
