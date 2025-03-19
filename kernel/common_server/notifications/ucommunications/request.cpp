#include "request.h"

bool TSendSmsRequest::BuildHttpRequest(NNeh::THttpRequest& request) const {
    if (!PhoneNumber && !PhoneId) {
        TFLEventLog::Log("Neither PhoneNumber nor PhoneId presents", TLOG_ERR);
        return false;
    }

    NJson::TJsonValue postData;
    if (PhoneNumber) {
        postData["phone"] = PhoneNumber;
    } else {
        postData["phone_id"] = PhoneId;
    }
    postData["sender"] = Sender;
    postData["intent"] = Intent;
    postData["text"] = Message;

    request.SetPostData(TBlob::FromString(postData.GetStringRobust()));
    request.SetUri("user/sms/send");
    return true;
}
