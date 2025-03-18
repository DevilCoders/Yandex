#include "convert_factors.h"
#include "eventlog_err.h"
#include "factor_names.h"

#include <util/generic/ptr.h>
#include <util/generic/singleton.h>
#include <util/generic/vector.h>

namespace {
#define FNAMES_BEGIN(ver) const char* FNamesVer##ver[] = {
#define FNAMES_END }
#define FNAMES_REF(ver) {sizeof(FNamesVer##ver)/sizeof(FNamesVer##ver)[0], FNamesVer##ver}

    struct TTableRef {
        size_t Size;
        const char** Table;
    };

    // TODO refactor
    FNAMES_BEGIN(1)
#include "factors_versions/factors_1.inc"
    FNAMES_END;

    FNAMES_BEGIN(2)
#include "factors_versions/factors_2.inc"
    FNAMES_END;

    FNAMES_BEGIN(3)
#include "factors_versions/factors_3.inc"
    FNAMES_END;

    FNAMES_BEGIN(4)
#include "factors_versions/factors_4.inc"
    FNAMES_END;

    FNAMES_BEGIN(5)
#include "factors_versions/factors_5.inc"
    FNAMES_END;

    FNAMES_BEGIN(6)
#include "factors_versions/factors_6.inc"
    FNAMES_END;

    FNAMES_BEGIN(7)
#include "factors_versions/factors_7.inc"
    FNAMES_END;

    FNAMES_BEGIN(8)
#include "factors_versions/factors_8.inc"
    FNAMES_END;

    FNAMES_BEGIN(9)
#include "factors_versions/factors_9.inc"
    FNAMES_END;

    FNAMES_BEGIN(10)
#include "factors_versions/factors_10.inc"
    FNAMES_END;

    FNAMES_BEGIN(11)
#include "factors_versions/factors_11.inc"
    FNAMES_END;

    FNAMES_BEGIN(12)
#include "factors_versions/factors_12.inc"
    FNAMES_END;

    FNAMES_BEGIN(13)
#include "factors_versions/factors_13.inc"
    FNAMES_END;

    FNAMES_BEGIN(14)
#include "factors_versions/factors_14.inc"
    FNAMES_END;

    FNAMES_BEGIN(15)
#include "factors_versions/factors_15.inc"
    FNAMES_END;

    FNAMES_BEGIN(16)
#include "factors_versions/factors_16.inc"
    FNAMES_END;

    FNAMES_BEGIN(17)
#include "factors_versions/factors_17.inc"
    FNAMES_END;

    FNAMES_BEGIN(18)
#include "factors_versions/factors_18.inc"
    FNAMES_END;

    FNAMES_BEGIN(19)
#include "factors_versions/factors_19.inc"
    FNAMES_END;

    FNAMES_BEGIN(20)
#include "factors_versions/factors_20.inc"
    FNAMES_END;

    FNAMES_BEGIN(21)
#include "factors_versions/factors_21.inc"
    FNAMES_END;

    FNAMES_BEGIN(22)
#include "factors_versions/factors_22.inc"
    FNAMES_END;

    FNAMES_BEGIN(23)
#include "factors_versions/factors_23.inc"
    FNAMES_END;

    FNAMES_BEGIN(24)
#include "factors_versions/factors_24.inc"
    FNAMES_END;

    FNAMES_BEGIN(25)
#include "factors_versions/factors_25.inc"
    FNAMES_END;

    FNAMES_BEGIN(26)
#include "factors_versions/factors_26.inc"
    FNAMES_END;

    FNAMES_BEGIN(27)
#include "factors_versions/factors_27.inc"
    FNAMES_END;

    FNAMES_BEGIN(28)
#include "factors_versions/factors_28.inc"
    FNAMES_END;

    FNAMES_BEGIN(29)
#include "factors_versions/factors_29.inc"
    FNAMES_END;

    FNAMES_BEGIN(30)
#include "factors_versions/factors_30.inc"
    FNAMES_END;

    FNAMES_BEGIN(31)
#include "factors_versions/factors_31.inc"
    FNAMES_END;

