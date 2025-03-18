package ru.yandex.client.retrofit

import okhttp3.Call
import okhttp3.Callback
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import okhttp3.Protocol
import okhttp3.Request
import okhttp3.Response
import okhttp3.ResponseBody.Companion.toResponseBody
import okio.Buffer
import okio.Timeout
import java.io.IOException
import java.net.http.HttpClient
import java.net.http.HttpRequest
import java.net.http.HttpResponse
import java.util.concurrent.CompletableFuture
import java.util.concurrent.TimeUnit
import java.util.concurrent.atomic.AtomicReference

class HttpClientCall(
    private val client: HttpClient,
    private val okRequest: Request,
    private val interceptor: HttpClientInterceptor?
) : Call {
    private val futureRef = AtomicReference<CompletableFuture<Void>>()

    override fun request(): Request {
        return okRequest
    }

    @Throws(IOException::class)
    override fun execute(): Response {
        return try {
            val response = client.send(prepareHttpRequest(), HttpResponse.BodyHandlers.ofByteArray())
            toOkhttpResponse(response)
        } catch (e: InterruptedException) {
            // todo: not sure about this
            throw IOException(e)
        }
    }

    override fun enqueue(responseCallback: Callback) {
        try {
            val future = client
                .sendAsync(prepareHttpRequest(), HttpResponse.BodyHandlers.ofByteArray())
                .thenAccept { response ->
                    try {
                        responseCallback.onResponse(this, toOkhttpResponse(response))
                    } catch (e: IOException) {
                        responseCallback.onFailure(this, e)
                    }
                }
            check(futureRef.compareAndSet(null, future)) { "Already executed!" }
        } catch (e: IOException) {
            responseCallback.onFailure(this, e)
        }
    }

    override fun cancel() {
        val future = futureRef.get()
        if (future != null && !future.isDone) {
            future.cancel(true)
        }
    }

    override fun clone(): Call {
        return HttpClientCall(client, okRequest, interceptor)
    }


    override fun isCanceled(): Boolean {
        val future = futureRef.get()
        return future != null && future.isCancelled
    }

    override fun isExecuted(): Boolean {
        val future = futureRef.get()
        return future != null && future.isDone
    }

    override fun timeout(): Timeout {
        val timeout = Timeout()
        client.connectTimeout().ifPresent { duration -> timeout.timeout(duration.toMillis(), TimeUnit.MILLISECONDS) }
        return timeout
    }

    @Throws(IOException::class)
    private fun prepareHttpRequest(): HttpRequest {
        val builder = HttpRequest.newBuilder().uri(okRequest.url.toUri())

        // interceptor
        interceptor?.intercept(builder)

        // headers
        val headers = okRequest.headers
        headers.names().forEach { name -> headers.values(name).forEach { value -> builder.header(name, value) } }

        // body
        val body = okRequest.body
        if (body != null && body.contentLength() > 0) {
            val contentType = body.contentType()
            if (contentType != null) {
                builder.setHeader(CONTENT_TYPE, contentType.toString())
            }
            val okioBuffer = Buffer()
            body.writeTo(okioBuffer)
            builder.method(okRequest.method, HttpRequest.BodyPublishers.ofByteArray(okioBuffer.readByteArray()))
        } else {
            builder.method(okRequest.method, HttpRequest.BodyPublishers.noBody())
        }

        return builder.build()
    }

    private fun toOkhttpResponse(httpResponse: HttpResponse<ByteArray>): Response {
        val okResponseBuilder = Response.Builder().request(request())

        // status code
        okResponseBuilder.code(httpResponse.statusCode()).message("") // todo

        // protocol
        when (requireNotNull(httpResponse.version())) {
            HttpClient.Version.HTTP_1_1 -> okResponseBuilder.protocol(Protocol.HTTP_1_1)
            HttpClient.Version.HTTP_2 -> okResponseBuilder.protocol(Protocol.HTTP_2)
        }

        // headers
        val headers = httpResponse.headers()
        headers.map().forEach { (key, values) -> values.forEach { value -> okResponseBuilder.addHeader(key, value) } }

        // body
        val body = httpResponse.body()
        if (body != null) {
            val contentType = headers.allValues(CONTENT_TYPE).singleOrNull()
            val mediaType = contentType?.toMediaTypeOrNull()
            okResponseBuilder.body(body.toResponseBody(mediaType))
        }

        return okResponseBuilder.build()
    }

    companion object {
        const val CONTENT_TYPE = "Content-Type"
    }
}
