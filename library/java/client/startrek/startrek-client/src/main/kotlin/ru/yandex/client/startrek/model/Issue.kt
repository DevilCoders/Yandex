package ru.yandex.client.startrek.model

import com.fasterxml.jackson.annotation.JsonProperty

data class Issue(
    @JsonProperty("self")
    val self: String,

    @JsonProperty("id")
    val id: String,

    @JsonProperty("key")
    val key: String,

    @JsonProperty("version")
    val version: Long,

    @JsonProperty("summary")
    val summary: String,

    @JsonProperty("description")
    val description: String?,

    @JsonProperty("queue")
    val queue: Queue,

    @JsonProperty("status")
    val status: Status,

    @JsonProperty("resolution")
    val resolution: Resolution? = null,

    @JsonProperty("type")
    val type: Type,
) {
    // intentionally not inherited from base type
    // todo: should we make 'id' Long/Int instead of String?
    data class Queue(
        @JsonProperty("self")
        val self: String,

        @JsonProperty("id")
        val id: String,

        @JsonProperty("key")
        val key: String,

        @JsonProperty("display")
        val display: String
    )

    data class Status(
        @JsonProperty("self")
        val self: String,

        @JsonProperty("id")
        val id: String,

        @JsonProperty("key")
        val key: String,

        @JsonProperty("display")
        val display: String
    )

    data class Resolution(
        @JsonProperty("self")
        val self: String,

        @JsonProperty("id")
        val id: String,

        @JsonProperty("key")
        val key: String,

        @JsonProperty("display")
        val display: String
    )

    data class Type(
        @JsonProperty("self")
        val self: String,

        @JsonProperty("id")
        val id: String,

        @JsonProperty("key")
        val key: String,

        @JsonProperty("display")
        val display: String
    )
}
