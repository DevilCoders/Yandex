#include <cloud/kms/client/cpp/kmsclient.h>

#include <library/cpp/getopt/last_getopt.h>
#include <library/cpp/logger/filter.h>
#include <library/cpp/logger/global/global.h>

#include <util/generic/algorithm.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/string/split.h>
#include <util/system/thread.h>

ELogPriority ParseLogPriority(const TString& s) {
    if (s == "debug" || s == "DEBUG") {
        return TLOG_DEBUG;
    } else if (s == "trace" || s == "TRACE") {
        return TLOG_RESOURCES;
    } else {
        return TLOG_INFO;
    }
}

struct TWorkerResult {
    TDuration encryptP50;
    TDuration encryptP95;
    TDuration encryptP99;
    TDuration encryptP100;

    TDuration decryptP50;
    TDuration decryptP95;
    TDuration decryptP99;
    TDuration decryptP100;

    int errors = 0;
};

void WorkerMain(NKMS::IClient* client, const TString& keyId, int workerNum, int numRequests,
                size_t dataSize, TWorkerResult& result) {
    TVector<char> plaintext;
    TVector<char> aad;
    TVector<char> ciphertext;
    TVector<char> newPlaintext;

    plaintext.resize(dataSize, 0x12);
    ciphertext.reserve(dataSize * 2);
    newPlaintext.reserve(dataSize);

    aad.resize(dataSize, (char)workerNum);

    TVector<TDuration> encryptTimes;
    TVector<TDuration> decryptTimes;
    encryptTimes.reserve(numRequests / 2);
    decryptTimes.reserve(numRequests / 2);

    for (int i = 0; i < numRequests; i++) {
        if (i % 1000 == 0) {
            INFO_LOG << "Worker #" << workerNum << " request " << i << "\n";
        }

        if (i % 2 == 0) {
            // Use different plaintext for each request.
            plaintext[0] = (char)i;

            TInstant start = TInstant::Now();
            grpc::Status st = client->Encrypt(keyId, aad, plaintext, ciphertext);
            TDuration delta = TInstant::Now() - start;
            if (!st.ok()) {
                ERROR_LOG << "Worker #" << workerNum << " failed to encrypt, status code: "
                          << int(st.error_code()) << ", message: " << st.error_message() << "\n";
                result.errors++;
                // Skip the next decrypt.
                i++;
                continue;
            }
            encryptTimes.push_back(delta);
        } else {
            TInstant start = TInstant::Now();
            grpc::Status st = client->Decrypt(keyId, aad, ciphertext, newPlaintext);
            TDuration delta = TInstant::Now() - start;
            if (!st.ok()) {
                ERROR_LOG << "Worker #" << workerNum << " failed to decrypt, status code: "
                          << int(st.error_code()) << ", message: " << st.error_message() << "\n";
                result.errors++;
                continue;
            }
            if (plaintext != newPlaintext) {
                FATAL_LOG << "plaintext and newPlaintext mismatch\n";
                exit(1);
            }
            decryptTimes.push_back(delta);
        }
    }

    Sort(encryptTimes);
    if (!encryptTimes.empty()) {
        size_t s = encryptTimes.size();
        result.encryptP50 = encryptTimes[s / 2];
        result.encryptP95 = encryptTimes[s * 95 / 100];
        result.encryptP99 = encryptTimes[s * 99 / 100];
        result.encryptP100 = encryptTimes[s - 1];
    }
    Sort(decryptTimes);
    if (!decryptTimes.empty()) {
        size_t s = encryptTimes.size();
        result.decryptP50 = decryptTimes[s / 2];
        result.decryptP95 = decryptTimes[s * 95 / 100];
        result.decryptP99 = decryptTimes[s * 99 / 100];
        result.decryptP100 = decryptTimes[s - 1];
    }
}

struct TKMSLogBackend : public TLogBackend {
    ELogPriority Priority;

    TKMSLogBackend(ELogPriority priority)
        : Priority(priority) {
    }

    void WriteData(const TLogRecord& rec) override {
        Cerr << "KMS: " << rec.Priority << ": " << TStringBuf(rec.Data, rec.Len);
    }

    void ReopenLog() override {
    }

    ELogPriority FiltrationLevel() const override {
        return Priority;
    }
};

