package yandex.cloud.team.integration.idm.exception;

import java.io.Serial;

public class MissingFieldException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = -1196533950393670357L;

    private MissingFieldException(String message) {
        super(message);
    }

    public static MissingFieldException of(String parameterName, String fieldName) {
        return new MissingFieldException(
                String.format("The field '%s' of the parameter '%s' is missing", fieldName, parameterName));
    }

}
