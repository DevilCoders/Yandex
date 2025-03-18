package ru.yandex.client.sample

import ru.yandex.client.startrek.StartrekClient

class SampleService(private val startrekClient: StartrekClient) {
    fun countIssues(): Int {
        val issues = startrekClient.loadIssues(queue = QUEUE)
        return issues.size
    }

    companion object {
        const val QUEUE = "STARTREK"
    }
}