int main(int argc, char** argv) {
    DoInitGlobalLog("console", TLOG_DEBUG, false, false);

    NLastGetopt::TOpts opts;
    opts.AddLongOption("addrs", "comma-separated list of endpoints to connect to")
        .Required();
    opts.AddLongOption("token", "IAM token")
        .Required();
    opts.AddLongOption("tls-server-name", "replace hostname in TLS validation (useful for directly connecting to backends)")
        .DefaultValue("");
    opts.AddLongOption("insecure", "disable TLS")
        .NoArgument();
    opts.AddLongOption("key-id", "key ID to use for encryption/decryption")
        .Required();
    opts.AddLongOption("requests", "number of encrypt/decrypt requests to do per thread")
        .DefaultValue("100");
    opts.AddLongOption("threads", "number of threads submitting encrypt/decrypt requests")
        .DefaultValue("1");
    opts.AddLongOption("retries", "maximum number of retries per request, 0 for default")
        .DefaultValue("0");
    opts.AddLongOption("data-size", "size of plaintext")
        .DefaultValue("32");
    opts.AddLongOption("log-level", "additional logging for the KMS client")
        .DefaultValue("info");
    opts.AddHelpOption();
    opts.SetFreeArgsNum(0);

    TVector<TString> addrs;
    TString tlsServerName;
    bool insecure;
    TString iamToken;
    TString keyId;
    int numRequests;
    int numThreads;
    int numRetries;
    size_t dataSize;
    ELogPriority logPriority;
    try {
        NLastGetopt::TOptsParseResult result(&opts, argc, argv);
        addrs = StringSplitter(result.Get("addrs")).Split(',');
        tlsServerName = result.Get("tls-server-name");
        insecure = result.Has("insecure");
        iamToken = result.Get("token");
        keyId = result.Get("key-id");
        numRequests = FromString(result.Get("requests"));
        numThreads = FromString(result.Get("threads"));
        numRetries = FromString(result.Get("retries"));
        dataSize = FromString(result.Get("data-size"));
        logPriority = ParseLogPriority(result.Get("log-level"));
    } catch (const yexception& ex) {
        DEBUG_LOG << ex.what();
        opts.PrintUsage("kms-example");
        return 1;
    }

    TLog log(MakeHolder<TKMSLogBackend>(logPriority));
    NKMS::TRoundRobinClientOptions options;
    options.TokenProvider = [&]() { return iamToken; };
    options.Log = &log;
    options.MaxRetries = numRetries;
    options.TLSServerName = tlsServerName;
    options.Insecure = insecure;
    std::unique_ptr<NKMS::IClient> client{NKMS::CreateRoundRobinClient(addrs, options)};

    INFO_LOG << "Starting " << numThreads << " workers\n";
    TVector<std::unique_ptr<TThread>> workers;
    TVector<TWorkerResult> results;
    results.resize(numThreads);
    NKMS::IClient* clientPtr = client.get();
    for (int i = 0; i < numThreads; i++) {
        workers.emplace_back(new TThread([=, &results]() {
            WorkerMain(clientPtr, keyId, i, numRequests, dataSize, results[i]);
        }));
    }
    TInstant startTime = TInstant::Now();
    for (auto& worker : workers) {
        worker->Start();
    }
    for (auto& worker : workers) {
        worker->Join();
    }
    uint64_t delta = (TInstant::Now() - startTime).MilliSeconds();
    if (delta == 0) {
        delta = 1;
    }

    uint64_t totalRequests = numRequests * numThreads;
    int rps = totalRequests * 1000 / delta;
    int totalErrors = 0;
    for (int i = 0; i < numThreads; i++) {
        INFO_LOG << "Times for worker#" << i
                 << ": decrypt p50: " << results[i].decryptP50.MicroSeconds() << "usec"
                 << ", p95: " << results[i].decryptP95.MicroSeconds() << "usec"
                 << ", p99: " << results[i].decryptP99.MicroSeconds() << "usec"
                 << ", p100: " << results[i].decryptP100.MicroSeconds() << "usec"
                 << ", encrypt p50: " << results[i].encryptP50.MicroSeconds() << "usec"
                 << ", p95: " << results[i].encryptP95.MicroSeconds() << "usec"
                 << ", p99: " << results[i].encryptP99.MicroSeconds() << "usec"
                 << ", p100: " << results[i].encryptP100.MicroSeconds() << "usec"
                 << "\n";
        totalErrors += results[i].errors;
    }

    INFO_LOG << "Success. Total RPS: " << rps << ", total errors: " << totalErrors << "\n";

    return 0;
}
