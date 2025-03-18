package ru.yandex.ci.core.config;

import java.util.Objects;

import javax.annotation.Nullable;

import com.fasterxml.jackson.annotation.JsonProperty;
import com.google.common.base.Preconditions;

import ru.yandex.ci.core.security.YavToken;
import ru.yandex.ci.ydb.Persisted;

@Persisted
public class ConfigSecurityState {
    private final YavToken.Id yavTokenUuid;
    private final ValidationStatus validationStatus;

    public ConfigSecurityState(@JsonProperty("yavTokenUuid") YavToken.Id yavTokenUuid,
                               @JsonProperty("validationStatus") ValidationStatus validationStatus) {
        if (validationStatus.isValid()) {
            Preconditions.checkArgument(yavTokenUuid != null);
        } else {
            Preconditions.checkArgument(yavTokenUuid == null);
        }
        this.yavTokenUuid = yavTokenUuid;
        this.validationStatus = validationStatus;
    }

    @Nullable
    public YavToken.Id getYavTokenUuid() {
        return yavTokenUuid;
    }

    public ValidationStatus getValidationStatus() {
        return validationStatus;
    }

    public boolean isValid() {
        return validationStatus.isValid();
    }

    @Persisted
    public enum ValidationStatus {
        NEW_TOKEN("New token delegated for this revision", true),
        CONFIG_NOT_CHANGED("Configuration wasn't change", true),
        VALID_USER("User belongs to ABC-group", true),
        VALID_SOX_USER("User belongs to ABC-group and sox scope", true),
        USER_IS_ADMIN("User belongs to admin group", true),
        OWNER_APPROVE("Changes approved (shipped) by config owner", true),
        USER_HAS_TOKEN("User has own token", true),
        INVALID_USER("User has no rights to edit configuration", false),
        SECRET_CHANGED("Secret was changed", false),
        ABC_SERVICE_UNKNOWN("ABC-service unknown: cannot find service slug in ABC", false),
        ABC_SERVICE_CHANGED("ABC-service was changed", false),
        SOX_NOT_APPROVED("SOX approval required", false),
        TOKEN_NOT_FOUND("No token was found for this configuration", false),
        TOKEN_INVALID("Token is invalid, must be UUID in YAV, like sec-..., without spaces and new lines", false),
        INVALID_CONFIG("Invalid config", false),
        NOT_CI("Not CI config", false);

        private final String message;
        private final boolean valid;

        ValidationStatus(String message, boolean valid) {
            this.message = message;
            this.valid = valid;
        }

        public String getMessage() {
            return message;
        }

        public boolean isValid() {
            return valid;
        }
    }

    @Override
    public String toString() {
        return "ConfigSecurityState{" +
                "yavTokenUuid=" + yavTokenUuid +
                ", validationStatus=" + validationStatus +
                '}';
    }

    @Override
    public boolean equals(Object o) {
        if (this == o) {
            return true;
        }
        if (!(o instanceof ConfigSecurityState)) {
            return false;
        }
        ConfigSecurityState that = (ConfigSecurityState) o;
        return Objects.equals(yavTokenUuid, that.yavTokenUuid) &&
                validationStatus == that.validationStatus;
    }

    @Override
    public int hashCode() {
        return Objects.hash(yavTokenUuid, validationStatus);
    }
}