    FNAMES_BEGIN(32)
#include "factors_versions/factors_32.inc"
    FNAMES_END;

    FNAMES_BEGIN(33)
#include "factors_versions/factors_33.inc"
    FNAMES_END;

    FNAMES_BEGIN(34)
#include "factors_versions/factors_34.inc"
    FNAMES_END;

    FNAMES_BEGIN(35)
#include "factors_versions/factors_35.inc"
    FNAMES_END;

    FNAMES_BEGIN(36)
#include "factors_versions/factors_36.inc"
    FNAMES_END;

    FNAMES_BEGIN(37)
#include "factors_versions/factors_37.inc"
    FNAMES_END;

    FNAMES_BEGIN(38)
#include "factors_versions/factors_38.inc"
    FNAMES_END;

    FNAMES_BEGIN(39)
#include "factors_versions/factors_39.inc"
    FNAMES_END;

    FNAMES_BEGIN(40)
#include "factors_versions/factors_40.inc"
    FNAMES_END;

    FNAMES_BEGIN(41)
#include "factors_versions/factors_41.inc"
    FNAMES_END;

    FNAMES_BEGIN(42)
#include "factors_versions/factors_42.inc"
    FNAMES_END;

    FNAMES_BEGIN(43)
#include "factors_versions/factors_43.inc"
    FNAMES_END;

    FNAMES_BEGIN(44)
#include "factors_versions/factors_44.inc"
    FNAMES_END;

    FNAMES_BEGIN(45)
#include "factors_versions/factors_45.inc"
    FNAMES_END;

    FNAMES_BEGIN(46)
#include "factors_versions/factors_46.inc"
    FNAMES_END;

    FNAMES_BEGIN(47)
#include "factors_versions/factors_47.inc"
    FNAMES_END;

    FNAMES_BEGIN(48)
#include "factors_versions/factors_48.inc"
    FNAMES_END;

    FNAMES_BEGIN(49)
#include "factors_versions/factors_49.inc"
    FNAMES_END;

    FNAMES_BEGIN(50)
#include "factors_versions/factors_50.inc"
    FNAMES_END;

    FNAMES_BEGIN(51)
#include "factors_versions/factors_51.inc"
    FNAMES_END;

    FNAMES_BEGIN(52)
#include "factors_versions/factors_52.inc"
    FNAMES_END;

    FNAMES_BEGIN(53)
#include "factors_versions/factors_53.inc"
    FNAMES_END;

    FNAMES_BEGIN(54)
#include "factors_versions/factors_54.inc"
    FNAMES_END;

    FNAMES_BEGIN(55)
#include "factors_versions/factors_55.inc"
    FNAMES_END;

    FNAMES_BEGIN(56)
#include "factors_versions/factors_56.inc"
    FNAMES_END;

    FNAMES_BEGIN(57)
#include "factors_versions/factors_57.inc"
    FNAMES_END;

    FNAMES_BEGIN(58)
#include "factors_versions/factors_58.inc"
    FNAMES_END;

    FNAMES_BEGIN(59)
#include "factors_versions/factors_59.inc"
    FNAMES_END;

    FNAMES_BEGIN(60)
#include "factors_versions/factors_60.inc"
    FNAMES_END;

    FNAMES_BEGIN(61)
#include "factors_versions/factors_61.inc"
    FNAMES_END;

    FNAMES_BEGIN(62)
#include "factors_versions/factors_62.inc"
    FNAMES_END;

    FNAMES_BEGIN(63)
#include "factors_versions/factors_63.inc"
    FNAMES_END;

    FNAMES_BEGIN(64)
#include "factors_versions/factors_64.inc"
    FNAMES_END;

    FNAMES_BEGIN(65)
#include "factors_versions/factors_65.inc"
    FNAMES_END;

    FNAMES_BEGIN(66)
#include "factors_versions/factors_66.inc"
    FNAMES_END;

    FNAMES_BEGIN(67)
#include "factors_versions/factors_67.inc"
    FNAMES_END;

    FNAMES_BEGIN(68)
#include "factors_versions/factors_68.inc"
    FNAMES_END;

