package yandex.cloud.team.integration.idm.exception;

import javax.ws.rs.ClientErrorException;
import javax.ws.rs.core.Response;

public abstract class IdmServiceException extends ClientErrorException {

    public IdmServiceException(String message, Response.Status status) {
        super(message, status);
    }

}
