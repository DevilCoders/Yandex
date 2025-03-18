package ru.yandex.ci.storage.core.clickhouse.sp;

import java.util.List;
import java.util.Objects;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class Owners {
    private final List<String> logins;
    private final List<String> groups;

    public Owners() {
        logins = List.of();
        groups = List.of();
    }

    @JsonCreator
    public Owners(@JsonProperty("logins") List<String> logins,
                  @JsonProperty("groups") List<String> groups) {
        this.logins = internStrings(logins);
        this.groups = internStrings(groups);
    }

    public List<String> getLogins() {
        return logins;
    }

    public List<String> getGroups() {
        return groups;
    }

    private static List<String> internStrings(@Nullable List<String> strings) {
        return strings == null ? List.of()
                : strings.stream().map(String::intern).collect(Collectors.toUnmodifiableList());
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) {
            return true;
        }

        if (!(obj instanceof Owners)) {
            return false;
        }

        Owners that = (Owners) obj;

        return Objects.equals(getLogins(), that.getLogins())
                && Objects.equals(getGroups(), that.getGroups());
    }

    @Override
    public int hashCode() {
        return Objects.hash(getLogins(), getGroups());
    }
}
