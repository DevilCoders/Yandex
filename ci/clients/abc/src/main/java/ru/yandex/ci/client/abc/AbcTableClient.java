package ru.yandex.ci.client.abc;

import java.io.IOException;

import com.fasterxml.jackson.core.JsonProcessingException;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.google.common.base.Preconditions;
import com.google.common.collect.MultimapBuilder;
import com.google.common.collect.SetMultimap;
import lombok.Value;
import lombok.extern.slf4j.Slf4j;
import okhttp3.ResponseBody;
import retrofit2.Response;
import retrofit2.http.GET;
import retrofit2.http.Query;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.base.http.RetrofitClient;

@Slf4j
public class AbcTableClient {
    private static final int INPUT_ROW_LIMIT = 10_000_000;
    private static final String QUERY_USER_TO_SERVICE = """
                login, slug
                from [//home/abc/db/services_servicemember] as members
                    join [//home/abc/db/intranet_staff] as staff
                on (members.staff_id) = (staff.id)
                    join [//home/abc/db/services_service] as services
                    on (members.service_id) = (services.id)
                where staff.is_dismissed = false
                  and staff.is_robot = false
                  and staff.affiliation = 'yandex'
                  and members.state = 'active'
                group by staff.login as login, string(services.slug) as slug
                order by login
                limit %s""".formatted(INPUT_ROW_LIMIT);

    private final ObjectMapper mapper;
    private final YtApi api;

    private AbcTableClient(HttpClientProperties properties) {
        var builder = RetrofitClient.builder(properties, getClass());

        this.mapper = builder.getObjectMapper();
        this.api = builder.build(YtApi.class);
    }

    public static AbcTableClient create(HttpClientProperties properties) {
        return new AbcTableClient(properties);
    }


    // key = login, value = abc slugs
    public SetMultimap<String, String> getUserToServicesMapping() {
        var result = MultimapBuilder.hashKeys().hashSetValues()
                .<String, String>build();

        String[] list;
        try {
            try (var body = api.selectRows(QUERY_USER_TO_SERVICE).body()) {
                Preconditions.checkState(body != null, "Body cannot be null");
                list = body.string().split("\n");
            }
        } catch (IOException e) {
            throw new RuntimeException("Unable to read body", e);
        }

        log.info("Loaded {} login/slug mappings", list.length);
        for (var value : list) {
            if (value.isEmpty()) {
                continue;
            }
            try {
                var map = mapper.readValue(value, LoginAndSlug.class);
                result.put(map.login, map.slug.toLowerCase());
            } catch (JsonProcessingException e) {
                throw new RuntimeException("Unable to parse " + value + " as " + LoginAndSlug.class, e);
            }
        }

        log.info("Mapped logins: {}", result.keySet().size());
        return result;
    }

    @Value
    static class LoginAndSlug {
        String login;
        String slug;
    }

    interface YtApi {
        @GET("/api/v3/select_rows?input_row_limit=" + INPUT_ROW_LIMIT)
        Response<ResponseBody> selectRows(
                @Query("query") String query);
    }
}
