package ru.yandex.ci.core.abc;

import java.util.Collection;
import java.util.List;
import java.util.Map;
import java.util.Optional;
import java.util.Set;

import ru.yandex.ci.client.abc.AbcServiceMemberInfo;

public interface AbcService {

    Map<String, AbcServiceEntity> getServices(List<String> abcSlugs);

    Optional<AbcServiceEntity> getService(String abcSlug);

    default boolean isMember(String login, String abcServiceSlug) {
        return isMember(login, abcServiceSlug, Set.of(), Set.of(), Set.of());
    }

    default boolean isMember(String login, String abcServiceSlug, Set<String> scopes) {
        return isMember(login, abcServiceSlug, scopes, Set.of(), Set.of());
    }

    /**
     * Check if user has access to any of given scopes, roles or duties.
     *
     * @param login          username
     * @param abcServiceSlug ABC slug
     * @param scopes         list of scopes
     * @param roles          list of roles
     * @param duties         list of duties
     * @return true is has access to anything from this lists
     */
    boolean isMember(String login, String abcServiceSlug, Set<String> scopes, Set<String> roles, Set<String> duties);

    Collection<AbcServiceMemberInfo> getServicesMembers(String login);

    Collection<AbcServiceMemberInfo> getServiceMembersWithDescendants(ServiceSlugWithRoles service);
}
