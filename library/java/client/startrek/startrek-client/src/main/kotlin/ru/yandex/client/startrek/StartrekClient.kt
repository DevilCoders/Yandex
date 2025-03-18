package ru.yandex.client.startrek

import ru.yandex.client.startrek.model.CreateCommentRequest
import ru.yandex.client.startrek.model.Issue

class StartrekClient(private val service: StartrekService) {
    fun loadIssues(queue: String): List<Issue> {
        return requireNotNull(service.listIssues(filter = "queue:$queue").execute().body())
    }

    fun loadIssue(issueId: String): Issue {
        return service.getIssue(issueId = issueId).execute().body()!!
    }

    fun createComment(issueId: String, body: String): String {
        val createCommentResponse = service.createComment(issueId = issueId, body = CreateCommentRequest(body)).execute().body()!!
        val commentId = createCommentResponse.longId
        // todo: host should be configurable
        return "https://st.yandex-team.ru/$issueId#$commentId" // todo: build url properly
    }
}
