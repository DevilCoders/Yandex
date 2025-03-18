package ru.yandex.ci.client.calendar;

import java.time.LocalDate;
import java.util.List;

import io.netty.handler.codec.http.HttpMethod;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.extension.ExtendWith;
import org.mockito.Mock;
import org.mockito.junit.jupiter.MockitoExtension;
import org.mockserver.client.MockServerClient;
import org.mockserver.junit.jupiter.MockServerExtension;
import org.mockserver.model.HttpStatusCode;
import org.mockserver.model.MediaType;

import ru.yandex.ci.client.base.http.HttpClientProperties;
import ru.yandex.ci.client.calendar.api.Country;
import ru.yandex.ci.client.calendar.api.DayType;
import ru.yandex.ci.client.calendar.api.Holiday;
import ru.yandex.ci.client.calendar.api.Holidays;
import ru.yandex.ci.client.tvm.TvmAuthProvider;
import ru.yandex.ci.util.ResourceUtils;
import ru.yandex.passport.tvmauth.TvmClient;

import static org.assertj.core.api.Assertions.assertThat;
import static org.mockito.Mockito.when;
import static org.mockserver.model.HttpRequest.request;
import static org.mockserver.model.HttpResponse.response;

@ExtendWith(MockServerExtension.class)
@ExtendWith(MockitoExtension.class)
class CalendarClientTest {

    private static final int CALENDAR_TVM_ID = 990;
    private final MockServerClient server;

    private CalendarClient client;

    @Mock
    private TvmClient tvmClient;

    CalendarClientTest(MockServerClient server) {
        this.server = server;
    }

    @BeforeEach
    public void setUp() {
        server.reset();
        when(tvmClient.getServiceTicketFor(CALENDAR_TVM_ID)).thenReturn("ticket-01");

        client = CalendarClientImpl.create(
                HttpClientProperties.builder()
                        .endpoint("http:/" + server.remoteAddress() + "/api/")
                        .authProvider(new TvmAuthProvider(tvmClient, CALENDAR_TVM_ID))
                        .build()
        );
    }

    @Test
    void getHolidays() {
        server.when(
                request("/api/get-holidays").withMethod(HttpMethod.GET.name())
                        .withHeader("X-Ya-Service-Ticket", "ticket-01")
                        .withQueryStringParameter("from", "2015-04-15")
                        .withQueryStringParameter("to", "2015-05-16")
                        .withQueryStringParameter("for", "225")
                        .withQueryStringParameter("for_yandex", "1")
        )
                .respond(
                        response(resource("get-holidays-response.json"))
                                .withContentType(MediaType.JSON_UTF_8)
                                .withStatusCode(HttpStatusCode.OK_200.code())
                );

        var holidays = client.getHolidays(LocalDate.of(2015, 4, 15), LocalDate.of(2015, 5, 16), Country.RUSSIA);
        assertThat(holidays).isEqualTo(
                new Holidays(List.of(
                        new Holiday("Новогодние каникулы", LocalDate.of(2015, 1, 1), DayType.HOLIDAY, false, null),
                        new Holiday("Перенос выходного с 31 января",
                                LocalDate.of(2015, 1, 8), DayType.WEEKEND, true, LocalDate.of(2015, 1, 31)),
                        new Holiday("День российской печати", LocalDate.of(2015, 1, 13), DayType.WEEKDAY, false, null)
                ))
        );
    }

    private static String resource(String name) {
        return ResourceUtils.textResource(name);
    }
}

