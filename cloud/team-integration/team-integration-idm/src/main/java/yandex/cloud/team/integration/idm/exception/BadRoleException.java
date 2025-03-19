package yandex.cloud.team.integration.idm.exception;

import java.io.Serial;

public class BadRoleException extends BadRequestException {

    @Serial
    private static final long serialVersionUID = 1108411551782130185L;

    private BadRoleException(String message) {
        super(message);
    }

    public static BadRoleException forbidden(String rolePath) {
        return new BadRoleException(String.format("Role '%s' is not allowed", rolePath));
    }

    public static BadRoleException unsupported(String roleName) {
        return new BadRoleException(String.format("Cloud service '%s' is not supported", roleName));
    }

}
