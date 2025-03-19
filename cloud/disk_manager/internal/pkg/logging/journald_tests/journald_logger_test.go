package tests

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"os"
	"os/exec"
	"strconv"
	"testing"
	"time"

	"github.com/coreos/go-systemd/journal"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"

	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/headers"
	"a.yandex-team.ru/cloud/disk_manager/internal/pkg/logging"
)

////////////////////////////////////////////////////////////////////////////////

type journaldEntry struct {
	message        string
	priority       journal.Priority
	idempotencyKey string
	requestID      string
}

func parseJournaldEntry(data []byte) (journaldEntry, error) {
	entry := make(map[string]string)
	err := json.Unmarshal(data, &entry)
	if err != nil {
		return journaldEntry{}, err
	}

	message, ok := entry["MESSAGE"]
	if !ok {
		return journaldEntry{}, fmt.Errorf("No MESSAGE field")
	}

	priorityString, ok := entry["PRIORITY"]
	if !ok {
		return journaldEntry{}, fmt.Errorf("No PRIORITY field")
	}
	priority, err := strconv.Atoi(priorityString)
	if err != nil {
		return journaldEntry{}, fmt.Errorf("PRIORITY field is not an int %w", err)
	}

	idempotencyKey, ok := entry["IDEMPOTENCY_KEY"]
	if !ok {
		return journaldEntry{}, fmt.Errorf("No IDEMPOTENCY_KEY field")
	}

	requestID, ok := entry["REQUEST_ID"]
	if !ok {
		return journaldEntry{}, fmt.Errorf("No IDEMPOTENCY_KEY field")
	}

	return journaldEntry{
		message:        message,
		priority:       journal.Priority(priority),
		idempotencyKey: idempotencyKey,
		requestID:      requestID,
	}, nil
}

func parseJournaldEntries(data []byte) ([]journaldEntry, error) {
	lines := bytes.Split(data, []byte("\n"))
	entries := make([]journaldEntry, 0, len(lines))

	for i, line := range lines {
		if len(line) == 0 {
			continue
		}

		entry, err := parseJournaldEntry(line)
		if err != nil {
			return nil, fmt.Errorf(
				"Error parsing line #%v (%v): %w",
				i+1,
				string(line),
				err,
			)
		}

		entries = append(entries, entry)
	}

	return entries, nil
}

func runJournalctl() ([]byte, error) {
	// We want our messages to show up in journalctl, wait for them to sync.
	// journalctl --sync does not work here, because it requires special
	// access rights which are not expected on a developer's machine.
	<-time.After(time.Second)

	pid := os.Getpid()
	return exec.Command(
		"journalctl",
		"-o",
		"json",
		fmt.Sprintf("_PID=%v", pid),
		"--no-pager",
	).Output()
}

////////////////////////////////////////////////////////////////////////////////

func createContext(idempotencyKey string, requestID string) context.Context {
	return headers.SetIncomingRequestID(
		headers.SetIncomingIdempotencyKey(context.Background(), idempotencyKey),
		requestID,
	)
}

////////////////////////////////////////////////////////////////////////////////

func TestJournaldLogInfo(t *testing.T) {
	logger := logging.CreateJournaldLogger(logging.InfoLevel)
	ctx := logging.SetLogger(createContext("idempID", "reqID"), logger)

	logging.Trace(ctx, "Trace did not happen")
	logging.Debug(ctx, "Debug did not happen")
	logging.Info(ctx, "Info happened")
	logging.Warn(ctx, "Warn happened")
	logging.Error(ctx, "Error happened")
	logging.Fatal(ctx, "Fatal happened")

	data, err := runJournalctl()
	require.NoError(t, err)

	entries, err := parseJournaldEntries(data)
	require.NoError(t, err)

	assert.EqualValues(
		t,
		[]journaldEntry{
			journaldEntry{
				message:        "Info happened",
				priority:       journal.PriInfo,
				idempotencyKey: "idempID",
				requestID:      "reqID",
			},
			journaldEntry{
				message:        "Warn happened",
				priority:       journal.PriWarning,
				idempotencyKey: "idempID",
				requestID:      "reqID",
			},
			journaldEntry{
				message:        "Error happened",
				priority:       journal.PriErr,
				idempotencyKey: "idempID",
				requestID:      "reqID",
			},
			journaldEntry{
				message:        "Fatal happened",
				priority:       journal.PriCrit,
				idempotencyKey: "idempID",
				requestID:      "reqID",
			},
		},
		entries,
	)
}
