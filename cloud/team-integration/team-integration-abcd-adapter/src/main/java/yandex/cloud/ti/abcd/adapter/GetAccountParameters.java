package yandex.cloud.ti.abcd.adapter;

import lombok.Value;

@Value
class GetAccountParameters {

    String accountId;

    String providerId;
    long abcServiceId;
    String abcFolderId;

    boolean withProvisions;
    boolean includeDeleted;

}
