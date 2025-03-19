package yandex.cloud.team.integration.idm.exception;

import java.io.Serial;

public class MissingParameterException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 7911378573652558644L;

    private MissingParameterException(String message) {
        super(message);
    }

    public static MissingParameterException of(String name) {
        return new MissingParameterException(String.format("Required parameter '%s' is missing", name));
    }

}
