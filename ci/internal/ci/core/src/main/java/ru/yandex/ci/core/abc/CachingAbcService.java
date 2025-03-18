package ru.yandex.ci.core.abc;

import java.time.Clock;
import java.time.Duration;
import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import com.google.common.collect.HashMultimap;
import com.google.common.collect.Lists;
import com.google.common.collect.Maps;
import com.google.common.collect.Multimap;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.abc.AbcServiceMemberInfo;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Name;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Role;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo.Scope;
import ru.yandex.ci.client.abc.AbcUserDutyInfo;
import ru.yandex.ci.util.ExceptionUtils;

@Slf4j
public abstract class CachingAbcService implements AbcService {

    private static final AbcServiceEntity NON_EXISTING_SERVICE = AbcServiceEntity.empty("");

    private final Clock clock;

    // Key is abc slug
    private final LoadingCache<String, AbcServiceEntity> serviceCache;

    // Key is username
    private final LoadingCache<String, Multimap<String, AbcServiceMemberInfo>> serviceMembersCache;

    // Key is slug with roles
    private final LoadingCache<ServiceSlugWithRoles, List<AbcServiceMemberInfo>> serviceDescendantsCache;

    // Key is username
    private final LoadingCache<String, List<AbcUserDutyInfo>> userDutiesCache;

    protected CachingAbcService(
            Clock clock,
            Duration cacheTime,
            @Nullable MeterRegistry meterRegistry
    ) {
        this.clock = clock;
        serviceCache = CacheBuilder.newBuilder()
                .expireAfterWrite(cacheTime)
                .recordStats()
                .build(new AbcServicesCacheLoader());
        serviceMembersCache = CacheBuilder.newBuilder()
                .expireAfterWrite(cacheTime)
                .recordStats()
                .build(CacheLoader.from(this::collectServiceMembers));
        serviceDescendantsCache = CacheBuilder.newBuilder()
                .expireAfterWrite(cacheTime)
                .recordStats()
                .build(CacheLoader.from(this::loadServiceMembersWithDescendants));
        userDutiesCache = CacheBuilder.newBuilder()
                .expireAfterWrite(cacheTime)
                .recordStats()
                .build(CacheLoader.from(this::collectUserDuty));

        if (meterRegistry != null) {
            GuavaCacheMetrics.monitor(meterRegistry, serviceCache, "abc-cache");
            GuavaCacheMetrics.monitor(meterRegistry, serviceMembersCache, "abc-members-cache");
            GuavaCacheMetrics.monitor(meterRegistry, serviceDescendantsCache, "abc-service-descendants-cache");
            GuavaCacheMetrics.monitor(meterRegistry, userDutiesCache, "user-duties-cache");
        }
    }

    protected Clock getClock() {
        return clock;
    }

    protected abstract Map<String, AbcServiceEntity> loadServices(List<String> abcSlugs);

    protected abstract List<AbcServiceMemberInfo> loadServiceMembers(String login);

    protected abstract List<AbcServiceMemberInfo> loadServiceMembersWithDescendants(ServiceSlugWithRoles service);

    protected abstract List<AbcUserDutyInfo> loadUserDuty(String login);

    @VisibleForTesting
    protected void flushCache() {
        serviceCache.invalidateAll();
        serviceMembersCache.invalidateAll();
        serviceDescendantsCache.invalidateAll();
        userDutiesCache.invalidateAll();
    }

    @Override
    public boolean isMember(
            String login,
            String abcServiceSlug,
            Set<String> scopes,
            Set<String> roles,
            Set<String> duties
    ) {
        var slug = abcServiceSlug.toLowerCase();
        var members = getServiceMembersImpl(login);
        var abcServices = members.get(slug);
        if (abcServices.isEmpty()) {
            return false;
        }
        if (scopes.isEmpty() && roles.isEmpty() && duties.isEmpty()) {
            return true;
        }
        for (var service : abcServices) {
            var role = service.getRole();
            if (isScopeMatched(role.getScope(), scopes)) {
                return true;
            }
            if (isRoleMatched(role, roles)) {
                return true;
            }
        }
        return isDutyMatched(login, duties);
    }

