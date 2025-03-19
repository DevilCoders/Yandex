#pragma once

#include <kernel/common_server/ut/scripts/default_actions.h>
#include <kernel/common_server/ut/test_base.h>
#include <kernel/common_server/solutions/fallback_proxy/src/server/server.h>

#include <kernel/common_server/tags/abstract.h>
#include <kernel/common_server/processors/emulation/handler.h>
#include <library/cpp/testing/common/env.h>

class TFallbackConfigGenerator: public NServerTest::TConfigGenerator {
public:
    TFallbackConfigGenerator() {
        SetMonitoringPort(GetRandomPort());
    }

    virtual TString GetExternalQueuesConfiguration() const override;
    virtual TString GetProcessorsConfiguration() const override;
    virtual TString GetAuthConfiguration() const override;
    virtual TString GetCustomManagersConfiguration() const override;
    virtual TString GetExternalApiConfiguration() const override;
};

class TFallbackServerGuard: public NServerTest::TServerGuard<NCS::NFallbackProxy::TServer, NCS::NFallbackProxy::TServerConfig> {
private:
    using TBase = NServerTest::TServerGuard<NCS::NFallbackProxy::TServer, NCS::NFallbackProxy::TServerConfig>;

public:
    using TConfigGenerator = TFallbackConfigGenerator;
    using TBase::TBase;
};

class TFallbackTestContext: public NServerTest::ITestContext {
private:
    using TBase = NServerTest::ITestContext;
    CSA_DEFAULT(TFallbackTestContext, TString, ContextId);

public:
    using TBase::TBase;
};

class TFallbackTestBase: public NServerTest::TAbstractTestCase<TFallbackServerGuard> {
    using TBase = NServerTest::TAbstractTestCase<TFallbackServerGuard>;

protected:
    using TTestContext = TFallbackTestContext;
    virtual bool FillSpecialDBFeatures(NServerTest::TConfigGenerator& configGenerator) const override;

    void OnTestStart();

    void PrepareSettings(TVector<NFrontend::TSetting>& settings) override;

    virtual void PrepareBackgrounds(TVector<TRTBackgroundProcessContainer>& rtbg) override;
};
