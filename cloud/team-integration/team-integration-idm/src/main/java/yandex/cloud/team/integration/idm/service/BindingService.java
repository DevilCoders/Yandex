package yandex.cloud.team.integration.idm.service;

import yandex.cloud.team.integration.idm.model.Subject;

/**
 * Service to manage access bindings.
 */
public interface BindingService {

    /**
     * Adds the role binding for the specified subject to the specified cloud.
     *
     * @param cloudId the id of the cloud to update bindings
     * @param subject the IAM subject id and type
     * @param role the role to add
     */
    void grantRole(String cloudId, Subject subject, String role);

    /**
     * Removes the role binding for the specified subject from the specified cloud.
     *
     * @param cloudId the id of the cloud to update bindings
     * @param subject the IAM subject id and type
     * @param role the role to remove
     */
    void revokeRole(String cloudId, Subject subject, String role);

}
