package ru.yandex.client.startrek

import org.junit.jupiter.api.Assertions.assertEquals
import org.junit.jupiter.api.Test
import ru.yandex.client.startrek.model.CreateCommentRequest
import ru.yandex.client.startrek.model.Issue

class FakeStartrekServiceTest {
    @Test
    fun test_addRemove() {
        val service = FakeStartrekService()
        val issue1 = FakeStartrekData.issue1
        val issue2 = FakeStartrekData.issue2

        assertEquals(emptyList<Issue>(), service.listIssues(issue1.queue.key).execute().body())

        service.addIssue(issue1)
        service.addIssue(issue2)
        assertEquals(listOf(issue1, issue2), service.listIssues(issue1.queue.key).execute().body())

        service.removeIssue(issue1)
        assertEquals(listOf(issue2), service.listIssues(issue1.queue.key).execute().body())
    }

    @Test
    fun test_addComment() {
        val service = FakeStartrekService()
        val issue1 = FakeStartrekData.issue1
        val issue2 = FakeStartrekData.issue2

        assertEquals(emptyList<String>(), service.getComments(issue1.id))
        assertEquals(emptyList<String>(), service.getComments(issue2.id))

        service.createComment(issue1.id, CreateCommentRequest("hello"))
        assertEquals(listOf("hello"), service.getComments(issue1.id))
        assertEquals(listOf<String>(), service.getComments(issue2.id))
    }
}
