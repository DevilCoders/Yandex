package yandex.cloud.team.integration.idm.service;

import yandex.cloud.team.integration.idm.model.response.RolesResponse;

public interface RoleService {

    boolean isSupportedRole(String role);

    RolesResponse getRoles();

}
