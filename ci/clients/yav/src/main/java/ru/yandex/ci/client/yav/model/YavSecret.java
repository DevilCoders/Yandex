package ru.yandex.ci.client.yav.model;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.Optional;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.MoreObjects;
import lombok.Value;

public class YavSecret extends YavResponse {
    public static final String CI_TOKEN = "ci.token";
    private static final String SECURITY_DOC_URL = "https://docs.yandex-team.ru/ci/secret";

    @Nullable
    private final String secretVersion;
    @Nullable
    private String secret;

    @JsonIgnore
    private final Map<String, String> value;

    @JsonCreator
    public YavSecret(
            @JsonProperty("status") @Nullable Status status,
            @JsonProperty("code") @Nullable String code,
            @JsonProperty("message") @Nullable String message,
            @JsonProperty("secret_version") @Nullable String secretVersion,
            @JsonProperty("value") @Nullable List<KeyValue> value) {
        super(status, code, message);
        this.secretVersion = secretVersion;
        this.value = unmap(value);
    }

    @Nullable
    public String getSecretVersion() {
        return secretVersion;
    }

    @Nullable
    public String getSecret() {
        return secret;
    }

    public void setSecret(@Nullable String secret) {
        this.secret = secret;
    }

    public Optional<String> getValueByKey(String key) {
        return Optional.ofNullable(value.get(key));
    }

    public String getCiToken() {
        return getValueByKey(YavSecret.CI_TOKEN)
                .orElseThrow(() -> new IllegalStateException(String.format(
                        "Token %s not found in secret %s version %s. Check %s",
                        YavSecret.CI_TOKEN,
                        getSecret(),
                        Objects.requireNonNullElse(getSecretVersion(), "latest"),
                        SECURITY_DOC_URL
                )));
    }


    @Override
    protected MoreObjects.ToStringHelper toStringHelper() {
        return super.toStringHelper()
                .add("secret", secret)
                .add("secretVersion", secretVersion);
    }

    @Override
    public int hashCode() {
        return Objects.hash(super.hashCode(), value, secret, secretVersion);
    }

    @Override
    public boolean equals(Object obj) {
        if (this == obj) {
            return true;
        }

        if (!(obj instanceof YavSecret)) {
            return false;
        }

        YavSecret that = (YavSecret) obj;

        return Objects.equals(value, that.value) &&
                Objects.equals(secret, that.secret) &&
                Objects.equals(secretVersion, that.secretVersion);
    }

    private static Map<String, String> unmap(@Nullable List<KeyValue> value) {
        if (value == null) {
            return Map.of();
        }
        Map<String, String> values = new LinkedHashMap<>(value.size());
        for (var e : value) {
            values.put(e.getKey(), e.getValue());
        }
        return values;
    }

    @Value
    public static class KeyValue {
        String key;
        String value;
    }

}
