package ru.yandex.ci.client.yav.model;

import java.util.List;

import javax.annotation.Nullable;

import lombok.Builder;
import lombok.Value;

@Value
public class YavTokenizedRequest {

    List<TokenizedRequest> tokenizedRequests;

    @Builder
    @Value
    public static class TokenizedRequest {
        @Nullable
        String token;
        @Nullable
        String signature;
        @Nullable
        String serviceTicket;
        @Nullable
        String secretVersion;
    }
}