    FNAMES_BEGIN(69)
#include "factors_versions/factors_69.inc"
    FNAMES_END;

    FNAMES_BEGIN(70)
#include "factors_versions/factors_70.inc"
    FNAMES_END;

    FNAMES_BEGIN(71)
#include "factors_versions/factors_71.inc"
    FNAMES_END;

    FNAMES_BEGIN(72)
#include "factors_versions/factors_72.inc"
    FNAMES_END;

    FNAMES_BEGIN(73)
#include "factors_versions/factors_73.inc"
    FNAMES_END;

    FNAMES_BEGIN(74)
#include "factors_versions/factors_74.inc"
    FNAMES_END;

    FNAMES_BEGIN(75)
#include "factors_versions/factors_75.inc"
    FNAMES_END;

    FNAMES_BEGIN(76)
#include "factors_versions/factors_76.inc"
    FNAMES_END;

    FNAMES_BEGIN(77)
#include "factors_versions/factors_77.inc"
    FNAMES_END;

    FNAMES_BEGIN(78)
#include "factors_versions/factors_78.inc"
    FNAMES_END;

    FNAMES_BEGIN(79)
#include "factors_versions/factors_79.inc"
    FNAMES_END;

    FNAMES_BEGIN(80)
#include "factors_versions/factors_80.inc"
    FNAMES_END;

    FNAMES_BEGIN(81)
#include "factors_versions/factors_81.inc"
    FNAMES_END;

    FNAMES_BEGIN(82)
#include "factors_versions/factors_82.inc"
    FNAMES_END;

    FNAMES_BEGIN(83)
#include "factors_versions/factors_83.inc"
    FNAMES_END;

    FNAMES_BEGIN(84)
#include "factors_versions/factors_84.inc"
    FNAMES_END;

    FNAMES_BEGIN(85)
#include "factors_versions/factors_85.inc"
    FNAMES_END;

    FNAMES_BEGIN(86)
#include "factors_versions/factors_86.inc"
    FNAMES_END;

    FNAMES_BEGIN(87)
#include "factors_versions/factors_87.inc"
    FNAMES_END;

    FNAMES_BEGIN(88)
#include "factors_versions/factors_88.inc"
    FNAMES_END;

    FNAMES_BEGIN(89)
#include "factors_versions/factors_89.inc"
    FNAMES_END;

    FNAMES_BEGIN(90)
#include "factors_versions/factors_90.inc"
    FNAMES_END;

    FNAMES_BEGIN(91)
#include "factors_versions/factors_91.inc"
    FNAMES_END;

    FNAMES_BEGIN(92)
#include "factors_versions/factors_92.inc"
    FNAMES_END;

    FNAMES_BEGIN(93)
#include "factors_versions/factors_93.inc"
    FNAMES_END;

    FNAMES_BEGIN(94)
#include "factors_versions/factors_94.inc"
    FNAMES_END;

    FNAMES_BEGIN(95)
#include "factors_versions/factors_95.inc"
    FNAMES_END;

    FNAMES_BEGIN(96)
#include "factors_versions/factors_96.inc"
    FNAMES_END;

    FNAMES_BEGIN(97)
#include "factors_versions/factors_97.inc"
    FNAMES_END;

    FNAMES_BEGIN(98)
#include "factors_versions/factors_98.inc"
    FNAMES_END;

    FNAMES_BEGIN(99)
#include "factors_versions/factors_99.inc"
    FNAMES_END;

    FNAMES_BEGIN(100)
#include "factors_versions/factors_100.inc"
    FNAMES_END;

    FNAMES_BEGIN(101)
#include "factors_versions/factors_101.inc"
    FNAMES_END;

    FNAMES_BEGIN(102)
#include "factors_versions/factors_102.inc"
    FNAMES_END;

    FNAMES_BEGIN(103)
#include "factors_versions/factors_103.inc"
    FNAMES_END;

    FNAMES_BEGIN(104)
#include "factors_versions/factors_104.inc"
    FNAMES_END;

    FNAMES_BEGIN(105)
#include "factors_versions/factors_105.inc"
    FNAMES_END;

