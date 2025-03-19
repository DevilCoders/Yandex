package yandex.cloud.ti.abcd.adapter;

import lombok.Value;
import org.jetbrains.annotations.Nullable;

@Value
class CreateAccountParameters {

    String providerId;
    long abcServiceId;
    String abcFolderId;

    @Nullable String operationId;

    String key;
    String displayName;
    boolean freeTier;

}
