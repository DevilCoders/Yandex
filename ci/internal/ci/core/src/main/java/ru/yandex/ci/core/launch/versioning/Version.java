package ru.yandex.ci.core.launch.versioning;

import java.util.Comparator;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.annotation.Nonnull;
import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonCreator;
import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;
import lombok.Builder;
import lombok.Value;

import ru.yandex.ci.ydb.Persisted;

@Persisted
@Value
@Builder
public class Version implements Comparable<Version> {
    private static final Pattern VERSION_PATTERN = Pattern.compile("(?<major>\\d+)(?:\\.(?<minor>\\d+))?");

    String major;
    @Nullable
    String minor;

    @JsonCreator
    private Version(@JsonProperty("major") String major,
                    @JsonProperty("minor") @Nullable String minor) {
        Preconditions.checkNotNull(major);
        Preconditions.checkArgument(!major.isBlank());
        Preconditions.checkArgument(minor == null || !minor.isBlank(), "minor version can be not blank or null");
        this.major = major;
        this.minor = minor;
    }

    public static Version majorMinor(String major, String minor) {
        return new Version(major, minor);
    }

    public static Version major(String major) {
        return new Version(major, null);
    }

    // backward compatibility for unstructured version string
    public static Version fromAsString(String versionString) {
        Matcher matcher = VERSION_PATTERN.matcher(versionString);
        if (matcher.matches()) {
            String major = matcher.group("major");
            String minor = matcher.group("minor");
            if (minor != null) {
                return Version.majorMinor(major, minor);
            }
            return Version.major(major);
        }
        return Version.major(versionString);
    }


    public String asString() {
        if (minor != null) {
            return major + "." + minor;
        }
        return major;
    }

    @Override
    public int compareTo(@Nonnull Version o) {
        return Comparator.comparing(Version::getMajor)
                .thenComparing(Version::getMinor, Comparator.nullsFirst(Comparator.naturalOrder()))
                .compare(this, o);
    }
}
