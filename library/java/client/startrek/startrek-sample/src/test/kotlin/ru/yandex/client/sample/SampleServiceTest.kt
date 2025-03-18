package ru.yandex.client.sample

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.client.startrek.FakeStartrekData
import ru.yandex.client.startrek.FakeStartrekService
import ru.yandex.client.startrek.StartrekClient

class SampleServiceTest {
    @Test
    fun test_countIssues() {
        val service = FakeStartrekService()
        val client = StartrekClient(service)
        val sampleService = SampleService(client)
        assertEquals(0, sampleService.countIssues())

        service.addIssue(FakeStartrekData.issue1)
        assertEquals(1, sampleService.countIssues())

        service.addIssue(FakeStartrekData.issue2)
        assertEquals(2, sampleService.countIssues())
    }
}
