package ru.yandex.ci.ayamler;

import java.time.Duration;
import java.util.List;
import java.util.Set;
import java.util.stream.Collectors;

import javax.annotation.Nullable;

import com.google.common.cache.CacheBuilder;
import com.google.common.cache.CacheLoader;
import com.google.common.cache.LoadingCache;
import io.micrometer.core.instrument.MeterRegistry;
import io.micrometer.core.instrument.binder.cache.GuavaCacheMetrics;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.client.abc.AbcClient;
import ru.yandex.ci.client.abc.AbcServiceMemberInfo;

@Slf4j
public class AbcService {

    private final AbcClient abcClient;
    // (abc_slug, abc_scope) -> Set<user_login>
    private final LoadingCache<AbcSlugAndScope, Set<String>> serviceMembersCache;

    public AbcService(
            AbcClient abcClient,
            Duration serviceMembersCacheExpireAfterWrite,
            MeterRegistry meterRegistry,
            int serviceMembersCacheConcurrencyLevel
    ) {
        this.abcClient = abcClient;

        serviceMembersCache = CacheBuilder.newBuilder()
                .expireAfterWrite(serviceMembersCacheExpireAfterWrite)
                .recordStats()
                .concurrencyLevel(serviceMembersCacheConcurrencyLevel)
                .build(CacheLoader.from(this::loadServicesMembers));

        GuavaCacheMetrics.monitor(meterRegistry, serviceMembersCache, "ayamler-abc-service-members-cache");
    }

    @Value
    private static class AbcSlugAndScope {
        String slug;
        @Nullable
        String scope;
    }

    boolean isUserBelongsToService(String login, String abcSlug, Set<String> abcScopes) {
        for (var scopeSlug : abcScopes) {
            var members = serviceMembersCache.getUnchecked(new AbcSlugAndScope(abcSlug, scopeSlug));
            if (members.contains(login)) {
                return true;
            }
        }
        return false;
    }

    void refreshServiceMembersCache(String abcSlug) {
        serviceMembersCache.refresh(new AbcSlugAndScope(abcSlug, null));
    }

    private Set<String> loadServicesMembers(AbcSlugAndScope abcSlugAndScope) {
        log.info("loading members of {}", abcSlugAndScope);
        var members = abcClient.getServicesMembers(abcSlugAndScope.slug, abcSlugAndScope.scope,
                List.of("service.slug", "person.login"), false);
        var logins = members.stream()
                .map(AbcServiceMemberInfo::getPerson)
                .map(AbcServiceMemberInfo.Person::getLogin)
                .collect(Collectors.toSet());
        log.info("loaded members of {}: {}", abcSlugAndScope, logins);
        return logins;
    }

}
