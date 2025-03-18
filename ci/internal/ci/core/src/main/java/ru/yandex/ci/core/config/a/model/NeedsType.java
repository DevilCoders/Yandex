package ru.yandex.ci.core.config.a.model;

import com.fasterxml.jackson.annotation.JsonAlias;
import com.fasterxml.jackson.annotation.JsonProperty;

public enum NeedsType {
    /*
     * Когда ровно N из N апстримов готово, N >= 0
     */
    @JsonProperty("all")
    @JsonAlias("ALL")
    ALL,

    /*
     * Когда 1 <= k <= N апстримов готово из N, N >= 0
     */
    @JsonProperty("any")
    @JsonAlias("ANY")
    ANY,

    /*
     * Когда есть хотя бы один из апстримов, который завершился ошибкой
     */
    @JsonProperty("fail")
    @JsonAlias("FAIL")
    FAIL
}
