#include "user_metric.h"


namespace NAntiRobot {

// TODO: переделать на TOwningThreadedLogBackend и применить параметры, как в TUnifiedAgentLogBackend
TUnifiedAgentBillingLog::TUnifiedAgentBillingLog(const TString& unifiedAgentUri) {
    ResetBackend(
        unifiedAgentUri.empty() || !ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService ?
            THolder<TLogBackend>(new TNullLogBackend) :
            THolder<TLogBackend>(new TUnifiedAgentLogBackend(unifiedAgentUri, "billinglog"))
    );
}

TUnifiedAgentResourceMetrics::TUnifiedAgentResourceMetrics(const TString& unifiedAgentUri) {
    ResetBackend(
        unifiedAgentUri.empty() || !ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService ?
            THolder<TLogBackend>(new TNullLogBackend) :
            THolder<TLogBackend>(new TUnifiedAgentLogBackend(unifiedAgentUri, "resource_metrics_log"))
    );
}

void TUnifiedAgentResourceMetrics::Track(ECaptchaUserMetricKey key, const TCaptchaSettingsPtr& settings) {
    if (!settings) {
        return;
    }
    if (!settings->Getcaptcha_id()) {
        return;
    }
    
    ui64 timestamp = TInstant::Now().Seconds();

    TString logString;
    TStringOutput so(logString);
    NJsonWriter::TBuf json(NJsonWriter::HEM_DONT_ESCAPE_HTML, &so);
    json.BeginObject();
    {
        json.WriteKey("labels").BeginObject();
        {
            json.WriteKey("name").WriteString(ToString(key));
            json.WriteKey("host").WriteString(ShortHostName());
            json.WriteKey("cloud_id").WriteString(settings->Getcloud_id());
            json.WriteKey("folder_id").WriteString(settings->Getfolder_id());
            json.WriteKey("captcha").WriteString(settings->Getcaptcha_id());
        }
        json.EndObject();

        json.WriteKey("value").WriteInt(1);
        json.WriteKey("type").WriteString("IGAUGE");
        json.WriteKey("ts").WriteULongLong(timestamp);
    }
    json.EndObject();
    *this << "{\"metrics\":[" << logString << "]}" << Endl;
}

} // namespace NAntiRobot
