package yandex.cloud.kms.s3fuzzer

import com.amazonaws.AmazonWebServiceRequest
import com.amazonaws.ClientConfiguration
import com.amazonaws.Protocol
import com.amazonaws.auth.AWSStaticCredentialsProvider
import com.amazonaws.auth.BasicAWSCredentials
import com.amazonaws.client.builder.AwsClientBuilder
import com.amazonaws.retry.RetryPolicy
import com.amazonaws.services.s3.AmazonS3
import com.amazonaws.services.s3.AmazonS3ClientBuilder
import com.amazonaws.services.s3.model.*
import org.apache.commons.cli.*
import org.apache.commons.io.IOUtils
import java.io.ByteArrayInputStream
import java.io.File
import java.time.Duration
import java.time.Instant
import java.util.*
import java.util.concurrent.*
import java.util.concurrent.atomic.AtomicLong
import java.util.concurrent.locks.ReentrantLock
import javax.net.ssl.SSLException
import kotlin.concurrent.withLock
import kotlin.math.exp
import kotlin.math.ln
import kotlin.math.min
import kotlin.random.Random
import kotlin.system.exitProcess

const val minSizeForMultipart = 10_000_000

// Fail the test is there are at least minConnectionResets and they represent at least
// connectionResetErrorThreshold of all reqiests.
const val minConnectionResets = 10
const val connectionResetErrorThreshold = 0.01

val lastReq = ThreadLocal<AmazonWebServiceRequest>()

data class ObjectInfo(val key: String, val size: Int, val seed: Long)

data class Range(val from: Int, val to: Int)
data class GenerateParams(val ranges: List<Range>)

data class Client(val s3: AmazonS3, val bucket: String, val executor: ExecutorService)

class ExecutorQueue<E> : ArrayBlockingQueue<E>(1) {
    override fun offer(e: E): Boolean {
        put(e)
        return true
    }
}

fun main(args: Array<String>) {
    try {
        mainImpl(args)
    } catch (e: Exception) {
        println("%s: Exception in main: %s".format(Instant.now().toString(), e))
        e.printStackTrace()
        exitProcess(1)
    }
}

data class CmdOptions(
        val endpoint: String,
        val accessKey: String,
        val secretKey: String,
        val bucket: String,
        val bucketSizeInMb: Long,
        val generateParams: GenerateParams,
        val generatePerSecond: Float,
        val numThreads: Int,
        val runFor: Duration,
)

