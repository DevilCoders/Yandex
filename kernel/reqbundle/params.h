#pragma once

#include "reqbundle.h"

namespace NReqBundle {
    // Set of parameters to switch
    // computation of factors
    // per expansion type
    class TReqBundleExpansionsParams {
        bool DefaultOn = true;
        enum EExpansionMode {
            EmEnable,
            EmDisable
        };
        TMap<EExpansionType, EExpansionMode> ExpModes;

    public:
        void EnableExpansions(EExpansionType type) {
            ExpModes[type] = EmEnable;
        }
        void DisableExpansions(EExpansionType type) {
            ExpModes[type] = EmDisable;
        }
        void DisableByDefault() {
            DefaultOn = false;
        }
        void EnableByDefault() {
            DefaultOn = true;
        }
        void DisableAll() {
            DisableByDefault();
            ExpModes.clear();
        }
        void EnableAll() {
            EnableByDefault();
            ExpModes.clear();
        }

        bool IsExplicitlyEnabled(EExpansionType type) const {
            const auto iter = ExpModes.find(type);
            return iter != ExpModes.end() && iter->second == EmEnable;
        }
        bool IsExplicitlyDisabled(EExpansionType type) const {
            const auto iter = ExpModes.find(type);
            return iter != ExpModes.end() && iter->second == EmDisable;
        }
        bool IsEnabled(EExpansionType type) const {
            return IsExplicitlyEnabled(type) ||
                (!IsExplicitlyDisabled(type) && DefaultOn);
        }
    };
} // NReqBundle
