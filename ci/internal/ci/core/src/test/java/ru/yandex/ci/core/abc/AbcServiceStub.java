package ru.yandex.ci.core.abc;

import java.time.Clock;
import java.time.Duration;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Objects;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.function.Function;
import java.util.stream.Collectors;

import one.util.streamex.StreamEx;

import ru.yandex.ci.client.abc.AbcServiceInfo;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Scope;
import ru.yandex.ci.client.abc.AbcUserDutyInfo;
import ru.yandex.ci.core.test.TestData;
import ru.yandex.ci.util.Clearable;

public class AbcServiceStub extends CachingAbcService implements Clearable {

    private final Map<String, AbcServiceInfo> services = new ConcurrentHashMap<>();
    private final Map<String, Collection<AbcServiceMemberInfo>> servicesMembers = new ConcurrentHashMap<>();
    private final Map<String, Collection<AbcUserDutyInfo>> userDuties = new ConcurrentHashMap<>();

    public AbcServiceStub(Clock clock) {
        super(clock, Duration.ofDays(1), null);
        clear();
    }

    @Override
    public Map<String, AbcServiceEntity> loadServices(List<String> abcSlugs) {
        return StreamEx.of(abcSlugs)
                .map(services::get)
                .nonNull()
                .map(info -> AbcServiceEntity.of(info, getClock().instant()))
                .toMap(AbcServiceEntity::getSlug, Function.identity());
    }

    @Override
    public List<AbcServiceMemberInfo> loadServiceMembers(String login) {
        return List.copyOf(servicesMembers.getOrDefault(login, List.of()));
    }

    @Override
    public List<AbcServiceMemberInfo> loadServiceMembersWithDescendants(ServiceSlugWithRoles service) {
        return loadServiceMembers(service.getSlug()).stream()
                .filter(m -> service.getRoles().isEmpty() || service.getRoles().contains(m.getRoleCode()))
                .collect(Collectors.toList());
    }

    @Override
    public List<AbcUserDutyInfo> loadUserDuty(String login) {
        return List.copyOf(userDuties.getOrDefault(login, List.of()));
    }

    public void reset() {
        services.clear();
        servicesMembers.clear();
        userDuties.clear();
        this.flushCache();
    }

    @Override
    public void clear() {
        reset();
        addService(Abc.CI, TestData.CI_USER, TestData.USER42);
        addService(Abc.TE, TestData.CI_USER);
        addService(Abc.DEVTECH);
        addService(Abc.INFRA);
        addService(Abc.SEARCH);
        addService(Abc.AUTOCHECK, TestData.CI_USER, TestData.USER42);
        addService(Abc.SERP_SEARCH);
    }

    public void removeService(String slug) {
        services.remove(slug);
        for (var list : servicesMembers.values()) {
            list.removeIf(info -> Objects.equals(slug, info.getServiceSlug()));
        }
        this.flushCache();
    }

    public void addService(Abc abc, String... logins) {
        String abcSlug = abc.getSlug().toLowerCase();

        removeService(abc.getSlug());
        services.put(abcSlug, abc.toServiceInfo());
        for (var login : logins) {
            servicesMembers.computeIfAbsent(login, m -> new LinkedBlockingQueue<>())
                    .add(new AbcServiceMemberInfo(
                            new AbcServiceMemberInfo.Service(abcSlug),
                            AbcServiceMemberInfo.Role.of("other", Scope.of("administration")),
                            new AbcServiceMemberInfo.Person(login)));
        }
        this.flushCache();
    }

    public void removeUserMembership(String login) {
        servicesMembers.remove(login);
        this.flushCache();
    }

    public void addUserMembership(String login, AbcServiceMemberInfo... list) {
        servicesMembers.computeIfAbsent(login, u -> new LinkedBlockingQueue<>())
                .addAll(List.of(list));
        this.flushCache();
    }


    public void removeDuties(String login) {
        userDuties.remove(login);
        this.flushCache();
    }

    public void addDuty(String login, AbcUserDutyInfo... list) {
        userDuties.computeIfAbsent(login, u -> new LinkedBlockingQueue<>())
                .addAll(List.of(list));
        this.flushCache();
    }

    public List<AbcServiceInfo> getServices() {
        return List.copyOf(services.values());
    }
}