fun mainImpl(args: Array<String>) {
    val options = parseOptions(args) ?: return

    val s3Client = makeS3Client(options)

    println("Starting %d threads to run for %s".format(options.numThreads, options.runFor))
    val executor = ThreadPoolExecutor(options.numThreads, options.numThreads, 60, TimeUnit.SECONDS, ExecutorQueue())
    val client = Client(s3Client, options.bucket, executor)
    // Run put requests in a separate thread pool: multipart uploads then will be run in the main executor, otherwise
    // we may get deadlocked.
    val putExecutor = ThreadPoolExecutor(options.numThreads, options.numThreads, 60, TimeUnit.SECONDS, ExecutorQueue())

    val pauseBetweenGenerate = Duration.ofMillis((1000.0 / options.generatePerSecond).toLong())
    var lastObjectGenerated = Instant.EPOCH

    deleteAllInBucket(client)

    val startTime = Instant.now()

    var currentBucketSize = 0
    // Halve the quota size just in case the quota is recomputed with delay.
    val maxBucketSize = options.bucketSizeInMb * 1024 * 1024 / 2

    val r = Random(System.nanoTime())
    // Pair of (S3 key, seed)
    val generatedObjects = ArrayList<ObjectInfo>()
    val generatedObjectsLock = ReentrantLock()
    var hasOneObject = false
    val numConnectionResets = AtomicLong(0L)
    val totalRequests = AtomicLong(0L)
    while (true) {
        val now = Instant.now()
        val elapsed = Duration.between(startTime, now)
        if (elapsed > options.runFor) {
            break
        }
        println("%s elapsed".format(elapsed))

        // Create new object and stored it into generateObjects once in a while
        if (now.minus(pauseBetweenGenerate).isAfter(lastObjectGenerated)) {
            val objectSize = genSize(options.generateParams)
            if (objectSize + currentBucketSize >= maxBucketSize) {
                // Horrible hack: sleep so that the concurrently running uploads should have finished.
                println("Sleeping before cleaning bucket")
                Thread.sleep(50 * 1000)
                deleteAllInBucket(client)
                println("Sleeping after cleaning bucket")
                Thread.sleep(50 * 1000)
                currentBucketSize = 0
                generatedObjectsLock.withLock {
                    generatedObjects.clear()
                }
                hasOneObject = false
            }
            currentBucketSize += objectSize

            val seed = r.nextLong()
            val key = "%d-%d".format(now.toEpochMilli(), seed)
            val info = ObjectInfo(key, objectSize, seed)
            val future = putExecutor.submit {
                try {
                    val useMultipart = if (objectSize > minSizeForMultipart) r.nextBoolean() else false
                    totalRequests.incrementAndGet()
                    generateAndPutObject(info, client, useMultipart)
                } catch (e: Exception) {
                    if (checkIgnorableException(e, numConnectionResets, totalRequests)) {
                        return@submit;
                    }
                    println("%s: ERROR: Exception in generateAndPutObject %s, request-id: %s: %s"
                            .format(Instant.now().toString(), key, getLastRequestId(client.s3), e))
                    if (e is AmazonS3Exception) {
                        println("%s: ERROR: Additional info: %s".format(Instant.now().toString(), e.errorResponseXml))
                    }
                    e.printStackTrace()
                    exitProcess(1)
                }
                generatedObjectsLock.withLock {
                    generatedObjects.add(info)
                }
            }
            if (!hasOneObject) {
                // Wait until at least one object is generated.
                future.get()
                hasOneObject = true
            }
            lastObjectGenerated = now
        }

        // Choose a random object, read and check the contents.
        val info = generatedObjectsLock.withLock {
            generatedObjects[r.nextInt(generatedObjects.size)]
        }
        val useRange = r.nextBoolean()
        val range = if (useRange) {
            val from = r.nextInt(0, info.size - 1)
            val to = r.nextInt(from + 1, info.size)
            Range(from, to)
        } else {
            Range(0, 0)
        }
        executor.submit {
            try {
                totalRequests.incrementAndGet()
                checkObject(info, client, useRange, range)
            } catch (e: Exception) {
                if (checkIgnorableException(e, numConnectionResets, totalRequests)) {
                    return@submit;
                }
                println("%s: ERROR: Exception in checkObject %s (range %d-%d), request-id: %s: %s"
                        .format(Instant.now().toString(), info.key, range.from, range.to, getLastRequestId(client.s3), e))
                if (e is AmazonS3Exception) {
                    println("%s: ERROR: Additional info: %s".format(Instant.now().toString(), e.errorResponseXml))
                }
                e.printStackTrace()
                exitProcess(1)
            }
        }
    }

    // NOTE: We must stop putExecutor before the executor to allow generateAndPutObject spawn all the subtasks.
    putExecutor.shutdown()
    putExecutor.awaitTermination(1, TimeUnit.MINUTES)
    executor.shutdown()
    executor.awaitTermination(1, TimeUnit.MINUTES)
    println("All OK!")
    deleteAllInBucket(client)
}

fun checkIgnorableException(e: Exception, numConnectionResets: AtomicLong, totalRequests: AtomicLong): Boolean {
    // Yes, comparing messages is the only way, see SocketInputStream::read.
    if (e is SSLException && e.message.equals("Connection reset")) {
        val curResets = numConnectionResets.incrementAndGet()
        if (curResets < minConnectionResets || curResets.toDouble() / totalRequests.get() < connectionResetErrorThreshold) {
            return true;
        }
        println("Got %d connection resets per %d requests".format(curResets, totalRequests.get()))
        return false;
    } else {
        return false;
    }
}

fun getLastRequestId(s3: AmazonS3): String {
    if (lastReq.get() != null) {
        return s3.getCachedResponseMetadata(lastReq.get()).requestId
    } else {
        return "null"
    }
}

fun makeS3Client(options: CmdOptions): AmazonS3 {
    return AmazonS3ClientBuilder.standard()
            .withEndpointConfiguration(AwsClientBuilder.EndpointConfiguration(
                    options.endpoint, "ru-central1"
            ))
            .withCredentials(AWSStaticCredentialsProvider(BasicAWSCredentials(options.accessKey, options.secretKey)))
            .withClientConfiguration(
                    ClientConfiguration()
                            .withProtocol(Protocol.HTTPS)
                            // See executor and putExecutor
                            .withMaxConnections(options.numThreads * 4)
                            .withConnectionTimeout(1000)
                            .withSocketTimeout(60 * 1000)
                            .withConnectionMaxIdleMillis(60 * 1000)
                            .withConnectionTTL(60 * 1000)
                            .withRetryPolicy(RetryPolicy(null, null, 10, true)))
            .build()
}

