package ru.yandex.ci.core.config.a.model;

import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

import javax.annotation.Nonnull;

import com.fasterxml.jackson.annotation.JsonAnyGetter;
import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonFormat;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonInclude;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.databind.annotation.JsonDeserialize;
import lombok.Builder;
import lombok.Value;
import lombok.With;

import ru.yandex.ci.ydb.Persisted;


@SuppressWarnings({"BoxedPrimitiveEquality", "ReferenceEquality"})
@Persisted
@Value
@Builder(toBuilder = true)
@JsonDeserialize(builder = Permissions.Builder.class)
@JsonIgnoreProperties(ignoreUnknown = true) // defaultOwnerAccess, defaultPermissionsForPrOwner
public class Permissions {

    public static final Permissions EMPTY = Permissions.builder().build();
    public static final List<PermissionForOwner> DEFAULT_PERMISSIONS_FOR_OWNER = List.of(PermissionForOwner.PR);

    @With
    @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
    @JsonProperty("default-permissions-for-owner")
    @JsonInclude(JsonInclude.Include.NON_NULL) // null and empty arrays are not the same
    List<PermissionForOwner> defaultPermissionsForOwner;

    @With
    @Nonnull
    @JsonIgnore
    Map<PermissionScope, List<PermissionRule>> permissions;

    @Nonnull
    @JsonAnyGetter
    public Map<PermissionScope, List<PermissionRule>> getPermissions() {
        return permissions;
    }

    public List<PermissionRule> getPermissions(PermissionScope scope) {
        return permissions.getOrDefault(scope, List.of());
    }

    public static class Builder {
        {
            permissions = new LinkedHashMap<>();
        }

        public Builder add(PermissionScope scope, PermissionRule... rules) {
            permissions.put(scope, List.of(rules));
            return this;
        }

        @JsonAnySetter
        @JsonFormat(with = JsonFormat.Feature.ACCEPT_SINGLE_VALUE_AS_ARRAY)
        public Builder add(String scope, List<PermissionRule> rules) {
            permissions.put(PermissionScope.ofJsonProperty(scope), rules);
            return this;
        }
    }
}
