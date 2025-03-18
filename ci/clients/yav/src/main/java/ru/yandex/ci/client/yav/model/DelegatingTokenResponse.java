package ru.yandex.ci.client.yav.model;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.MoreObjects;

public class DelegatingTokenResponse extends YavResponse {
    @Nullable
    private final String token;
    @Nullable
    private final String tokenUuid;

    @JsonCreator
    public DelegatingTokenResponse(@JsonProperty("status") @Nullable Status status,
                                   @JsonProperty("code") @Nullable String code,
                                   @JsonProperty("message") @Nullable String message,
                                   @JsonProperty("token") @Nullable String token,
                                   @JsonProperty("token_uuid") @Nullable String tokenUuid) {
        super(status, code, message);
        this.token = token;
        this.tokenUuid = tokenUuid;
    }

    @Nullable
    public String getToken() {
        return token;
    }

    @Nullable
    public String getTokenUuid() {
        return tokenUuid;
    }

    @Override
    protected MoreObjects.ToStringHelper toStringHelper() {
        return super.toStringHelper()
                .add("token", token)
                .add("tokenUuid", tokenUuid);
    }
}
