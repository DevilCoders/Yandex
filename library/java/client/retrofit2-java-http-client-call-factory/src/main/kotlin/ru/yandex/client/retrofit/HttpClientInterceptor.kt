package ru.yandex.client.retrofit

import java.net.http.HttpRequest

fun interface HttpClientInterceptor {
    fun intercept(requestBuilder: HttpRequest.Builder)
}
