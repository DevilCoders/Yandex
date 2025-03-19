package events_test

import (
	"testing"
	"time"

	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/mdb/internal/metadb"
	"a.yandex-team.ru/cloud/mdb/mdb-event-producer/internal/events"
)

func timeFromRFC3339Must(value string) time.Time {
	t, err := time.Parse(time.RFC3339, value)
	if err != nil {
		panic("Bad time value: " + err.Error())
	}
	return t
}

func TestFormatEvent(t *testing.T) {
	t.Run("return right event.ID", func(t *testing.T) {
		got, err := events.FormatEvent(metadb.WorkerQueueEvent{
			ID: 42,
			Data: `{
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1"
				}	
			}`,
			CreatedAt: time.Now(),
		}, events.StartStatus)
		require.NoError(t, err)
		require.Equal(t, int64(42), got.ID)
	})
	t.Run("with full event json", func(t *testing.T) {
		got, err := events.FormatEvent(metadb.WorkerQueueEvent{
			Data: `{
				"authentication": {
					"authenticated": true,
					"subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
					"subject_id": "142"
				},
				"authorization": {
					"authorized": true,
					"permissions": [{
						"permission": "get",
						"resource_type": "cluster",
						"resource_id": "cid1"
					}]
				},
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1"
				},
				"request_metadata": {
					"remote_address": "8.8.8.8",
					"user_agent": "Spider"
				},
				"details": {
					"cluster_id": "cid1"
				}
			}`,
			CreatedAt: timeFromRFC3339Must("2019-09-12T13:40:28Z"),
		}, events.StartStatus)
		require.NoError(t, err)
		require.JSONEq(
			t,
			`{
				"authentication": {
					"authenticated": true,
					"subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
					"subject_id": "142"
				},
				"authorization": {
					"authorized": true,
					"permissions": [{
						"permission": "get",
						"resource_type": "cluster",
						"resource_id": "cid1"
					}]
				},
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1",
					"created_at": "2019-09-12T13:40:28Z"
				},
				"event_status": "STARTED",
				"request_metadata": {
					"remote_address": "8.8.8.8",
					"user_agent": "Spider"
				},
				"details": {
					"cluster_id": "cid1"
				}
			}`,
			string(got.Data),
		)
	})
	t.Run("with event that already have status and create_at", func(t *testing.T) {
		got, err := events.FormatEvent(metadb.WorkerQueueEvent{
			Data: `{
				"authentication": {
					"authenticated": true,
					"subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
					"subject_id": "142"
				},
				"authorization": {
					"authorized": true,
					"permissions": [{
						"permission": "get",
						"resource_type": "cluster",
						"resource_id": "cid1"
					}]
				},
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1",
					"created_at": "1800-00-10T15:40:28+07:00"
				},
				"event_status": "EVENT_STATUS_UNSPECIFIED",
				"request_metadata": {
					"remote_address": "8.8.8.8",
					"user_agent": "Spider"
				},
				"details": {
					"cluster_id": "cid1"
				}
			}`,
			CreatedAt: timeFromRFC3339Must("2019-09-12T13:40:28Z"),
		}, events.DoneStatus)
		require.NoError(t, err)
		require.JSONEq(
			t,
			`{
				"authentication": {
					"authenticated": true,
					"subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
					"subject_id": "142"
				},
				"authorization": {
					"authorized": true,
					"permissions": [{
						"permission": "get",
						"resource_type": "cluster",
						"resource_id": "cid1"
					}]
				},
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1",
					"created_at": "2019-09-12T13:40:28Z"
				},
				"event_status": "DONE",
				"request_metadata": {
					"remote_address": "8.8.8.8",
					"user_agent": "Spider"
				},
				"details": {
					"cluster_id": "cid1"
				}
			}`,
			string(got.Data),
		)
	})
	t.Run("encode created at in UTC", func(t *testing.T) {
		got, err := events.FormatEvent(metadb.WorkerQueueEvent{
			Data: `{
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1"
				}
			}`,
			CreatedAt: timeFromRFC3339Must("2019-09-12T13:40:28+03:40"),
		}, events.DoneStatus)
		require.NoError(t, err)
		require.JSONEq(
			t,
			`{
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1",
					"created_at": "2019-09-12T10:00:28Z"
				},
				"event_status": "DONE"
			}`,
			string(got.Data),
		)
	})
	t.Run("for invalid status return error", func(t *testing.T) {
		_, err := events.FormatEvent(metadb.WorkerQueueEvent{}, "BLAH")
		require.Error(t, err)
	})
	t.Run("for bad json return error", func(t *testing.T) {
		_, err := events.FormatEvent(metadb.WorkerQueueEvent{
			Data: "<>",
		}, events.DoneStatus)
		require.Error(t, err)
	})
	t.Run("for event without event_metadata node return error", func(t *testing.T) {
		_, err := events.FormatEvent(metadb.WorkerQueueEvent{
			Data: `{
				"authentication": {
					"authenticated": true,
					"subject_type": "YANDEX_PASSPORT_USER_ACCOUNT",
					"subject_id": "142"
				},
				"authorization": {
					"authorized": true,
					"permissions": [{
						"permission": "get",
						"resource_type": "cluster",
						"resource_id": "cid1"
					}]
				},
				"request_metadata": {
					"remote_address": "8.8.8.8",
					"user_agent": "Spider"
				},
				"details": {
					"cluster_id": "cid1"
				}
			}`,
			CreatedAt: timeFromRFC3339Must("2019-09-12T13:40:28+03:00"),
		}, events.DoneStatus)
		require.Error(t, err)
	})
}

func TestFormatEvents(t *testing.T) {
	got, err := events.FormatEvents([]metadb.WorkerQueueEvent{{
		ID: 42,
		Data: `{
				"event_metadata": {
					"event_id": "event1",
					"event_type": "yandex.cloud.events.mdb.postgresql.CreateCluster",
					"cloud_id": "cloud1",
					"folder_id": "folder1"
				}	
			}`,
		CreatedAt: time.Now(),
	}}, events.StartStatus)
	require.NoError(t, err)
	require.Len(t, got, 1)
}
