package ru.yandex.client.startrek

import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test

class MockNetworkTest {
    private val server = MockWebServer()
    private val listJson = requireNotNull(this.javaClass.getResource("/issue/list.json")).readText()

    @BeforeEach
    fun beforeEach() {
        server.enqueue(MockResponse().setBody(listJson))
        server.start()
    }

    @AfterEach
    fun afterEach() {
        server.shutdown()
    }

    @Test
    fun testLoadIssues() {
        val baseUrl = server.url("/")
        val token = "dummy-token"
        val config = StartrekConfig(token = token, baseUrl = baseUrl.toString())
        val client = StartrekBuilder(config).build()

        val issues = client.loadIssues(queue = "dummy")
        assertEquals(10, issues.size)
        assertEquals("STARTREK-4126", issues[0].key)

        val headers = server.takeRequest().headers
        assertEquals("OAuth $token", headers["Authorization"])
    }
}
