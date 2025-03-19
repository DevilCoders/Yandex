package yandex.cloud.team.integration.idm.service;

import java.util.List;

import javax.inject.Inject;

import yandex.cloud.auth.bindings.AccessBinding;
import yandex.cloud.auth.bindings.AccessBindingAction;
import yandex.cloud.auth.bindings.AccessSubject;
import yandex.cloud.team.integration.idm.model.Subject;
import yandex.cloud.ti.rm.client.ResourceManagerClient;

public class BindingServiceImpl implements BindingService {

    static final String SERVICE_NAME = "IDM";

    @Inject
    private static ResourceManagerClient resourceManagerClient;

    @Override
    public void grantRole(String cloudId, Subject subject, String role) {
        updateBindings(cloudId, subject, role, AccessBindingAction.Action.ADD);
    }

    @Override
    public void revokeRole(String cloudId, Subject subject, String role) {
        updateBindings(cloudId, subject, role, AccessBindingAction.Action.REMOVE);
    }

    private void updateBindings(String cloudId, Subject subject, String role, AccessBindingAction.Action action) {
        var actions = List.of(buildBinding(subject, role, action));
        var op = resourceManagerClient.updateCloudAccessBindings(cloudId, actions, true);
        // TODO: wait till operation is done?
    }

    private static AccessBindingAction buildBinding(Subject subject, String role, AccessBindingAction.Action action) {
        return AccessBindingAction.builder().action(action)
            .accessBinding(AccessBinding.builder()
                .roleId(role)
                .subject(AccessSubject.create(subject.getSubjectId(), subject.getSubjectType()))
                .managedBy(SERVICE_NAME)
                .build())
            .build();
    }

}
