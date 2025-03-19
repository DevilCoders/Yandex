package yandex.cloud.team.integration.idm.exception;

import java.io.Serial;

public class InvalidServiceSpecException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 300024088877681997L;

    private InvalidServiceSpecException(String message) {
        super(message);
    }

    public static InvalidServiceSpecException of(String value) {
        return new InvalidServiceSpecException(
            String.format("The field 'service_spec' of the parameter 'fields' should be in format 'slug-id-group', got '%s'", value));
    }

}
