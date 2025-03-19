package yandex.cloud.team.integration.idm.config;

import yandex.cloud.di.Configuration;
import yandex.cloud.team.integration.idm.service.BindingService;
import yandex.cloud.team.integration.idm.service.BindingServiceImpl;
import yandex.cloud.team.integration.idm.service.RoleService;
import yandex.cloud.team.integration.idm.service.RoleServiceImpl;
import yandex.cloud.team.integration.idm.service.SubjectService;
import yandex.cloud.team.integration.idm.service.SubjectServiceImpl;

public class IdmServicesConfiguration extends Configuration {

    @Override
    public void configure() {
        put(BindingService.class, BindingServiceImpl::new);
        put(RoleService.class, RoleServiceImpl::new);
        put(SubjectService.class, SubjectServiceImpl::new);
    }

}
