package ru.yandex.ci.storage.core.clickhouse.sp;

import java.util.Objects;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

@JsonIgnoreProperties(ignoreUnknown = true)
public class TestOutput {
    private final String hash;
    private final String url;
    private final Integer size;

    @SuppressWarnings("ParameterMissingNullable")
    public TestOutput() {
        this(null, null, null);
    }

    public TestOutput(@JsonProperty("hash") String hash,
                      @JsonProperty("url") String url,
                      @JsonProperty("size") Integer size) {
        this.hash = hash;
        this.url = url;
        this.size = size;
    }

    public String getHash() {
        return hash;
    }

    public String getUrl() {
        return url;
    }

    public Integer getSize() {
        return size;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj == this) {
            return true;
        }

        if (!(obj instanceof TestOutput)) {
            return false;

        }
        TestOutput that = (TestOutput) obj;
        return Objects.equals(getHash(), that.getHash())
                && Objects.equals(getUrl(), that.getUrl())
                && Objects.equals(getSize(), that.getSize());
    }

    @Override
    public int hashCode() {
        return Objects.hash(getHash(), getUrl(), getSize());
    }
}