    @SuppressWarnings("ReferenceEquality")
    @Override
    public Map<String, AbcServiceEntity> getServices(List<String> abcSlugs) {
        try {
            return Maps.filterValues(serviceCache.getAll(abcSlugs), info -> info != NON_EXISTING_SERVICE);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    @SuppressWarnings("ReferenceEquality")
    @Override
    public Optional<AbcServiceEntity> getService(String abcSlug) {
        try {
            return Optional.of(serviceCache.get(abcSlug)).filter(info -> info != NON_EXISTING_SERVICE);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    @Override
    public Collection<AbcServiceMemberInfo> getServicesMembers(String login) {
        return getServiceMembersImpl(login).values();
    }

    @Override
    public Collection<AbcServiceMemberInfo> getServiceMembersWithDescendants(ServiceSlugWithRoles service) {
        try {
            return serviceDescendantsCache.get(service);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    //

    private boolean isScopeMatched(@Nullable Scope scope, Set<String> scopes) {
        if (scopes.isEmpty()) {
            return false;
        }
        if (scope == null) {
            return false;
        }
        if (scopes.contains(scope.getSlug())) { // #1, scope.slug
            return true;
        }
        if (isNameMatched(scope.getName(), scopes)) {
            return true;
        }
        // #4, scope.id
        return scopes.contains(String.valueOf(scope.getId()));
    }

    private boolean isRoleMatched(Role role, Set<String> roles) {
        if (roles.isEmpty()) {
            return false;
        }
        if (roles.contains(role.getCode())) { // #1, role.code
            return true;
        }
        if (isNameMatched(role.getName(), roles)) {
            return true;
        }
        // #4, role.id
        return roles.contains(String.valueOf(role.getId()));
    }

    private boolean isDutyMatched(String login, Set<String> duties) {
        if (duties.isEmpty()) {
            return false;
        }
        var userDuties = getUserDutiesImpl(login);
        if (userDuties.isEmpty()) {
            return false;
        }
        var now = clock.instant();
        for (var duty : userDuties) {
            log.info("Check {}, Now: {}, start: {}, end: {}", duty.getId(), now, duty.getStart(), duty.getEnd());
            if (now.isAfter(duty.getStart()) && now.isBefore(duty.getEnd())) {
                var schedule = duty.getSchedule();
                if (duties.contains(schedule.getSlug())) { // #1 schedule.slug
                    return true;
                }
                if (duties.contains(schedule.getName())) { // #2 schedule.name
                    return true;
                }
                if (duties.contains(String.valueOf(schedule.getId()))) { // #3 schedule.id
                    return true;
                }
            }
        }
        return false;
    }

    private boolean isNameMatched(@Nullable Name name, Set<String> set) {
        if (name == null) {
            return false;
        }
        // #2, scope.name.en
        // #3, scope.name.ru
        return set.contains(name.getEn()) ||
                set.contains(name.getRu());
    }

    private Multimap<String, AbcServiceMemberInfo> getServiceMembersImpl(String login) {
        try {
            return serviceMembersCache.get(login);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    private List<AbcUserDutyInfo> getUserDutiesImpl(String login) {
        try {
            return userDutiesCache.get(login);
        } catch (Exception e) {
            throw ExceptionUtils.unwrap(e);
        }
    }

    private Multimap<String, AbcServiceMemberInfo> collectServiceMembers(String login) {
        var members = loadServiceMembers(login);
        var map = HashMultimap.<String, AbcServiceMemberInfo>create();
        for (var member : members) {
            map.put(member.getServiceSlug(), member);
        }
        return map;
    }

    private List<AbcUserDutyInfo> collectUserDuty(String login) {
        return loadUserDuty(login).stream()
                .filter(AbcUserDutyInfo::isApproved)
                .toList();
    }

    private class AbcServicesCacheLoader extends CacheLoader<String, AbcServiceEntity> {

        @Override
        public Map<String, AbcServiceEntity> loadAll(Iterable<? extends String> slugIterable) {
            var slugs = Lists.newArrayList(slugIterable).stream()
                    .map(String::toLowerCase)
                    .toList();
            var response = loadServices(slugs);
            for (String slug : slugs) {
                response.putIfAbsent(slug, NON_EXISTING_SERVICE);
            }
            return response;
        }

        @Override
        public AbcServiceEntity load(String slug) {
            slug = slug.toLowerCase();
            return getServices(List.of(slug)).getOrDefault(slug, NON_EXISTING_SERVICE);
        }
    }
}
