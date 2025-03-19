package yandex.cloud.team.integration.idm.model.response;

import java.util.List;
import java.util.Map;

import lombok.Builder;
import lombok.Value;

@Builder
@Value
public class RoleDefinition {

    String slug;
    LocalizedString name;
    LocalizedString help;
    RoleDefinition roles;
    Map<String, RoleDefinition> values;
    List<FieldDefinition> fields;
    Boolean visibility;

}

