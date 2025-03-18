package ru.yandex.ci.core.abc;

import java.time.Clock;
import java.time.Duration;
import java.time.Instant;
import java.util.List;
import java.util.Map;
import java.util.function.Function;

import javax.annotation.Nullable;

import io.micrometer.core.instrument.MeterRegistry;
import lombok.extern.slf4j.Slf4j;
import one.util.streamex.StreamEx;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcServiceInfo;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo;
import ru.yandex.ci.client.abc.AbcUserDutyInfo;
import ru.yandex.ci.core.db.CiMainDb;

@Slf4j
public class AbcServiceImpl extends CachingAbcService {

    private final AbcClient abcClient;
    private final CiMainDb db;

    public AbcServiceImpl(
            Clock clock,
            AbcClient abcClient,
            Duration cacheTime,
            CiMainDb db,
            @Nullable MeterRegistry meterRegistry
    ) {
        super(clock, cacheTime, meterRegistry);
        this.abcClient = abcClient;
        this.db = db;
    }

    public void syncServices() {
        syncServicesImpl(abcClient.getAllServices());
    }

    @Override
    protected Map<String, AbcServiceEntity> loadServices(List<String> abcSlugs) {
        return StreamEx.of(db.currentOrReadOnly(() -> db.abcServices().loadServices(abcSlugs)))
                .toMap(AbcServiceEntity::getSlug, Function.identity());
    }

    @Override
    protected List<AbcServiceMemberInfo> loadServiceMembers(String login) {
        return abcClient.getServiceMembers(login);
    }

    @Override
    protected List<AbcServiceMemberInfo> loadServiceMembersWithDescendants(ServiceSlugWithRoles service) {
        return abcClient.getServiceMembersWithDescendants(service.getSlug(), service.getRoles());
    }

    @Override
    protected List<AbcUserDutyInfo> loadUserDuty(String login) {
        return abcClient.getUserDutyInfo(login);
    }

    protected List<AbcServiceEntity> syncServicesImpl(List<AbcServiceInfo> services) {
        log.info("About to sync {} services", services.size());

        Instant updated = getClock().instant();
        var mapped = services.stream()
                .map(info -> AbcServiceEntity.of(info, updated))
                .toList();
        db.currentOrTx(() -> {
            db.abcServices().save(mapped);
            // TODO: delete old services?
        });

        return mapped;
    }

}
