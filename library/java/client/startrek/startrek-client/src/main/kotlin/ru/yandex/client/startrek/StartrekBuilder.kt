package ru.yandex.client.startrek

import okhttp3.OkHttpClient
import ru.yandex.client.common.CommonBuilder

class StartrekBuilder(private val config: StartrekConfig) : CommonBuilder<StartrekService, StartrekClient>() {
    override fun build(okBuilder: OkHttpClient.Builder?): StartrekClient {
        val service = createOAuthService(
            okBuilder = okBuilder,
            baseUrl = config.baseUrl,
            token = config.token,
            serviceClass = StartrekService::class.java,
        )
        return StartrekClient(service = service)
    }
}