fun parseOptions(args: Array<String>): CmdOptions? {
    val options = Options()
    options.addOption(Option.builder()
            .longOpt("endpoint")
            .hasArg()
            .required()
            .desc("S3 endpoint, e.g. storage.yandexcloud.net")
            .type(String::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("access-key")
            .hasArg()
            .required()
            .desc("S3 access key")
            .type(String::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("secret-key-file")
            .hasArg()
            .desc("File with S3 secret key, if not specified, will be read from SECRET_KEY_FILE environment variable")
            .type(String::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("bucket")
            .hasArg()
            .required()
            .desc("S3 bucket")
            .type(String::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("bucket-size")
            .hasArg()
            .required()
            .desc("Size of S3 bucket in megabytes")
            .type(Number::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("size-ranges")
            .hasArg()
            .required()
            .desc("S3 object size range, list of integer pairs, e.g. 10-100,10000-100000")
            .type(String::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("generate-per-second")
            .hasArg()
            .desc("Number of objects to generate per second, 1 by default")
            .type(Number::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("num-threads")
            .hasArg()
            .desc("Number of threads, 1 by default")
            .type(Number::class.java)
            .build())
    options.addOption(Option.builder()
            .longOpt("run-for")
            .hasArg()
            .desc("Run for given period, 1h by default")
            .type(String::class.java)
            .build())

    val parser = DefaultParser()

    val cmd = try {
        parser.parse(options, args)
    } catch (e: ParseException) {
        val help = HelpFormatter()
        help.printHelp("kms-s3-fuzzer [OPTIONS]", "", options, e.message)
        return null
    }

    val endpoint = cmd.getParsedOptionValue("endpoint") as String
    val accessKey = cmd.getParsedOptionValue("access-key") as String
    val bucket = cmd.getParsedOptionValue("bucket") as String
    val bucketSizeInMb = ((cmd.getParsedOptionValue("bucket-size") ?: 1000) as Number).toLong()
    val generateParams = parseGenerateParams(cmd.getParsedOptionValue("size-ranges") as String)
    val generatePerSecond = ((cmd.getParsedOptionValue("generate-per-second") ?: 1.0f) as Number).toFloat()
    val numThreads = ((cmd.getParsedOptionValue("num-threads") ?: 1) as Number).toInt()
    val runForStr = cmd.getOptionValue("run-for", "PT1H") as String

    val secretKey = if (cmd.hasOption("secret-key-file")) {
        val secretKeyFile = cmd.getParsedOptionValue("secret-key-file") as String
        File(secretKeyFile).readText(Charsets.UTF_8)
    } else {
        System.getenv("SECRET_KEY_FILE")
    }
    if (secretKey == null) {
        println("Either --secret-key-file or SECRET_KEY_FILE must be specified")
        return null
    }

    val runFor = Duration.parse(runForStr)

    return CmdOptions(
            endpoint,
            accessKey,
            secretKey,
            bucket,
            bucketSizeInMb,
            generateParams,
            generatePerSecond,
            numThreads,
            runFor,
    )
}

fun deleteAllInBucket(client: Client) {
    println("Deleting all objects in bucket %s".format(client.bucket))
    var continuationToken: String? = null

    val startTime = System.nanoTime()
    var totalObjects = 0
    while (true) {
        var req = ListObjectsV2Request()
                .withBucketName(client.bucket)
        if (continuationToken != null) {
            req = req.withContinuationToken(continuationToken)
        }
        val list = client.s3.listObjectsV2(req)
        continuationToken = list.continuationToken
        val keys = list.objectSummaries
                .map { DeleteObjectsRequest.KeyVersion(it.key) }
                .toList()
        client.s3.deleteObjects(DeleteObjectsRequest(client.bucket)
                .withKeys(keys))
        totalObjects += keys.size
        if (!list.isTruncated) {
            break
        }
    }
    val elapsedMsec = (System.nanoTime() - startTime) / 1000000
    println("Deleted %d objects in %dmsec".format(totalObjects, elapsedMsec))
}

fun parseGenerateParams(s: String): GenerateParams {
    val ranges = ArrayList<Range>()
    for (part in s.split(',')) {
        val parts = part.trim().split('-')
        if (parts.size != 2) {
            throw RuntimeException("wrong size range: $s")
        }
        val range = Range(parts[0].toInt(), parts[1].toInt())
        if (range.from >= range.to) {
            throw RuntimeException("range from is greater than to: $range")
        }
        ranges.add(range)
    }
    return GenerateParams(ranges)
}

fun generateAndPutObject(info: ObjectInfo, client: Client, useMultipart: Boolean) {
    val bytes = ByteArray(info.size)
    val random = Random(info.seed)
    random.nextBytes(bytes)

    val startTime = System.nanoTime()
    if (useMultipart) {
        val req = InitiateMultipartUploadRequest(client.bucket, info.key)
        lastReq.set(req)
        val upload = client.s3.initiateMultipartUpload(req)
        val metadata = client.s3.getCachedResponseMetadata(req)
        println("Request-id for initiate multipart upload %s: %s (host %s)".format(info.key, metadata.requestId,
                metadata.hostId))
        val futures = ArrayList<Future<PartETag>>()
        var partNumber = 1
        for (start in 0..info.size step minSizeForMultipart) {
            val end = min(start + minSizeForMultipart, info.size)
            val thisPartNumber = partNumber
            val uploader: () -> PartETag = {
                val uploadReq = UploadPartRequest()
                        .withBucketName(client.bucket)
                        .withKey(info.key)
                        .withUploadId(upload.uploadId)
                        .withPartSize((end - start).toLong())
                        .withPartNumber(thisPartNumber)
                        .withInputStream(ByteArrayInputStream(bytes, start, end - start))
                lastReq.set(uploadReq)
                val p = client.s3.uploadPart(uploadReq)
                p.partETag
            }
            futures.add(client.executor.submit(uploader))
            partNumber++
        }

        val partEtags = futures.map { it.get() }.toList()
        val completeReq = CompleteMultipartUploadRequest(client.bucket, info.key, upload.uploadId,
                partEtags)
        lastReq.set(completeReq)
        client.s3.completeMultipartUpload(completeReq)
        val elapsedMsec = (System.nanoTime() - startTime) / 1_000_000
        println("Put multipart %s (%dkb) in %dms".format(info.key, info.size / 1024, elapsedMsec))
    } else {
        val metadata = ObjectMetadata()
        metadata.contentLength = info.size.toLong()
        client.executor.submit {
            val req = PutObjectRequest(client.bucket, info.key, ByteArrayInputStream(bytes), metadata)
            lastReq.set(req)
            client.s3.putObject(req)
        }.get()
        val elapsedMsec = (System.nanoTime() - startTime) / 1_000_000
        println("Put %s (%dkb) in %dms".format(info.key, info.size / 1024, elapsedMsec))
    }
    lastReq.set(null)
}

fun genSize(genParams: GenerateParams): Int {
    val range = genParams.ranges[Random.nextInt(genParams.ranges.size)]
    // Bias the lower sizes.
    val a = ln(range.from.toDouble())
    val b = ln(range.to.toDouble())
    val c = a + Random.nextFloat() * (b - a)
    val v = exp(c)
    return v.toInt()
}

fun checkObject(info: ObjectInfo, client: Client, useRangeRead: Boolean, range: Range) {
    val bytes = ByteArray(info.size)
    val random = Random(info.seed)
    random.nextBytes(bytes)

    val startTime = System.nanoTime()

    if (useRangeRead) {
        val req = GetObjectRequest(client.bucket, info.key)
                .withRange(range.from.toLong(), range.to.toLong())
        lastReq.set(req)
        val gotObject = client.s3.getObject(req)
        gotObject.use {
            val gotContents = IOUtils.readFully(gotObject.objectContent, range.to - range.from)
            gotObject.objectContent.abort()
            if (!Arrays.equals(gotContents, 0, range.to - range.from, bytes, range.from, range.to)) {
                println("ERROR: Contents differ for %s, request-id %s, test failed!"
                        .format(info.key, getLastRequestId(client.s3)))
                exitProcess(1)
            }
        }
        val elapsedMsec = (System.nanoTime() - startTime) / 1_000_000
        println("Checked range read object %s (%dkb) in %dms".format(info.key, (range.to - range.from) / 1024, elapsedMsec))
    } else {
        val req = GetObjectRequest(client.bucket, info.key)
        lastReq.set(req)
        val gotObject = client.s3.getObject(req)
        gotObject.use {
            val gotContents = IOUtils.readFully(gotObject.objectContent, info.size)
            gotObject.objectContent.abort()
            if (!Arrays.equals(gotContents, bytes)) {
                println("ERROR: Contents differ for %s, request-id %s, test failed!"
                        .format(info.key, getLastRequestId(client.s3)))
                exitProcess(1)
            }
        }
        val elapsedMsec = (System.nanoTime() - startTime) / 1_000_000
        println("Checked object %s (%dkb) in %dms".format(info.key, info.size / 1024, elapsedMsec))
    }
    lastReq.set(null)
}
