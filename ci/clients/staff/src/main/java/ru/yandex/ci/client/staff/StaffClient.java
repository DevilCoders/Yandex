package ru.yandex.ci.client.staff;

import javax.annotation.Nullable;

import com.google.common.annotations.VisibleForTesting;
import com.google.common.base.Preconditions;
import com.google.common.base.Strings;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

/**
 * Docs: https://staff-api.yandex-team.ru
 */
public class StaffClient {
    @VisibleForTesting
    static final String ABC_SERVICE_GROUP_PREFIX = "svc_";

    private final StaffApi staffApi;

    private StaffClient(HttpClientProperties httpClientProperties) {
        this.staffApi = RetrofitClient.builder(httpClientProperties, getClass())
                .snakeCaseNaming()
                .failOnMissingCreatorProperties()
                .build(StaffApi.class);
    }

    public static StaffClient create(HttpClientProperties httpClientProperties) {
        return new StaffClient(httpClientProperties);
    }

    public boolean isUserInAbcService(String login, String abcService) {
        return isUserInAbcService(login, abcService, null);
    }

    public boolean isUserInAbcService(String login, String abcService, @Nullable String scope) {
        Preconditions.checkState(!Strings.isNullOrEmpty(login), "Login can't be empty");
        Preconditions.checkState(!Strings.isNullOrEmpty(abcService), "Group can't be empty");

        var groupUrl = ABC_SERVICE_GROUP_PREFIX + abcService.toLowerCase() +
                (Strings.isNullOrEmpty(scope) ? "" : "_" + scope.toLowerCase());

        var response = staffApi.groupMembership(groupUrl, login);
        return isContainsLogin(response, login);
    }

    private boolean isContainsLogin(GroupMembershipResponse response, String login) {
        var result = response.getResult();

        if (result == null || result.isEmpty()) {
            return false;
        }

        return login.equals(result.get(0).getPerson().getLogin());
    }

    public StaffPerson getStaffPerson(String login) {
        return staffApi.person(login);
    }

    /*
     * Для получения других полей или коллекций нужно запросить доступы. Пример:
     * https://idm.yandex-team.ru/system/staffapi#role=66073653,f-role-id=66073653
     */
    interface StaffApi {
        @GET("/v3/groupmembership?_fields=person.login,person.official.affiliation,person.official.is_robot")
        GroupMembershipResponse groupMembership(
                @Query("group.url") String groupUrl,
                @Query("person.login") String login
        );

        @GET("/v3/persons?_one=1&_fields=login,official.affiliation,official.is_robot")
        StaffPerson person(@Query("login") String login);

    }
}
