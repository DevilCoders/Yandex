package ru.yandex.client.startrek

import retrofit2.Call
import retrofit2.mock.Calls
import ru.yandex.client.startrek.model.CreateCommentRequest
import ru.yandex.client.startrek.model.CreateCommentResponse
import ru.yandex.client.startrek.model.Issue
import kotlin.random.Random

class FakeStartrekService : StartrekService {
    private val issues = mutableListOf<Issue>()
    private val comments = mutableMapOf<String, MutableList<String>>()

    private fun extractQueueName(filter: String): String {
        return filter.substringAfter("queue:")
    }

    override fun listIssues(filter: String): Call<List<Issue>> {
        // todo: throw some exceptions
        val filtered = issues.filter { it.queue.key == extractQueueName(filter) }
        return Calls.response(filtered)
    }

    override fun getIssue(issueId: String): Call<Issue> {
        // todo: throw some exceptions
        val filtered = issues.first { it.key == issueId }
        return Calls.response(filtered)
    }

    override fun createComment(issueId: String, body: CreateCommentRequest): Call<CreateCommentResponse> {
        // todo: throw some exceptions
        val comments = comments.computeIfAbsent(issueId) { mutableListOf() }
        // todo: create `data class Comment` to store commentId and body text
        comments.add(body.text)
        val commentId = Random.nextLong().toString()
        return Calls.response(CreateCommentResponse(commentId))
    }

    fun addIssue(issue: Issue) {
        issues.add(issue)
    }

    fun removeIssue(issue: Issue) {
        issues.remove(issue)
    }

    fun getComments(issueId: String): List<String> {
        return comments[issueId].orEmpty()
    }
}
