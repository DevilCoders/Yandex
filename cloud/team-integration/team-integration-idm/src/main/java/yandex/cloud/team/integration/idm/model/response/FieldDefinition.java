package yandex.cloud.team.integration.idm.model.response;

import lombok.Builder;
import lombok.Value;

@Builder
@Value
public class FieldDefinition {

    private static final String FILED_TYPE_CHARFIELD = "charfield";

    String slug;
    LocalizedString name;
    String type;
    boolean required;

    public static FieldDefinition requiredCharfield(String slug, LocalizedString name) {
        return new FieldDefinition(slug, name, FILED_TYPE_CHARFIELD, true);
    }

}
