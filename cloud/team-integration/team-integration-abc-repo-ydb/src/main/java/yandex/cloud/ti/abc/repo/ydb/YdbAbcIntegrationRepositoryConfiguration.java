package yandex.cloud.ti.abc.repo.ydb;

import lombok.NonNull;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepository;
import yandex.cloud.ti.abc.repo.AbcIntegrationRepositoryConfiguration;

public class YdbAbcIntegrationRepositoryConfiguration extends AbcIntegrationRepositoryConfiguration {

    @Override
    protected @NonNull AbcIntegrationRepository abcIntegrationRepository() {
        // todo depends on TeamIntegrationKikimrRepositoryConfiguration
        return new YdbAbcIntegrationRepositoryImpl();
    }

}