    FNAMES_BEGIN(106)
#include "factors_versions/factors_106.inc"
    FNAMES_END;

    FNAMES_BEGIN(107)
#include "factors_versions/factors_107.inc"
    FNAMES_END;

    FNAMES_BEGIN(108)
#include "factors_versions/factors_108.inc"
    FNAMES_END;

    FNAMES_BEGIN(109)
#include "factors_versions/factors_109.inc"
    FNAMES_END;

    FNAMES_BEGIN(110)
#include "factors_versions/factors_110.inc"
    FNAMES_END;

    FNAMES_BEGIN(111)
#include "factors_versions/factors_111.inc"
    FNAMES_END;

    FNAMES_BEGIN(112)
#include "factors_versions/factors_112.inc"
    FNAMES_END;

    FNAMES_BEGIN(113)
#include "factors_versions/factors_113.inc"
    FNAMES_END;

    FNAMES_BEGIN(114)
#include "factors_versions/factors_114.inc"
    FNAMES_END;

    FNAMES_BEGIN(115)
#include "factors_versions/factors_115.inc"
    FNAMES_END;

    FNAMES_BEGIN(116)
#include "factors_versions/factors_116.inc"
    FNAMES_END;

    TTableRef factorVersions[] = {
        FNAMES_REF(1),
        FNAMES_REF(2),
        FNAMES_REF(3),
        FNAMES_REF(4),
        FNAMES_REF(5),
        FNAMES_REF(6),
        FNAMES_REF(7),
        FNAMES_REF(8),
        FNAMES_REF(9),
        FNAMES_REF(10),
        FNAMES_REF(11),
        FNAMES_REF(12),
        FNAMES_REF(13),
        FNAMES_REF(14),
        FNAMES_REF(15),
        FNAMES_REF(16),
        FNAMES_REF(17),
        FNAMES_REF(18),
        FNAMES_REF(19),
        FNAMES_REF(20),
        FNAMES_REF(21),
        FNAMES_REF(22),
        FNAMES_REF(23),
        FNAMES_REF(24),
        FNAMES_REF(25),
        FNAMES_REF(26),
        FNAMES_REF(27),
        FNAMES_REF(28),
        FNAMES_REF(29),
        FNAMES_REF(30),
        FNAMES_REF(31),
        FNAMES_REF(32),
        FNAMES_REF(33),
        FNAMES_REF(34),
        FNAMES_REF(35),
        FNAMES_REF(36),
        FNAMES_REF(37),
        FNAMES_REF(38),
        FNAMES_REF(39),
        FNAMES_REF(40),
        FNAMES_REF(41),
        FNAMES_REF(42),
        FNAMES_REF(43),
        FNAMES_REF(44),
        FNAMES_REF(45),
        FNAMES_REF(46),
        FNAMES_REF(47),
        FNAMES_REF(48),
        FNAMES_REF(49),
        FNAMES_REF(50),
        FNAMES_REF(51),
        FNAMES_REF(52),
        FNAMES_REF(53),
        FNAMES_REF(54),
        FNAMES_REF(55),
        FNAMES_REF(56),
        FNAMES_REF(57),
        FNAMES_REF(58),
        FNAMES_REF(59),
        FNAMES_REF(60),
        FNAMES_REF(61),
        FNAMES_REF(62),
        FNAMES_REF(63),
        FNAMES_REF(64),
        FNAMES_REF(65),
        FNAMES_REF(66),
        FNAMES_REF(67),
        FNAMES_REF(68),
        FNAMES_REF(69),
        FNAMES_REF(70),
        FNAMES_REF(71),
        FNAMES_REF(72),
        FNAMES_REF(73),
        FNAMES_REF(74),
        FNAMES_REF(75),
        FNAMES_REF(76),
        FNAMES_REF(77),
        FNAMES_REF(78),
        FNAMES_REF(79),
        FNAMES_REF(80),
        FNAMES_REF(81),
        FNAMES_REF(82),
        FNAMES_REF(83),
        FNAMES_REF(84),
        FNAMES_REF(85),
        FNAMES_REF(86),
        FNAMES_REF(87),
        FNAMES_REF(88),
        FNAMES_REF(89),
        FNAMES_REF(90),
        FNAMES_REF(91),
        FNAMES_REF(92),
        FNAMES_REF(93),
        FNAMES_REF(94),
        FNAMES_REF(95),
        FNAMES_REF(96),
        FNAMES_REF(97),
        FNAMES_REF(98),
        FNAMES_REF(99),
        FNAMES_REF(100),
        FNAMES_REF(101),
        FNAMES_REF(102),
        FNAMES_REF(103),
        FNAMES_REF(104),
        FNAMES_REF(105),
        FNAMES_REF(106),
        FNAMES_REF(107),
        FNAMES_REF(108),
        FNAMES_REF(109),
        FNAMES_REF(110),
        FNAMES_REF(111),
        FNAMES_REF(112),
        FNAMES_REF(113),
        FNAMES_REF(114),
        FNAMES_REF(115),
        FNAMES_REF(116),
    };

#undef FNAMES_BEGIN
#undef FNAMES_END
#undef FNAMES_REF

