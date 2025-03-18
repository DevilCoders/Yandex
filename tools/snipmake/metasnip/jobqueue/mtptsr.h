#pragma once

#include <util/thread/pool.h>

namespace NSnippets {
    struct IThreadSpecificResourceManager {
        virtual void* CreateThreadSpecificResource() = 0;
        virtual void DestroyThreadSpecificResource(void*) = 0;
        virtual ~IThreadSpecificResourceManager() {
        }
    };
    struct TNoThreadDataManager : IThreadSpecificResourceManager {
        void* CreateThreadSpecificResource() override {
            return nullptr;
        }
        void DestroyThreadSpecificResource(void*) override {
        }
    };
    struct TMtpQueueTsr : public TThreadPool {
    private:
        IThreadSpecificResourceManager* const TSRManager;
        void* CreateThreadSpecificResource() override {
            return TSRManager->CreateThreadSpecificResource();
        }
        void DestroyThreadSpecificResource(void* tsr) override {
            TSRManager->DestroyThreadSpecificResource(tsr);
        }
    public:
        TMtpQueueTsr(IThreadSpecificResourceManager* tsrManager)
          : TSRManager(tsrManager)
        {
        }
        ~TMtpQueueTsr() override {
            try {
                Stop();
            } catch (...) {
            }
        }
    };
}
