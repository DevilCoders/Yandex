package ru.yandex.client.startrek

data class StartrekConfig(
    val token: String,
    val baseUrl: String = "https://st-api.yandex-team.ru"
)
