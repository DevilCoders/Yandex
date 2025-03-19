package yandex.cloud.team.integration.idm.service;

import javax.inject.Inject;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.team.integration.idm.grpc.client.PassportFederationClient;
import yandex.cloud.team.integration.idm.model.Subject;

public class SubjectServiceImpl implements SubjectService {

    @Inject
    private static PassportFederationClient passportFederationClient;

    @Override
    public Subject resolve(@NotNull String login) {
        return passportFederationClient.addUserAccounts(login).get(0);
    }

}
