package yandex.cloud.team.integration.idm.model;

import java.util.Set;

public interface SubjectType {

    String USER_ACCOUNT = "userAccount";

    String FEDERATED_USER = "federatedUser";

    Set<String> ALL = Set.of(USER_ACCOUNT, FEDERATED_USER);

}
