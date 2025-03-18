package ru.yandex.client.startrek

import retrofit2.Call
import retrofit2.http.Body
import retrofit2.http.GET
import retrofit2.http.POST
import retrofit2.http.Path
import retrofit2.http.Query
import ru.yandex.client.startrek.model.Issue
import ru.yandex.client.startrek.model.CreateCommentRequest
import ru.yandex.client.startrek.model.CreateCommentResponse

interface StartrekService {
    /**
     * https://wiki.yandex-team.ru/tracker/api/issues/list/
     */
    @GET("v2/issues")
    fun listIssues(@Query("filter") filter: String): Call<List<Issue>>

    /**
     * https://wiki.yandex-team.ru/tracker/api/issues/get/
     */
    @GET("v2/issues/{issue_id}")
    fun getIssue(@Path("issue_id") issueId: String): Call<Issue>

    /**
     * https://wiki.yandex-team.ru/tracker/api/issues/comments/create/
     */
    @POST("v2/issues/{issue_id}/comments")
    fun createComment(
        @Path("issue_id") issueId: String,
        @Body body: CreateCommentRequest
    ): Call<CreateCommentResponse>
}
