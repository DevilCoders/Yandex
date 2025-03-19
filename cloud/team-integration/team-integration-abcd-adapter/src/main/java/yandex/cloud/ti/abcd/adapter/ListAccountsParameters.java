package yandex.cloud.ti.abcd.adapter;

import lombok.Value;
import org.jetbrains.annotations.NotNull;
import org.jetbrains.annotations.Nullable;

@Value
class ListAccountsParameters {

    @NotNull String providerId;

    // todo explicit flag to specify whether abc service/folder is present?
    // optional
    long abcServiceId;
    // optional
    @Nullable String abcFolderId;

    boolean withProvisions;
    boolean includeDeleted;

    long limit;
    @Nullable String pageToken;

}
