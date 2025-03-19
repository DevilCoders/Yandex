package grpc

import (
	mlock "a.yandex-team.ru/cloud/mdb/mlock/api"
	"a.yandex-team.ru/cloud/mdb/mlock/pkg/mlockclient"
)

func conflictFromGRPC(conflict *mlock.Conflict) mlockclient.Conflict {
	return mlockclient.Conflict{
		Object:  conflict.GetObject(),
		LockIDs: conflict.GetIds(),
	}
}

func conflictsFromConflictSlice(slice []*mlock.Conflict) []mlockclient.Conflict {
	result := make([]mlockclient.Conflict, len(slice))
	for index, conflict := range slice {
		result[index] = conflictFromGRPC(conflict)
	}
	return result
}
