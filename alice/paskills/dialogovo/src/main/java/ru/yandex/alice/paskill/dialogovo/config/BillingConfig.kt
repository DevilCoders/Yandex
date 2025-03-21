package ru.yandex.alice.paskill.dialogovo.config

import org.springframework.boot.context.properties.ConfigurationProperties
import org.springframework.boot.context.properties.ConstructorBinding

@ConstructorBinding
@ConfigurationProperties("billing-config")
class BillingConfig(
    override val url: String,
    override val timeout: Int,
    override val connectTimeout: Int
) : EndpointConfig
