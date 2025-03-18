package ru.yandex.ci.core.config.a.model;

import java.util.List;
import java.util.Objects;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Singular;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
@JsonDeserialize(builder = PermissionRule.Builder.class)
public class PermissionRule {

    @Nonnull
    @JsonProperty("service")
    String abcService;

    @Singular
    @JsonProperty("scope")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> abcScopes;

    @Singular
    @JsonProperty("role")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> abcRoles;

    @Singular
    @JsonProperty("duty")
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    List<String> abcDuties;

    public List<String> getAbcScopes() {
        return Objects.requireNonNullElse(abcScopes, List.of());
    }

    public List<String> getAbcRoles() {
        return Objects.requireNonNullElse(abcRoles, List.of());
    }

    public List<String> getAbcDuties() {
        return Objects.requireNonNullElse(abcDuties, List.of());
    }

    @Override
    public String toString() {
        String result = "[ABC Service = " + abcService;
        if (!abcScopes.isEmpty()) {
            result += ", scopes = " + abcScopes;
        }
        if (!abcRoles.isEmpty()) {
            result += ", roles = " + abcRoles;
        }
        if (!abcDuties.isEmpty()) {
            result += ", duties = " + abcDuties;
        }
        result += "]";
        return result;
    }

    //

    public static PermissionRule ofScopes(String service, String... scopes) {
        return ofScopes(service, List.of(scopes));
    }

    public static PermissionRule ofScopes(String service, List<String> scopes) {
        return of(service, scopes, List.of(), List.of());
    }

    public static PermissionRule ofRoles(String service, String... roles) {
        return ofRoles(service, List.of(roles));
    }

    public static PermissionRule ofRoles(String service, List<String> roles) {
        return of(service, List.of(), roles, List.of());
    }

    public static PermissionRule ofDuties(String service, String... duties) {
        return ofDuties(service, List.of(duties));
    }

    public static PermissionRule ofDuties(String service, List<String> duties) {
        return of(service, List.of(), List.of(), duties);
    }

    public static PermissionRule of(String service, List<String> scopes, List<String> roles, List<String> duties) {
        return new PermissionRule(service, scopes, roles, duties);
    }

    public static class Builder {
        public Builder() {
        }

        public Builder(String abcService) {
            this.abcService(abcService);
        }

    }

}
