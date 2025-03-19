package yandex.cloud.team.integration.idm.model.response;

import lombok.Builder;
import lombok.Value;

@Builder
@Value
public class GenericResponse {

    int code;
    String error;

    public static final GenericResponse OK = new GenericResponseBuilder().code(0).build();

}
