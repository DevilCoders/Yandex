package ru.yandex.ci.client.abc;

import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;
import java.util.function.Function;

import javax.annotation.Nullable;

import com.google.common.base.Strings;
import lombok.Value;
import one.util.streamex.StreamEx;
import retrofit2.http.GET;
import retrofit2.http.Query;
import retrofit2.http.Url;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

/**
 * https://wiki.yandex-team.ru/intranet/abc/api/
 */
public class AbcClient {

    private static final String MEMBER_FIELDS = "service.slug,role";
    private static final String SERVICE_FIELDS = "id,slug,name,description,path";
    private static final String USER_DUTY_FIELDS = "id,is_approved,schedule.id,schedule.name,schedule.slug";

    private static final int MAX_PAGE_SIZE = 200;

    private final AbcApi api;

    private AbcClient(HttpClientProperties properties) {
        this.api = RetrofitClient.builder(properties, getClass()).build(AbcApi.class);
    }

    public static AbcClient create(HttpClientProperties properties) {
        return new AbcClient(properties);
    }

    public List<AbcServiceInfo> getServices(List<String> abcSlugs) {
        if (abcSlugs.isEmpty()) {
            return List.of();
        }
        return StreamEx.ofSubLists(abcSlugs, MAX_PAGE_SIZE)
                .map(slugs -> api.getServices(MAX_PAGE_SIZE, SERVICE_FIELDS, String.join(",", slugs)).results)
                .flatCollection(Function.identity())
                .toList();
    }

    public List<AbcServiceInfo> getAllServices() {
        return collectServices(api.getServices(MAX_PAGE_SIZE, SERVICE_FIELDS, null));
    }

    public List<AbcServiceMemberInfo> getServiceMembers(String login) {
        var response = api.getServicesWhichUserBelongsTo(MAX_PAGE_SIZE, MEMBER_FIELDS, login);
        return collectServiceMembers(response);
    }

    public List<AbcServiceMemberInfo> getServicesMembers(
            String abcSlug,
            @Nullable String roleScopeSlug,
            List<String> fields,
            boolean unique
    ) {
        var response = api.getServicesMembers(MAX_PAGE_SIZE, String.join(",", fields), abcSlug, roleScopeSlug, unique);
        return collectServiceMembers(response);
    }

    /**
     * get services of service members and descendant service members
     *
     * @param abcSlug service slug
     * @param roles   additional role codes filter, empty roles list means any role
     */
    public List<AbcServiceMemberInfo> getServiceMembersWithDescendants(String abcSlug, List<String> roles) {
        var roleString = roles.isEmpty() ? null : String.join(",", roles);
        var response = api.getServiceMembersWithDescendants(MAX_PAGE_SIZE, MEMBER_FIELDS, abcSlug, roleString);
        return collectServiceMembers(response);
    }

    public List<AbcUserDutyInfo> getUserDutyInfo(String login) {
        return api.getUserDutyInfo(USER_DUTY_FIELDS, login);
    }

    private List<AbcServiceInfo> collectServices(AbcServicesResponse response) {
        Set<AbcServiceInfo> members = new LinkedHashSet<>(response.results);
        while (!Strings.isNullOrEmpty(response.next)) {
            response = api.getServicesNext(response.next);
            members.addAll(response.results);
        }
        return List.copyOf(members);
    }

    private List<AbcServiceMemberInfo> collectServiceMembers(AbcServicesMembersResponse response) {
        Set<AbcServiceMemberInfo> members = new LinkedHashSet<>(response.results);
        while (!Strings.isNullOrEmpty(response.next)) {
            response = api.getServicesMembersNext(response.next);
            members.addAll(response.results);
        }
        return List.copyOf(members);
    }

    @Value
    static class AbcServicesResponse {
        List<AbcServiceInfo> results;
        String next;
    }

    @Value
    static class AbcServicesMembersResponse {
        List<AbcServiceMemberInfo> results;
        String next;
    }

    interface AbcApi {

        @GET("/api/v4/services/members/")
        AbcServicesMembersResponse getServicesWhichUserBelongsTo(
                @Query("page_size") int pageSize,
                @Query(value = "fields", encoded = true) String fields,
                @Query("person__login") String login);

        @GET("/api/v4/services/members/")
        AbcServicesMembersResponse getServicesMembers(
                @Query("page_size") int pageSize,
                @Query(value = "fields", encoded = true) String fields,
                @Query("service__slug") String withSlug,
                @Nullable @Query("role__scope__slug") String withRoleScopeSlug,
                @Query("unique") boolean unique);

        @GET("/api/v4/services/members/")
        AbcServicesMembersResponse getServiceMembersWithDescendants(
                @Query("page_size") int pageSize,
                @Query(value = "fields", encoded = true) String fields,
                @Query("service__with_descendants__slug") String withSlug,
                @Nullable @Query(value = "role__code__in", encoded = true) String rolesIn);

        @GET("/api/v4/duty/on_duty/")
        List<AbcUserDutyInfo> getUserDutyInfo(
                @Query(value = "fields", encoded = true) String fields,
                @Query("person") String person);

        @GET
        AbcServicesMembersResponse getServicesMembersNext(@Url String next);

        @GET("/api/v4/services/")
        AbcServicesResponse getServices(
                @Query("page_size") int pageSize,
                @Query(value = "fields", encoded = true) String fields,
                @Nullable @Query(value = "slug__in", encoded = true) String slugs);

        @GET
        AbcServicesResponse getServicesNext(@Url String next);
    }

}
