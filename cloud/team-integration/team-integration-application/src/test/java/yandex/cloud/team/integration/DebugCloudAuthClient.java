package yandex.cloud.team.integration;

import yandex.cloud.auth.api.CloudAuthClient;
import yandex.cloud.auth.api.Resource;
import yandex.cloud.auth.api.Subject;
import yandex.cloud.auth.api.bulkauthorize.BulkAuthorizeRequest;
import yandex.cloud.auth.api.bulkauthorize.BulkAuthorizeResponse;
import yandex.cloud.auth.api.credentials.AbstractCredentials;

public class DebugCloudAuthClient implements CloudAuthClient {

    private static final Subject INSTANCE = new DebugServiceAccount();


    @Override
    public Subject authenticate(AbstractCredentials credentials) {
        return INSTANCE;
    }

    @Override
    public Subject authorize(AbstractCredentials abstractCredentials, String permission, Iterable<Resource> iterable) {
        return INSTANCE;
    }

    @Override
    public Subject authorize(Subject.Id id, String permission, Iterable<Resource> iterable) {
        return INSTANCE;
    }

    @Override
    public BulkAuthorizeResponse bulkAuthorize(BulkAuthorizeRequest bulkAuthorizeRequest) {
        return null;
    }


    private static class DebugServiceAccount extends Subject.ServiceAccount.Id implements Subject.ServiceAccount {

        private DebugServiceAccount() {
            super("DEBUG_SA");
        }

        @Override
        public Id toId() {
            return this;
        }

        @Override
        public String getFolderId() {
            return "DEBUG_FOLDER";
        }

    }

}
