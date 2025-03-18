package ru.yandex.client.startrek.model

import com.fasterxml.jackson.annotation.JsonProperty

data class CreateCommentResponse(
    @JsonProperty("longId")
    val longId: String
)
