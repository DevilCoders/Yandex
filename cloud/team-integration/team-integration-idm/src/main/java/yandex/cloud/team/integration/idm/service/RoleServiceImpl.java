package yandex.cloud.team.integration.idm.service;

import java.io.IOException;
import java.io.InputStream;
import java.io.UncheckedIOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.StringTokenizer;

import javax.inject.Inject;

import lombok.Getter;
import yandex.cloud.team.integration.idm.config.IdmConfig;
import yandex.cloud.team.integration.idm.model.response.RoleDefinition;
import yandex.cloud.team.integration.idm.model.response.RolesResponse;
import yandex.cloud.util.Json;

public class RoleServiceImpl implements RoleService {

    private static final String BUILT_IN_ROLES = "/roles.yaml";

    @Inject
    private static IdmConfig idmConfig;

    @Getter
    private final RolesResponse roles = loadRoles();

    @Override
    public boolean isSupportedRole(String role) {
        StringTokenizer st = new StringTokenizer(role, "/");
        return isSupportedRole(roles.getRoles(), st);
    }

    private static boolean isSupportedRole(RoleDefinition roleDef, StringTokenizer st) {
        if (st.hasMoreTokens() && st.nextToken().equals(roleDef.getSlug()) && st.hasMoreTokens()) {
            var childRole = roleDef.getValues().get(st.nextToken());
            return childRole != null && (!st.hasMoreTokens() || isSupportedRole(childRole.getRoles(), st));
        }
        return false;
    }

    private RolesResponse loadRoles() {
        try (var rolesStream = getRolesStream()) {
            return Json.yaml.readerFor(Json.yaml.getTypeFactory().constructType(RolesResponse.class))
                .readValue(rolesStream);
        } catch (IOException e) {
            throw new UncheckedIOException(e);
        }
    }

    private static InputStream getRolesStream() throws IOException {
        if (idmConfig == null || idmConfig.getRolesPath() == null) {
            return RoleServiceImpl.class.getResourceAsStream(BUILT_IN_ROLES);
        }
        return Files.newInputStream(Path.of(idmConfig.getRolesPath()));
    }

}
