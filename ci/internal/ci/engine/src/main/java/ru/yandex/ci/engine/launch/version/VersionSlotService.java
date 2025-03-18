package ru.yandex.ci.engine.launch.version;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.function.Supplier;
import java.util.stream.Collectors;

import javax.annotation.Nonnull;

import lombok.RequiredArgsConstructor;
import lombok.extern.slf4j.Slf4j;

import ru.yandex.ci.core.arc.OrderedArcRevision;
import ru.yandex.ci.core.config.CiProcessId;
import ru.yandex.ci.core.db.CiMainDb;
import ru.yandex.ci.core.launch.versioning.Slot;
import ru.yandex.ci.core.launch.versioning.Version;
import ru.yandex.ci.core.launch.versioning.Versions;

@Slf4j
@RequiredArgsConstructor
public class VersionSlotService {

    @Nonnull
    private final CiMainDb db;

    public Version nextVersion(
            SlotFor slotType,
            CiProcessId processId,
            OrderedArcRevision revision,
            Supplier<Version> nextGenerator) {

        Versions versions = db.versions().getVersionsOrCreate(processId, revision);
        NextVersion nextVersion = getNextVersion(versions, slotType, nextGenerator);

        db.versions().save(nextVersion.getVersions());

        return nextVersion.getNext();
    }

    public void occupySlot(SlotFor slotType, CiProcessId processId, OrderedArcRevision revision, Version version) {
        log.info("Occupy version {} for {} at {}, process {}", version, slotType, revision, processId);
        Versions versions = db.versions().getVersionsOrCreate(processId, revision);
        var slots = new ArrayList<>(versions.getSlots());

        slots.stream()
                .filter(s -> s.getVersion().equals(version))
                .findFirst()
                .ifPresentOrElse(slot -> {
                    log.info("Occupy existing slot {}", slot);
                    slots.remove(slot);
                    slots.add(slotType.occupy(slot));
                }, () -> {
                    Slot slot = slotWithVersion(version);
                    log.info("Create new occupied slot {}", slot);
                    slots.add(slotType.occupy(slot));
                });

        db.versions().save(versions.toBuilder().slots(slots).build());
    }

    private NextVersion getNextVersion(Versions versions, SlotFor slotType, Supplier<Version> nextGenerator) {
        List<Slot> slots = versions.getSlots()
                .stream()
                .sorted(Comparator.<Slot>naturalOrder().reversed())
                .collect(Collectors.toList());

        if (slots.isEmpty() || !slotType.matches(slots.get(0))) {
            log.info("Not found available version slot for {}, generating new", slotType);
            Slot slot = slotType.occupy(slotWithVersion(nextGenerator.get()));

            slots.add(slot);

            Versions updatedVersions = versions.toBuilder()
                    .slots(slots)
                    .build();

            return NextVersion.of(updatedVersions, slot.getVersion());
        }

        Slot availableSlot = slots.get(0);
        log.info("Found available version slot: {}", availableSlot);

        Slot updatedSlot = slotType.occupy(availableSlot);
        slots.set(0, updatedSlot);

        Versions updatedVersions = versions.toBuilder()
                .slots(slots)
                .build();

        return NextVersion.of(updatedVersions, updatedSlot.getVersion());
    }

    private static Slot slotWithVersion(Version version) {
        return Slot.builder()
                .version(version)
                .build();
    }
}
