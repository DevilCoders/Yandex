#include <kernel/text_machine/parts/core/parts_multi_tracker.h>
#include <kernel/text_machine/parts/core/parts_aggregator.h>
#include <kernel/text_machine/parts/core/parts_single_tracker.h>
#include <kernel/text_machine/parts/core/parts_bow_tracker.h>
#include <kernel/text_machine/parts/core/parts_positionless.h>
#include <kernel/text_machine/parts/core/parts_collector.h>

#define TEXT_MACHINE_STATIC_CPP

#include <kernel/text_machine/module/module_def.inc>

// Re-evaluate unit declarations to initialize static unit info
//
#include <kernel/text_machine/parts/core/expansion_trackers.inc>

UNIT_REGISTER(Core, TCoreUnit, NTextMachine::NCore::NCoreParts::TCoreUnitInfo);
UNIT_REGISTER(Core, TCollectorUnit, NTextMachine::NCore::NCoreParts::TCollectorUnitInfo);
UNIT_FAMILY_REGISTER(Core, TPositionlessTrackerFamily, NTextMachine::NCore::NCoreParts::TPositionlessTrackerFamilyInfo);
