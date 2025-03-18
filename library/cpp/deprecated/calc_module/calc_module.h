#pragma once

#include "access_point_info.h"
#include "access_points.h"

#include "map_helpers.h"

#include <util/string/vector.h>

class TAccessPointException: public yexception {
};

class TWrongPointNameException: public TAccessPointException {
};

class TAbsentPointException: public TAccessPointException {
};

class TDuplicatePointException: public TAccessPointException {
};

class TNotConnectedPointException: public TAccessPointException {
};

class ICalcModule {
public:
    ICalcModule(const TString&) {
    }
    virtual ~ICalcModule() = default;

    /*
     * Gets all names of points (set of points could change)
     */
    virtual TSet<TString> GetPointNames() const = 0;

    /*
     * Gets name of this module
     */
    virtual const TString& GetName() const = 0;

    /*
     * Gets named access point
     */
    virtual IAccessPoint& GetAccessPoint(const TString& name) = 0;

    /*
     * Gets hints structure for a named access point
     */
    virtual const TAccessPointInfo* GetAccessPointInfo(const TString& name) const = 0;

    /*
     * Checks that all internal modules are connected and all master access points
     * are connected. Every composite modules must check this after (re)connecting
     * modules before using them. Throws an exception if unready
     */
    virtual void CheckReady() const = 0;

    void Connect(const TString& name, IAccessPoint& accessPoint) {
        Connect(GetAccessPoint(name), accessPoint);
    }
    static void Connect(IAccessPoint& accessPoint1, IAccessPoint& accessPoint2) {
        accessPoint1.Connect(accessPoint2);
    }
    static void MagicConnect(ICalcModule& module1, ICalcModule& module2, TString pointName) {
        bool connected = false;
        TString errorMessage;
        TString linkName;
        typedef std::pair<TString, TString> TNameVariant;
        TNameVariant pointNames;
        if (!pointName) {
            linkName = "*";
            TNameVariant variant("input", "output");
            if (connected = module1.TryConnection(module2, variant, errorMessage))
                pointNames = variant;
        } else {
            linkName = pointName;
            TString inputPointName = pointName;
            TString outputPointName = pointName;
            inputPointName += "_input";
            outputPointName += "_output";
            TNameVariant variants[4] = {
                TNameVariant(inputPointName, outputPointName),
                TNameVariant(pointName, pointName),
                TNameVariant(inputPointName, "output"),
                TNameVariant("input", outputPointName)};
            for (size_t i = 0; !connected && i < 4; i++) {
                if (connected = module1.TryConnection(module2, variants[i], errorMessage))
                    pointNames = variants[i];
            }
        }
        if (!connected) {
            Complain(module1, module2, linkName, errorMessage);
        }
    }
    static void MagicConnect(ICalcModule& module1, TString pointName1, ICalcModule& module2, TString pointName2) {
        TString errorMessage;
        TString linkName = pointName1 + "-" + pointName2;
        std::pair<TString, TString> variant(pointName1, pointName2);
        if (!module1.TryConnection(module2, variant, errorMessage)) {
            Complain(module1, module2, linkName, errorMessage);
        }
    }

private:
    bool TryConnection(ICalcModule& module2, const std::pair<TString, TString>& pointNames, TString& errorMessage) noexcept {
        try {
            Connect(pointNames.first, module2.GetAccessPoint(pointNames.second));
            return true;
        } catch (const TIncompatibleAccessPoints& e) {
            if (!!errorMessage)
                errorMessage += " ";
            errorMessage += e.what();
        } catch (...) {
        }
        return false;
    }
    static inline void Complain(
        const ICalcModule& module1,
        const ICalcModule& module2,
        const TString& linkName,
        const TString& errorMessage) {
        const TString& moduleName1 = module1.GetName();
        const TString& moduleName2 = module2.GetName();
        ythrow yexception() << "MagicConnect is unable to connect \"" << moduleName1 << "\" and \"" << moduleName2 << "\" by \"" << linkName << "\" link pattern" << ((!errorMessage) ? "" : ", because \n") << errorMessage;
    }
};

using TCalcModuleHolder = TAtomicSharedPtr<ICalcModule>;
using TCalcModuleSet = TSet<TCalcModuleHolder, TSharedPtrLess<TCalcModuleHolder>>;

void MagicConnect(ICalcModule& module1, ICalcModule& module2, const TString& pointName = "");
void MagicConnect(ICalcModule& module1, TCalcModuleHolder module2, const TString& pointName = "");
void MagicConnect(TCalcModuleHolder module1, ICalcModule& module2, const TString& pointName = "");
void MagicConnect(TCalcModuleHolder module1, TCalcModuleHolder module2, const TString& pointName = "");
void MagicConnect(ICalcModule& module1, const TString& pointName1, ICalcModule& module2, const TString& pointName2);
void MagicConnect(ICalcModule& module1, const TString& pointName1, TCalcModuleHolder module2, const TString& pointName2);
void MagicConnect(TCalcModuleHolder module1, const TString& pointName1, ICalcModule& module2, const TString& pointName2);
void MagicConnect(TCalcModuleHolder module1, const TString& pointName1, TCalcModuleHolder module2, const TString& pointName2);
void MagicBatchConnect(ICalcModule& module1, ICalcModule& module2, TVector<TString> pointsNames);
void MagicBatchConnect(ICalcModule& module1, ICalcModule& module2, const TString& pointsNames);
void MagicBatchConnect(ICalcModule& module1, TCalcModuleHolder module2, const TString& pointsNames);
void MagicBatchConnect(TCalcModuleHolder module1, ICalcModule& module2, const TString& pointsNames);
void MagicBatchConnect(TCalcModuleHolder module1, TCalcModuleHolder module2, const TString& pointsNames);
