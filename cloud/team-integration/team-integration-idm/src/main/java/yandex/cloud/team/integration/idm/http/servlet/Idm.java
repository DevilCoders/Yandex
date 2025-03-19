package yandex.cloud.team.integration.idm.http.servlet;

import javax.inject.Inject;
import javax.ws.rs.FormParam;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

import lombok.extern.log4j.Log4j2;
import yandex.cloud.team.integration.abc.ResolveServiceFacade;
import yandex.cloud.team.integration.idm.Validation;
import yandex.cloud.team.integration.idm.exception.BadRoleException;
import yandex.cloud.team.integration.idm.model.request.FieldsParameter;
import yandex.cloud.team.integration.idm.model.request.RoleParameter;
import yandex.cloud.team.integration.idm.model.response.GenericResponse;
import yandex.cloud.team.integration.idm.service.BindingService;
import yandex.cloud.team.integration.idm.service.RoleService;
import yandex.cloud.team.integration.idm.service.SubjectService;
import yandex.cloud.ti.abc.AbcServiceCloud;
import yandex.cloud.ti.yt.abc.client.AbcServiceNotFoundException;
import yandex.cloud.util.Json;

/**
 * IDM client for Yandex.Cloud.
 *
 * @see <a href="https://wiki.yandex-team.ru/Intranet/idm/">IDM docs</a>
 */
@Log4j2
@Path("/idm/")
@Produces(MediaType.APPLICATION_JSON)
public class Idm {

    private static final String IDM_ROLE_PREFIX = "yc.yandex-team.ru.";

    @Inject
    private static ResolveServiceFacade resolveServiceFacade;

    @Inject
    private static SubjectService subjectService;

    @Inject
    private static RoleService roleService;

    @Inject
    private static BindingService bindingService;

    /**
     * Returns the list of supported roles.
     *
     * <p>Example: {@code curl https://iam.cloud.yandex-team.ru/idm/info/}</p>
     *
     * @return roles hierarchy in the Json format
     */
    @Path("info")
    @GET
    public Object info() {
        return Response.ok(Json.toJson(roleService.getRoles())).build();
    }

    /**
     * Grants a role to an user specified by the login parameter for a ABC id specified in the fields parameter.
     *
     * @return {code: 0} for success, the error description otherwise.
     */
    @Path("add-role")
    @POST
    public Response addRole(
            @FormParam("login") String login,
            @FormParam("path") String path,
            @FormParam("fields") FieldsParameter fields,
            @FormParam("role") RoleParameter role
    ) {
        validateParameters(login, path, fields, role);

        AbcServiceCloud abcServiceCloud;
        try {
            abcServiceCloud = resolveServiceFacade.resolveAbcServiceCloudByAbcServiceId(fields.getAbcId());
        } catch (AbcServiceNotFoundException ex) {
            return abcServiceNotFoundResponse(ex.getMessage());
        }
        var roleName = getRoleName(role);
        var subject = subjectService.resolve(login);
        log.debug("Grant role {} for {} to cloud {}", roleName, subject, abcServiceCloud.cloudId());
        bindingService.grantRole(abcServiceCloud.cloudId(), subject, roleName);

        return Response.ok(Json.toJson(GenericResponse.OK)).build();
    }

    /**
     * Revokes a role to an user specified by the login parameter for a ABC id specified in the fields parameter.
     *
     * @return {code: 0} for success, the error description otherwise.
     */
    @Path("remove-role")
    @POST
    public Response removeRole(
            @FormParam("login") String login,
            @FormParam("path") String path,
            @FormParam("fields") FieldsParameter fields,
            @FormParam("role") RoleParameter role
    ) {
        validateParameters(login, path, fields, role);

        AbcServiceCloud abcServiceCloud;
        try {
            abcServiceCloud = resolveServiceFacade.resolveAbcServiceCloudByAbcServiceId(fields.getAbcId());
        } catch (AbcServiceNotFoundException ex) {
            return abcServiceNotFoundResponse(ex.getMessage());
        }
        var roleName = getRoleName(role);
        var subject = subjectService.resolve(login);
        log.debug("Revoke role {} for {} from cloud {}", roleName, subject, abcServiceCloud.cloudId());
        bindingService.revokeRole(abcServiceCloud.cloudId(), subject, roleName);

        return Response.ok(Json.toJson(GenericResponse.OK)).build();
    }

    /**
     * Returns the list of granted roles.
     *
     * <p>Example: {@code curl https://iam.cloud.yandex-team.ru/idm/get-roles/}</p>
     *
     * @return empty array
     * @implNote This is a stub method
     */
    @Path("get-roles")
    @GET
    public Response getRoles() {
        return Response.ok("{\"code\": 0, \"roles\": []}").build();
    }

    private static String getRoleName(RoleParameter role) {
        if ("s3".equals(role.getCloud())) {
            return IDM_ROLE_PREFIX + "storage." + role.getStorage();
        }
        throw BadRoleException.unsupported(role.getCloud());
    }

    private static void validateParameters(String login, String path, FieldsParameter fields, RoleParameter role) {
        Validation.checkNotNullParameter(login, "login");
        Validation.checkNotNullParameter(path, "path");
        Validation.checkNotNullParameter(fields, "fields");
        Validation.checkNotNullParameter(role, "role");
        Validation.checkNotEmptyField(role.getCloud(), "role", "cloud");
        Validation.checkNotEmptyField(role.getStorage(), "role", "storage");
        if (!roleService.isSupportedRole(path)) {
            throw BadRoleException.forbidden(path);
        }
    }

    private static Response abcServiceNotFoundResponse(String reasonPhrase) {
        return Response.status(Response.Status.PRECONDITION_FAILED.getStatusCode(), reasonPhrase)
                .build();
    }

}
