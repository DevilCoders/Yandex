package ru.yandex.client.sample

import ru.yandex.client.startrek.StartrekBuilder
import ru.yandex.client.startrek.StartrekConfig

fun main() {
    val config = StartrekConfig(System.getenv("STARTREK_TOKEN"))
    val client = StartrekBuilder(config).build()

    val issues = client.loadIssues(queue = "MOBDEVTOOLS")
    println("Loaded ${issues.size} issues")
    val issue = client.loadIssue(issues[0].id)
    println("Loaded single issue: $issue")
}
