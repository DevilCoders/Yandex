package ru.yandex.client.common

import com.fasterxml.jackson.databind.DeserializationFeature
import com.fasterxml.jackson.databind.ObjectMapper
import com.fasterxml.jackson.module.kotlin.jacksonObjectMapper
import okhttp3.Interceptor
import okhttp3.OkHttpClient
import retrofit2.Retrofit
import retrofit2.converter.jackson.JacksonConverterFactory

abstract class CommonBuilder<S, C> {
    private fun createOAuthClient(okBuilder: OkHttpClient.Builder?, token: String): OkHttpClient {
        return createClient(okBuilder, "OAuth $token")
    }

    private fun createClient(okBuilder: OkHttpClient.Builder?, authorizationHeaderValue: String): OkHttpClient {
        return (okBuilder ?: OkHttpClient.Builder())
            .addInterceptor { chain: Interceptor.Chain ->
                val request = chain.request().newBuilder()
                    .header("Authorization", authorizationHeaderValue)
                    .header("Accept", "application/json")
                    .build()
                chain.proceed(request)
            }
            .build()
    }

    protected fun createOAuthService(
        okBuilder: OkHttpClient.Builder?,
        baseUrl: String,
        token: String,
        serviceClass: Class<S>
    ): S {
        return Retrofit.Builder()
            .baseUrl(baseUrl)
            .addConverterFactory(JacksonConverterFactory.create(mapper()))
            .client(createOAuthClient(okBuilder, token))
            .build()
            .create(serviceClass)
    }

    abstract fun build(okBuilder: OkHttpClient.Builder? = null): C

    companion object {
        fun mapper(): ObjectMapper {
            return jacksonObjectMapper().configure(DeserializationFeature.FAIL_ON_UNKNOWN_PROPERTIES, false)
        }
    }
}
