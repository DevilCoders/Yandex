#include "composite.h"

namespace NExternalAPI {

    TCompositeRequestCustomizer::TFactory::TRegistrator<TCompositeRequestCustomizer> TCompositeRequestCustomizer::Registrator(TCompositeRequestCustomizer::GetTypeName());

    bool TCompositeRequestCustomizer::DoTuneRequest(const IServiceApiHttpRequest& baseRequest, NNeh::THttpRequest& request, const IRequestCustomizationContext* context, const TSender* sender) const {
        for (auto&& i : OrderedCustomizers) {
            auto it = Customizers.find(i);
            if (it == Customizers.end()) {
                continue;
            }
            if (!it->second->TuneRequest(baseRequest, request, context, sender)) {
                TFLEventLog::Log("cannot rune request")("customization_id", i);
                return false;
            }
        }
        return true;
    }

    void TCompositeRequestCustomizer::DoInit(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        for (auto&& i : children) {
            TRequestCustomizerContainer c;
            c.Init(i.second);
            CHECK_WITH_LOG(!!c);
            if (c->GetIsNecessary()) {
                IsNecessary = true;
            }
            Customizers.emplace(i.first, std::move(c));
            OrderedCustomizers.emplace_back(i.first);
        }
        section->GetDirectives().TryFillArray<TString, char, false>("OrderedCustomizers", OrderedCustomizers);
        CHECK_WITH_LOG(OrderedCustomizers.size() <= Customizers.size());
        for (auto&& i : OrderedCustomizers) {
            CHECK_WITH_LOG(Customizers.contains(i));
        }
    }

    void TCompositeRequestCustomizer::DoToString(IOutputStream& os) const {
        for (auto&& i : OrderedCustomizers) {
            auto it = Customizers.find(i);
            if (it == Customizers.end()) {
                continue;
            }
            os << "<" << it->first << ">" << Endl;
            it->second.ToString(os);
            os << "</" << it->first << ">" << Endl;
        }
    }

    bool TCompositeRequestCustomizer::DoStart(const TSender& client) {
        if (!TBase::DoStart(client)) {
            return false;
        }
        for (auto&& i : Customizers) {
            if (!i.second->Start(client)) {
                return false;
            }
        }
        return true;
    }

    bool TCompositeRequestCustomizer::DoStop() {
        if (!TBase::DoStop()) {
            return false;
        }
        for (auto&& i : Customizers) {
            if (!i.second->Stop()) {
                return false;
            }
        }
        return true;
    }

}
