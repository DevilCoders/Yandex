package ru.yandex.ci.core.common;

import java.beans.Transient;
import java.util.UUID;

import com.fasterxml.jackson.annotation.JsonIgnore;

public interface SourceCodeEntity {
    /**
     * Gets unique persistent source code identifier.
     * <p>
     * Generate id here https://www.uuidgenerator.net/version4 and place it to UUID.fromString().
     *
     * @return Unique persistent identifier.
     */
    @Transient
    @JsonIgnore
    UUID getSourceCodeId();
}
