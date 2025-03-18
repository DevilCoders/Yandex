package ru.yandex.ci.client.calendar.api;

import ru.yandex.ci.client.base.http.retrofit.QueryValue;

public enum OutMode {
    @QueryValue("all")
    ALL, // все дни в заданном интервале;
    @QueryValue("holidays")
    HOLIDAYS, // выходные и праздники;
    @QueryValue("weekdays")
    WEEKDAYS, // рабочие дни;
    @QueryValue("overrides")
    OVERRIDES, // только праздники (не выходные), или переносы;
    @QueryValue("with_names")
    WITH_NAMES, // дни с названиями (праздники, переносы или памятные даты);
    @QueryValue("holidays_and_with_names")
    HOLIDAYS_AND_WITH_NAMES // (дефолт)  объединение holidays и with_names.
}
