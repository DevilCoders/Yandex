#include <kernel/text_machine/parts/tracker/parts_annotation.h>
#include <kernel/text_machine/parts/tracker/parts_base.h>
#include <kernel/text_machine/parts/tracker/parts_bow.h>
#include <kernel/text_machine/parts/tracker/parts_positionless.h>
#include <kernel/text_machine/parts/tracker/parts_window.h>

#define TEXT_MACHINE_STATIC_CPP

#include <kernel/text_machine/module/module_def.inc>

// Re-evaluate unit declarations to initialize static unit info
//
#include <kernel/text_machine/parts/tracker/annotation_units.inc>
#include <kernel/text_machine/parts/tracker/bow_units.inc>
#include <kernel/text_machine/parts/tracker/positionless_units.inc>

UNIT_REGISTER(Tracker, TCoreUnit, NTextMachine::NCore::NTrackerParts::TCoreUnitInfo);
UNIT_REGISTER(Tracker, TMinWindowUnit, NTextMachine::NCore::NTrackerParts::TMinWindowUnitInfo);
