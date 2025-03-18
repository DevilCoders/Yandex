package ru.yandex.client.retrofit

import retrofit2.Call
import retrofit2.http.GET
import retrofit2.http.Query

interface SampleService {
    @GET("get")
    fun get(@Query("hello") hello: String): Call<List<String>>
}
