package yandex.cloud.team.integration.idm.http;

import javax.ws.rs.core.Response;
import javax.ws.rs.ext.ExceptionMapper;
import javax.ws.rs.ext.Provider;

import lombok.extern.log4j.Log4j2;
import yandex.cloud.team.integration.idm.exception.IdmServiceException;
import yandex.cloud.team.integration.idm.model.response.GenericResponse;
import yandex.cloud.util.Json;

/**
 * Adds application-specific error message to the response.
 */
@Log4j2
@Provider
public class IdmServiceExceptionMapper implements ExceptionMapper<IdmServiceException> {

    @Override
    public Response toResponse(IdmServiceException exception) {
        Response.StatusType status = exception.getResponse().getStatusInfo();
        if (status.getFamily() == Response.Status.Family.SERVER_ERROR) {
            log.error("Internal error", exception);
        }

        GenericResponse body = GenericResponse.builder()
            .code(status.getStatusCode())
            .error(exception.getMessage())
            .build();

        return Response.fromResponse(exception.getResponse())
            .entity(Json.toJson(body))
            .build();
    }

}
