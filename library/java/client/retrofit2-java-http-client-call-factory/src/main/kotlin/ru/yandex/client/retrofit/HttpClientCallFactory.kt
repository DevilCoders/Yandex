package ru.yandex.client.retrofit

import okhttp3.Call
import okhttp3.Request
import java.net.http.HttpClient

class HttpClientCallFactory(
    private val client: HttpClient = HttpClient.newHttpClient(),
    private val interceptor: HttpClientInterceptor? = null
) : Call.Factory {
    override fun newCall(request: Request): Call {
        return HttpClientCall(client, request, interceptor)
    }
}
