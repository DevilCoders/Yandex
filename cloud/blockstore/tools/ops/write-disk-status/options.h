#pragma once

#include <cloud/blockstore/public/api/protos/disk.pb.h>

#include <util/datetime/base.h>
#include <util/generic/string.h>

////////////////////////////////////////////////////////////////////////////////

struct TOptions
{
    enum class EDiskState {
        Online      /* "online"      */ ,
        Migration   /* "migration" */ ,
        Unavailable /* "unavailable" */ ,
        Error       /* "error"       */
    };

    enum class ELogLevel {
        Error   /* "error" */ = 3,
        Warn    /* "warn"  */ = 4,
        Info    /* "info"  */ = 6,
        Debug   /* "debug" */ = 7
    };

    TString DiskId;
    EDiskState DiskState = EDiskState::Online;
    TString Message;

    TString Topic;
    TString SourceId;
    TString Endpoint;
    TString Database;
    bool ReconnectOnFailure = false;
    TDuration Timeout;
    ui64 SeqNo = 0;

    bool UseOAuth = false;
    TString IamJwtKeyFilename;
    TString IamEndpoint;
    TString CaCertFilename;

    ELogLevel VerboseLevel = ELogLevel::Error;

    void Parse(int argc, char** argv);
};
