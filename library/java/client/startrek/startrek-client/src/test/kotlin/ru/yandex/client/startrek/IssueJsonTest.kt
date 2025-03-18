package ru.yandex.client.startrek

import com.fasterxml.jackson.module.kotlin.readValue
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.client.common.CommonBuilder
import ru.yandex.client.startrek.model.Issue

class IssueJsonTest {
    @Test
    fun deserialize() {
        val mapper = CommonBuilder.mapper()
        val url = requireNotNull(this.javaClass.getResource("/issue/list.json"))
        val result = mapper.readValue<List<Issue>>((url))
        assertEquals(10, result.size)
        assertEquals(
            Issue(
                self = "https://st-api.yandex-team.ru/v2/issues/STARTREK-4126",
                id = "539ee230e4b08d60c73ee99d",
                key = "STARTREK-4126",
                version = 1402927540266L,
                summary = "Сломался стартрек",
                description = "Закончились лимиты из-за проксирования поиска в джиру",
                queue = Issue.Queue(
                    self = "https://st-api.yandex-team.ru/v2/queues/STARTREK",
                    id = "111",
                    key = "STARTREK",
                    display = "Стартрек"
                ),
                status = Issue.Status(
                    self = "https://st-api.yandex-team.ru/v2/statuses/3",
                    id = "3",
                    key = "closed",
                    display = "Closed"
                ),
                resolution = Issue.Resolution(
                    self = "https://st-api.yandex-team.ru/v2/resolutions/1",
                    id = "1",
                    key = "fixed",
                    display = "Fixed"
                ),
                type = Issue.Type(
                    self = "https://st-api.yandex-team.ru/v2/issuetypes/1",
                    id = "1",
                    key = "bug",
                    display = "Bug"
                )
            ), result[0]
        )
        assertEquals(
            Issue(
                self = "https://st-api.yandex-team.ru/v2/issues/STARTREK-4125",
                id = "539edc09e4b05e332b6e8395",
                key = "STARTREK-4125",
                version = 1402927547260L,
                summary = "Перенести на продакшн STATKEY — любое время",
                description = "oircn_resolve\n\nTask\nBug",
                queue = Issue.Queue(
                    self = "https://st-api.yandex-team.ru/v2/queues/STARTREK",
                    id = "111",
                    key = "STARTREK",
                    display = "Стартрек"
                ),
                status = Issue.Status(
                    self = "https://st-api.yandex-team.ru/v2/statuses/1",
                    id = "1",
                    key = "open",
                    display = "Open"
                ),
                type = Issue.Type(
                    self = "https://st-api.yandex-team.ru/v2/issuetypes/2",
                    id = "2",
                    key = "task",
                    display = "Task"
                )
            ), result[1]
        )
    }
}
