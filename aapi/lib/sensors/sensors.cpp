#include "sensors.h"

#include <aapi/lib/trace/events.ev.pb.h>
#include <aapi/lib/trace/trace.h>

#include <library/cpp/monlib/deprecated/json/writer.h>

#include <library/cpp/neh/rpc.h>

#include <util/thread/factory.h>
#include <util/system/event.h>
#include <util/system/mutex.h>
#include <util/generic/hash.h>

namespace {

class TSensorsThread: public IThreadFactory::IThreadAble {
public:
    TSensorsThread(const TString& host, ui64 port)
        : Host(host)
        , Port(port)
    {
        Thread = SystemThreadFactory()->Run(this);
    }

    void InitSensor(const TString& name, i64 value, bool modeDeriv) {
        TGuard<TMutex> guard(Mutex);
        SensorsMap[name] = {value, modeDeriv};
    }

    void SetSensor(const TString& name, i64 value) {
        TGuard<TMutex> guard(Mutex);
        SensorsMap[name].Value = value;
    }

    void AddToSensor(const TString& name, i64 add) {
        TGuard<TMutex> guard(Mutex);
        SensorsMap[name].Value += add;
    }

    ~TSensorsThread() {
        Stopped.Signal();
        Thread->Join();
    }

private:
    struct TSensor {
        i64 Value;
        bool ModeDeriv;
    };

    class TSensorsService: public NNeh::IService {
    public:
        explicit TSensorsService(TSensorsThread* sensors)
            : Sensors(sensors)
        {
        }

        void ServeRequest(const NNeh::IRequestRef& request) override {
            TString dataStr = Sensors->DumpSensors();
            NNeh::TDataSaver dataSaver;
            dataSaver.DoWrite(dataStr.data(), dataStr.size());
            NAapi::Trace(NAapi::TSensorsRequest(dataStr.size()));
            request->SendReply(dataSaver);
        }

    private:
        TSensorsThread* Sensors;
    };

    using TSensorsMap = THashMap<TString, TSensor>;

    const TString Host;
    const ui64 Port;

    TAutoEvent Stopped;
    THolder<IThreadFactory::IThread> Thread;

    mutable TMutex Mutex;
    TSensorsMap SensorsMap;

    TString DumpSensors() const {
        TSensorsMap sensors;

        with_lock(Mutex) {
            sensors = SensorsMap;
        }

        TStringStream data;
        NMonitoring::TDeprecatedJsonWriter writer(&data);

        writer.OpenDocument();

        const ui64 ts = Now().Seconds();

        // sensors
        writer.OpenMetrics();
        for (auto it: sensors) {
            // sensor
            writer.OpenMetric();

            // labels
            writer.OpenLabels();
            writer.WriteLabel("sensor", it.first);
            writer.CloseLabels();

            // value
            writer.WriteValue(it.second.Value);

            // deriv or ts
            if (it.second.ModeDeriv) {
                writer.WriteModeDeriv();
            } else {
                writer.WriteTs(ts);
            }

            writer.CloseMetric();
        }
        writer.CloseMetrics();

        writer.CloseDocument();

        return data.Str();
    }

    void DoExecute() override {
        NNeh::IServicesRef httpServer = NNeh::CreateLoop();
        httpServer->Add("http2://" + Host + ":" + ToString(Port) + "/sensors", NNeh::IServiceRef(new TSensorsService(this)));
        httpServer->ForkLoop(2);
        Stopped.Wait();
    }
};

class TSensorsThreadHolder {
public:
    void Init(const TString& host, ui64 port) {
        Sensors = MakeHolder<TSensorsThread>(host, port);
    }

    void InitSensor(const TString& name, i64 value, bool modeDeriv) {
        Sensors->InitSensor(name, value, modeDeriv);
    }

    void SetSensor(const TString& name, i64 value) {
        Sensors->SetSensor(name, value);
    }

    void AddToSensor(const TString& name, i64 add) {
        Sensors->AddToSensor(name, add);
    }

    static TSensorsThreadHolder* Instance() {
        return SingletonWithPriority<TSensorsThreadHolder, 120000>();
    };

private:
    THolder<TSensorsThread> Sensors;
};

}  // namespace

void NAapi::InitSolomonSensors(const TString& host, ui64 port) {
    TSensorsThreadHolder::Instance()->Init(host, port);
}

void NAapi::InitSensor(const TString& sensor, i64 value, bool modeDeriv) {
    TSensorsThreadHolder::Instance()->InitSensor(sensor, value, modeDeriv);
}

void NAapi::SetSensor(const TString& sensor, i64 value) {
    TSensorsThreadHolder::Instance()->SetSensor(sensor, value);
}

void NAapi::AddToSensor(const TString& sensor, i64 add) {
    TSensorsThreadHolder::Instance()->AddToSensor(sensor, add);
}
