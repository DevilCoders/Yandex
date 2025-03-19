package yandex.cloud.team.integration.idm.model.response;

import lombok.Builder;
import lombok.Value;

@Builder
@Value
public class RolesResponse {

    int code;
    RoleDefinition roles;

}

