package clusters

import (
	"fmt"
	"strings"

	"a.yandex-team.ru/library/go/core/xerrors"
)

type Status int

const (
	StatusUnknown Status = iota
	StatusCreating
	StatusCreateError
	StatusRunning
	StatusModifying
	StatusModifyError
	StatusStopping
	StatusStopped
	StatusStopError
	StatusStarting
	StatusStartError
	StatusDeleting
	StatusDeleteError
	StatusDeleted
	StatusPurging
	StatusPurgeError
	StatusPurged
	StatusMetadataDeleting
	StatusMetadataDeleteError
	StatusMetadataDeleted
	StatusRestoringOnline
	StatusRestoringOffline
	StatusRestoringOnlineError
	StatusRestoringOfflineError
	StatusMaintainingOffline
	StatusMaintainOfflineError
)

// ATTENTION: These names currently map to enum in metadb, do not change
var (
	mapStatusToString = map[Status]string{
		StatusUnknown:               "UNKNOWN",
		StatusCreating:              "CREATING",
		StatusCreateError:           "CREATE-ERROR",
		StatusRunning:               "RUNNING",
		StatusModifying:             "MODIFYING",
		StatusModifyError:           "MODIFY-ERROR",
		StatusStopping:              "STOPPING",
		StatusStopped:               "STOPPED",
		StatusStopError:             "STOP-ERROR",
		StatusStarting:              "STARTING",
		StatusStartError:            "START-ERROR",
		StatusDeleting:              "DELETING",
		StatusDeleteError:           "DELETE-ERROR",
		StatusDeleted:               "DELETED",
		StatusPurging:               "PURGING",
		StatusPurgeError:            "PURGE-ERROR",
		StatusPurged:                "PURGED",
		StatusMetadataDeleting:      "METADATA-DELETING",
		StatusMetadataDeleteError:   "METADATA-DELETE-ERROR",
		StatusMetadataDeleted:       "METADATA-DELETED",
		StatusRestoringOnline:       "RESTORING-ONLINE",
		StatusRestoringOffline:      "RESTORING-OFFLINE",
		StatusRestoringOnlineError:  "RESTORE-ONLINE-ERROR",
		StatusRestoringOfflineError: "RESTORE-OFFLINE-ERROR",
		StatusMaintainingOffline:    "MAINTAINING-OFFLINE",
		StatusMaintainOfflineError:  "MAINTAIN-OFFLINE-ERROR",
	}
	nameToStatusMapping = make(map[string]Status, len(mapStatusToString))
)

func init() {
	for status, str := range mapStatusToString {
		nameToStatusMapping[strings.ToLower(str)] = status
	}
}

func (s Status) String() string {
	str, ok := mapStatusToString[s]
	if !ok {
		return fmt.Sprintf("UNKNOWN_CLUSTER_STATUS_%d", s)
	}

	return str
}

func ParseStatus(str string) (Status, error) {
	// TODO: https://st.yandex-team.ru/MDB-10151
	s, ok := nameToStatusMapping[strings.ToLower(str)]
	if !ok {
		return StatusUnknown, xerrors.Errorf("unknown cluster status %q", str)
	}

	return s, nil
}
