package yandex.cloud.team.integration.idm.exception;

import javax.ws.rs.core.Response;

public abstract class BadRequestException extends IdmServiceException {

    protected BadRequestException(String message) {
        super(message, Response.Status.BAD_REQUEST);
    }

}
