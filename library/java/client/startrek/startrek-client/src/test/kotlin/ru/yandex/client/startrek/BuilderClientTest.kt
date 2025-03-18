package ru.yandex.client.startrek

import okhttp3.Interceptor
import okhttp3.OkHttpClient
import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test

class BuilderClientTest {
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
    fun testCustomClient() {
        val config = StartrekConfig(token = "dummy-token", baseUrl = server.url("/").toString())
        val customBuilder = OkHttpClient.Builder()
        customBuilder.addInterceptor { chain: Interceptor.Chain ->
            val request = chain.request().newBuilder()
                .header("custom", "header")
                .build()
            chain.proceed(request)
        }
        val client = StartrekBuilder(config).build(customBuilder)
        client.loadIssues(queue = "dummy")

        val headers = server.takeRequest().headers
        assertEquals("header", headers["custom"])
    }
}
