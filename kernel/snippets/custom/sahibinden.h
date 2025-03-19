#pragma once

#include <kernel/snippets/replace/replace.h>

namespace NSnippets {

class TSahibindenFakeReplacer : public IReplacer {
public:
    void DoWork(TReplaceManager* manager) override;
    TSahibindenFakeReplacer()
        : IReplacer("sahibinden_fake")
    {
    }
};

class TSahibindenTemplatesReplacer : public IReplacer {
public:
    void DoWork(TReplaceManager* manager) override;
    TSahibindenTemplatesReplacer()
        : IReplacer("sahibinden_template")
    {
    }
};

}
