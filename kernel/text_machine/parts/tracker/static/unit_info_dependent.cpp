// We have to separate FieldSet units from other units to avoid MSVC compiler bug;
// when MSVC encounters a duplicate template using statement (technically legal),
// it starts to infer random errors in unrelated code.
// parts_field_set.h includes annotation_units.inc,
// so we cannot include parts_field_set.h and annotation_units.inc in the same cpp.
// The same goes for parts_atten.h+parts_plane.h and positionless_units.inc.

#include <kernel/text_machine/parts/tracker/parts_atten.h>
#include <kernel/text_machine/parts/tracker/parts_field_set.h>
#include <kernel/text_machine/parts/tracker/parts_plane.h>

#define TEXT_MACHINE_STATIC_CPP

#include <kernel/text_machine/module/module_def.inc>

#include <kernel/text_machine/parts/tracker/field_set_units.inc>

UNIT_REGISTER(Tracker, TTextAttenUnit, NTextMachine::NCore::NTrackerParts::TTextAttenUnitInfo);
UNIT_REGISTER(Tracker, TTextAttenExactUnit, NTextMachine::NCore::NTrackerParts::TTextAttenExactUnitInfo);
UNIT_REGISTER(Tracker, TFieldSet1Unit, NTextMachine::NCore::NTrackerParts::TFieldSet1UnitInfo);
UNIT_REGISTER(Tracker, TFieldSet2Unit, NTextMachine::NCore::NTrackerParts::TFieldSet2UnitInfo);
UNIT_REGISTER(Tracker, TFieldSet3Unit, NTextMachine::NCore::NTrackerParts::TFieldSet3UnitInfo);
UNIT_REGISTER(Tracker, TFieldSet4Unit, NTextMachine::NCore::NTrackerParts::TFieldSet4UnitInfo);
UNIT_REGISTER(Tracker, TFieldSet5Unit, NTextMachine::NCore::NTrackerParts::TFieldSet5UnitInfo);
UNIT_REGISTER(Tracker, TFieldSetUTUnit, NTextMachine::NCore::NTrackerParts::TFieldSetUTUnitInfo);
UNIT_FAMILY_REGISTER(Tracker, TPlaneAccumulatorFamily, NTextMachine::NCore::NTrackerParts::TPlaneAccumulatorFamilyInfo);
UNIT_REGISTER(Tracker,
    TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19Unit,
    NTextMachine::NCore::NTrackerParts::TFieldSetForDssmCountersForTextFieldSetWithSSHardsLongClicksPredictorVersFeb19UnitInfo);
UNIT_REGISTER(Tracker,
    TFieldSetForDssmSSHardWordWeightsFeaturesUnit,
    NTextMachine::NCore::NTrackerParts::TFieldSetForDssmSSHardWordWeightsFeaturesUnitInfo);
UNIT_REGISTER(Tracker,
    TFieldSetForFullSplitBertCountersUnit,
    NTextMachine::NCore::NTrackerParts::TFieldSetForFullSplitBertCountersUnitInfo);
