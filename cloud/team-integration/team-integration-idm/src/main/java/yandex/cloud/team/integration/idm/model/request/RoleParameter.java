package yandex.cloud.team.integration.idm.model.request;

import lombok.Value;
import yandex.cloud.util.Json;

@Value
public class RoleParameter {

    String cloud;
    String storage;

    public static RoleParameter valueOf(String value) {
        return Json.fromJson(RoleParameter.class, value);
    }

}

