package ru.yandex.client.startrek.model

import com.fasterxml.jackson.annotation.JsonProperty

data class CreateCommentRequest(
    @JsonProperty("text")
    val text: String
)
