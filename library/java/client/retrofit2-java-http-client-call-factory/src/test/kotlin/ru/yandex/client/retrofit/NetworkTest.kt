package ru.yandex.client.retrofit

import okhttp3.mockwebserver.MockResponse
import okhttp3.mockwebserver.MockWebServer
import org.junit.jupiter.api.AfterEach
import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.BeforeEach
import org.junit.jupiter.api.Test
import org.junit.jupiter.api.assertThrows
import retrofit2.Retrofit
import retrofit2.converter.jackson.JacksonConverterFactory
import java.net.http.HttpClient

class NetworkTest {
    private val token = "dummy-token"
    private val server = MockWebServer()

    @BeforeEach
    fun beforeEach() {
        server.enqueue(MockResponse().setBody("""["s1", "s2", "s3"]"""))
        server.start()
    }

    @AfterEach
    fun afterEach() {
        server.shutdown()
    }

    @Test
    fun test_Client() {
        val client = createClient()
        val call = client.get("world")

        // first call goes ok
        call.execute()

        val request = server.takeRequest()
        assertEquals("OAuth $token", request.headers["Authorization"])

        // second call throws IllegalStateException
        assertThrows<IllegalStateException> { call.execute() }
    }

    @Test
    fun test_Get() {
        val client = createClient()
        val call = client.get("world")

        val response = requireNotNull(call.execute().body())
        assertEquals(listOf("s1", "s2", "s3"), response)

        val request = server.takeRequest()
        assertEquals("world", requireNotNull(request.requestUrl).queryParameter("hello"))
    }

    private fun createClient(): SampleService {
        return Retrofit.Builder()
            .baseUrl(server.url("/"))
            .addConverterFactory(JacksonConverterFactory.create())
            .callFactory(HttpClientCallFactory(HttpClient.newHttpClient(), createOAuthInterceptor(token))).build()
            .create(SampleService::class.java)
    }

    private fun createOAuthInterceptor(token: String): HttpClientInterceptor {
        return HttpClientInterceptor { requestBuilder -> requestBuilder.header("Authorization", "OAuth $token") }
    }
}
