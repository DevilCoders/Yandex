package ru.yandex.ci.engine.notification.xiva;

import java.nio.charset.StandardCharsets;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Preconditions;
import com.google.common.hash.Hashing;
import lombok.Getter;
import lombok.ToString;

import ru.yandex.ci.client.xiva.SendRequest;

@ToString
public abstract class XivaBaseEvent {

    private static final ObjectMapper MAPPER = new ObjectMapper();
    private static final Pattern TOPIC_PATTERN = Pattern.compile("^[a-zA-Z0-9][a-zA-Z0-9@.-]*$");
    static final Pattern FORBIDDEN_TOPIC_CHAR_PATTERN = Pattern.compile("(?<char>[^a-zA-Z0-9@.-])");
    static final Pattern FORBIDDEN_TAG_CHAR_PATTERN = Pattern.compile("(?<char>[^a-zA-Z0-9_])");
    static final int TOPIC_MAX_LENGTH = 200;

    @Nonnull
    @Getter(onMethod_ = @JsonIgnore)
    private final String topic;

    @Nonnull
    @Getter(onMethod_ = @JsonIgnore)
    private final List<String> tags;

    XivaBaseEvent(String topic) {
        this(topic, List.of());
    }

    XivaBaseEvent(String topic, List<String> tags) {
        var encodedTopic = trimLongTopic(encodeTopic(topic), topic);
        Preconditions.checkArgument(
                TOPIC_PATTERN.matcher(encodedTopic).matches(),
                "Encoded topic '%s' doesn't match pattern %s, original topic %s",
                encodedTopic, TOPIC_PATTERN.pattern(), topic
        );
        this.topic = encodedTopic;
        this.tags = tags.stream()
                .map(XivaBaseEvent::encodeTag)
                .distinct()
                .collect(Collectors.toList());
    }

    SendRequest toXivaSendRequest() {
        return new SendRequest(MAPPER.valueToTree(this), tags);
    }

    public abstract Type getType();

    static String encodeTopic(@Nonnull String source) {
        return encode(FORBIDDEN_TOPIC_CHAR_PATTERN.matcher(source), ".");
    }

    static String encodeTag(@Nonnull String source) {
        return encode(FORBIDDEN_TAG_CHAR_PATTERN.matcher(source), "_");
    }

    private static String encode(Matcher matcher, String specialCharPrefix) {
        var sb = new StringBuilder();
        while (matcher.find()) {
            var charCode = matcher.group("char").codePointAt(0);
            matcher.appendReplacement(sb, specialCharPrefix + Integer.toHexString(charCode));
        }
        matcher.appendTail(sb);
        return sb.toString();
    }

    static String trimLongTopic(@Nonnull String encodedTopic, @Nonnull String originalTopic) {
        if (encodedTopic.length() <= TOPIC_MAX_LENGTH) {
            return encodedTopic;
        }
        var hash = Hashing.murmur3_32_fixed().hashString(originalTopic, StandardCharsets.UTF_8)
                .toString();
        return encodedTopic.substring(0, TOPIC_MAX_LENGTH - hash.length() - 1) + "@" + hash;
    }

    static ObjectMapper getMapper() {
        return MAPPER;
    }

    enum Type {
        @JsonProperty("project_statistics_changed")
        PROJECT_STATISTICS_CHANGED,
        @JsonProperty("releases_timeline_changed")
        RELEASES_TIMELINE_CHANGED;
    }

}
