#include "cbb_list_watcher.h"


namespace NAntiRobot {


void TCbbListWatcher::Poll(ICbbIO* cbb) {
    if (!Future.HasValue() && !Future.HasException()) {
        return;
    }

    TVector<TCbbGroupId> ids;

    for (const auto* manager : Managers) {
        const auto managerIds = manager->GetIds();
        ids.insert(ids.end(), managerIds.begin(), managerIds.end());
    }

    Future = cbb->CheckFlags(ids).Apply([this, cbb, ids = std::move(ids)] (const auto& future) {
        const auto& timestamps = future.GetValue();

        TVector<std::pair<TCbbGroupId, TInstant>> idToTimestamp;
        idToTimestamp.reserve(timestamps.size());

        for (const auto& [id, timestamp] : Zip(ids, timestamps)) {
            idToTimestamp.push_back({id, timestamp});
        }

        for (auto* manager : Managers) {
            manager->Update(cbb, idToTimestamp);
        }
    });
}


} // namespace NAntiRobot
