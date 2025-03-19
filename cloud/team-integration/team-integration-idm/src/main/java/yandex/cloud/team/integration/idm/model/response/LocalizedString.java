package yandex.cloud.team.integration.idm.model.response;

import lombok.Builder;
import lombok.Value;

@Builder
@Value
public class LocalizedString {

    String en;
    String ru;

    public static LocalizedString of(String en, String ru) {
        return new LocalizedString(en, ru);
    }

}
