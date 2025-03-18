package ru.yandex.ci.client.blackbox;

import java.util.Set;

import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Value;

import ru.yandex.ci.client.blackbox.jackson.StringToSetDeserializer;

@Value
public class OAuthResponse {
    String login;
    OAuthInfo oauth;

    @Value
    public static class OAuthInfo {
        @JsonDeserialize(using = StringToSetDeserializer.class)
        Set<String> scope;
    }
}