    const size_t factorVersionsSize = sizeof(factorVersions) / sizeof(factorVersions[0]);
    static_assert(factorVersionsSize == NAntiRobot::FACTORS_VERSION - 1, "expect factorVersionsSize == NAntiRobot::FACTORS_VERSION - 1"); // factorVersions must contain all previous versions
} // anonymous namespace

namespace NAntiRobot {
    class TIndexRemap {
        struct TRemap {
            TVector<i32> Indexes;
        };

        TRemap Remaps[FACTORS_VERSION - 1];
    public:
        TIndexRemap() {
            for (size_t i = 0; i < FACTORS_VERSION - 1; i++) {
                TTableRef& tblRef = factorVersions[i];
                TRemap& remap = Remaps[i];

                size_t numSimpleFactors = tblRef.Size;
                for (size_t j = 0; j < tblRef.Size; j++) {
                    TString name = TString(tblRef.Table[j]);
                    if (name.find("|") != TString::npos) {
                        numSimpleFactors = j;
                        break;
                    }

                    size_t index = 0;
                    if (TFactorNames::Instance()->TryGetFactorIndexByName(TString(tblRef.Table[j]), index)) {
                        remap.Indexes.push_back((i32) index);
                    } else {
                        remap.Indexes.push_back(-1);
                    }
                }
                if (i < 84) {
                    Y_ENSURE(numSimpleFactors == tblRef.Size / 4);
                } else {
                    Y_ENSURE(numSimpleFactors == tblRef.Size / TAllFactors::NUM_FACTOR_VALUES);
                }
            }
        }

        inline size_t FactorsCount(ui32 version) const {
            if (version < 1 && version > FACTORS_VERSION - 1)
                ythrow yexception() << "Bad version number";

            return Remaps[version - 1].Indexes.size();
        }

        inline i32 GetNewIndex(ui32 version, ui32 oldIndex) const {
            if (version < 1 && version > FACTORS_VERSION - 1)
                ythrow yexception() << "Bad version number";

            const TRemap& remap = Remaps[version - 1];
            if (oldIndex >= remap.Indexes.size())
                ythrow yexception() << "Bad factor index";

            return remap.Indexes[oldIndex];
        }

        static TIndexRemap* Instance() {
            return Singleton<TIndexRemap>();
        }
    };

    void InitFactorsConvert() {
        (void) TIndexRemap::Instance();
    }

    void ConvertFactors(ui32 fromVersion, const float* srcFactors, TFactorsWithAggregation& dest) {
        if (fromVersion - 1 >= factorVersionsSize) {
            EVLOG_MSG << "Could not convert factors from version: " << fromVersion << Endl;
            return;
        }

        TIndexRemap* ir = TIndexRemap::Instance();

        for (size_t i = 0; i < ir->FactorsCount(fromVersion); i++) {
            i32 newIndex = ir->GetNewIndex(fromVersion, i);
            if (newIndex != -1) {
                dest.GetFactor(newIndex) = srcFactors[i];
            }
        }
    }
} // namespace NAntiRobot
