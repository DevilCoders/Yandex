package yandex.cloud.team.integration.idm.service;

import org.jetbrains.annotations.NotNull;
import yandex.cloud.team.integration.idm.model.Subject;

/**
 * Staff login to IAM subject mapper.
 */
public interface SubjectService {

    /**
     * Resolves a staff login to it's subject id and type.
     *
     * @param login staff login
     * @return user subject
     */
    Subject resolve(@NotNull String login);

}
