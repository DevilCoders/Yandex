package yandex.cloud.team.integration.idm.model.request;

import com.fasterxml.jackson.annotation.JsonProperty;
import lombok.AllArgsConstructor;
import lombok.Value;
import yandex.cloud.team.integration.idm.Validation;
import yandex.cloud.team.integration.idm.exception.InvalidServiceSpecException;
import yandex.cloud.util.Json;

/**
 * Add/remove role additional parameters. Format is service_spec: slug-id-group, for example
 * {@code {"service_spec": "my-service-slug-2-4"}}.
 */
@Value
public class FieldsParameter {

    public static final char DELIMITER = '-';

    String abcSlug;
    int abcId;

    public static FieldsParameter valueOf(String value) {
        if (value == null) {
            return null;
        }

        Raw raw = Json.fromJson(Raw.class, value);
        Validation.checkNotEmptyField(raw.serviceSpec, "fields", "service_spec");

        var groupIdx = raw.serviceSpec.lastIndexOf(DELIMITER);
        var idIdx = raw.serviceSpec.lastIndexOf(DELIMITER, groupIdx - 1);

        if (idIdx < 0) {
            throw InvalidServiceSpecException.of(raw.serviceSpec);
        }

        String abcSlug = raw.serviceSpec.substring(0, idIdx);
        int abcId;
        try {
            abcId = Integer.parseInt(raw.serviceSpec.substring(idIdx + 1, groupIdx));
        } catch (NumberFormatException ex) {
            throw InvalidServiceSpecException.of(raw.serviceSpec);
        }

        return new FieldsParameter(abcSlug, abcId);
    }

    @AllArgsConstructor
    private static class Raw {

        @JsonProperty("service_spec")
        String serviceSpec;

    }

}

