package main

import (
	"bufio"
	"context"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path"
	"path/filepath"
	"sort"
	"strconv"
	"strings"
	"testing"
	"time"

	"a.yandex-team.ru/library/go/test/canon"
	"github.com/stretchr/testify/require"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/release"

	"github.com/golang/protobuf/proto"
)

////////////////////////////////////////////////////////////////////////////////

func normalizeTempFileName(fileName string) string {
	// /foo/bar/temp/ololo/cms-foo-bar-baz-XXX -> cms-foo-bar-baz
	baseName := path.Base(fileName)
	i := strings.LastIndex(baseName, "-")
	if i != -1 {
		return baseName[0:i]
	}
	return baseName
}

////////////////////////////////////////////////////////////////////////////////

type psshLogWriter struct {
	content []string
}

func (l *psshLogWriter) Write(p []byte) (
	n int,
	err error,
) {
	l.content = append(l.content, string(p))
	return len(p), nil
}

func (l *psshLogWriter) dump(w io.Writer) error {
	sort.Strings(l.content)

	if _, err := fmt.Fprintln(w, "======= PSSH ======="); err != nil {
		return err
	}

	for _, line := range l.content {
		_, err := fmt.Fprint(w, line)
		if err != nil {
			return err
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type cmsClientMock struct {
	logutil.WithLog
}

////////////////////////////////////////////////////////////////////////////////

type jugglerClientMock struct {
	logutil.WithLog

	mutes     int
	downhosts []string
}

func (j *jugglerClientMock) SetMutes(
	ctx context.Context,
	service string,
	host string,
	startTime time.Time,
	duration time.Duration,
	namespace string,
	project string,
) (juggler.MuteID, error) {
	j.LogInfo(ctx, "[JUGGLER] SetMutes: %v %v %v %v", service, host, duration, namespace)

	id := fmt.Sprintf("mute-id-%v", j.mutes)

	j.mutes += 1

	return id, nil
}

func (j *jugglerClientMock) GetDownHosts(ctx context.Context) (
	[]string,
	error,
) {
	j.LogInfo(ctx, "[JUGGLER] GetDownHosts")

	return j.downhosts, nil
}

func (j *jugglerClientMock) HasDowntime(ctx context.Context, host string) (
	bool,
	error,
) {
	j.LogInfo(ctx, "[JUGGLER] HasDowntime")

	for _, h := range j.downhosts {
		if h == host {
			return true, nil
		}
	}

	return false, nil
}

////////////////////////////////////////////////////////////////////////////////

type psshMock struct {
	logutil.WithLog

	requestCount int
	limitNRD     uint64
}

func (pssh *psshMock) normalizeCmd(cmd string) string {
	const update = "blockstore-client updatediskregistryconfig --input "

	i := strings.Index(cmd, update)
	if i != -1 {
		i += len(update)
		j := strings.Index(cmd[i:], " ")
		if j != -1 {
			cmd = strings.Replace(cmd, cmd[i:i+j], normalizeTempFileName(cmd[i:i+j]), 1)
		}

		return cmd
	}

	const kikimr = "kikimr admin console execute "

	i = strings.Index(cmd, kikimr)
	if i != -1 {
		reqPath := cmd[i+len(kikimr):]
		return kikimr + normalizeTempFileName(reqPath)
	}

	return cmd
}

func (pssh *psshMock) Run(
	ctx context.Context,
	cmd string,
	target string,
) ([]string, error) {
	pssh.LogInfo(ctx, "[PSSH] Run: '%v' on host: %v", pssh.normalizeCmd(cmd), target)

	pssh.requestCount++

	if strings.Contains(target, "broken") {
		return nil, errors.New("broken host")
	}

	/*if pssh.requestCount%3 == 0 {
		return nil, errors.New("routine error")
	}*/

	lines := strings.Split(cmd, "&&")

	var response []string

	for _, line := range lines {
		s := strings.TrimSpace(line)

		if strings.HasPrefix(s, "echo ") && !strings.Contains(s, "Timestamp") {
			// skip 'echo text > file', but not 'echo CurrentTimestampMonotonic ...'

			if !strings.Contains(s, " > ") {
				response = append(response, strings.TrimPrefix(s, "echo "))
			}

			continue
		}

		if strings.HasPrefix(s, "blockstore-client describediskregistryconfig") {
			response = append(
				response,
				"Version: 42",
			)

			// ...

			continue
		}

		if strings.HasPrefix(s, "blockstore-client updatediskregistryconfig") {
			continue
		}

		if strings.HasPrefix(s, "kikimr admin console execute") {
			if strings.Contains(s, " cms-get-items-") {
				response = append(
					response,
					`
					GetConfigItemsResponse {
						Status {
							Code: SUCCESS
						}
					}
					Status {
						Code: SUCCESS
					}
					`,
				)

				continue
			}

			if strings.Contains(s, " cms-update-item-") {
				response = append(response,
					`ConfigureResponse {
						Status {
							Code: SUCCESS
						}
						AddedItemIds: 42
					}
					Status {
						Code: SUCCESS
					}`,
				)

				continue
			}

			response = append(response, "<<<<unknown CMS request>>>>")

			continue
		}

		if strings.HasPrefix(s, "kikimr db schema user-attribute get") {
			response = append(
				response,
				fmt.Sprintf("__volume_space_limit_ssd_nonrepl: %v", pssh.limitNRD),
			)
			continue
		}

		if strings.HasPrefix(s, "kikimr db schema user-attribute set ") {
			parts := strings.Split(s, " ")
			parts = strings.SplitN(parts[len(parts)-1], "=", 2)
			if parts[0] == "__volume_space_limit_ssd_nonrepl" {
				var err error
				pssh.limitNRD, err = strconv.ParseUint(parts[1], 10, 64)
				if err != nil {
					response = append(
						response,
						fmt.Sprintf("can't parse '%v': %v", parts[1], err),
					)
					/*} else {
					response = append(response, "")*/
				}
			} else {
				response = append(
					response,
					fmt.Sprintf(
						"TX-0052 UserAttributes: unsupported attribute '%v'",
						parts[0],
					),
				)
			}

			continue
		}

		if strings.HasPrefix(s, "sudo systemctl show ") {
			response = append(
				response,
				"ActiveState=active",
				"ActiveEnterTimestampMonotonic=4157897297894",
			)

			continue
		}

		if strings.Contains(s, "CurrentTimestamp") {
			response = append(response, "CurrentTimestampMonotonic=4157902329869")

			continue
		}

		if strings.Contains(s, "blockstore-client ping") {
			response = append(response, "OK")

			continue
		}

		if s == "sudo cat /usr/bin/blockstore-server > /dev/null" {
			continue
		}

		if strings.HasPrefix(s, "sudo service") && strings.HasSuffix(s, "restart") {
			continue
		}

		if strings.HasPrefix(s, "sleep ") {
			continue
		}

		if strings.HasPrefix(s, "mkdir -p") {
			continue
		}

		if strings.HasPrefix(s, "test ") {
			continue
		}

		if strings.HasPrefix(s, "chmod ") {
			continue
		}

		response = append(response, "unknown command: "+s)
	}

	return response, nil
}

func (pssh *psshMock) CopyFile(
	ctx context.Context,
	source string,
	target string,
) error {
	source = normalizeTempFileName(source)

	pssh.LogInfo(ctx, "[PSSH] CopyFile: '%v' to %v", source, target)

	pssh.requestCount++

	if strings.Contains(target, "broken") {
		return errors.New("broken host")
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type infraClientMock struct {
	logutil.WithLog

	lastEventID int
}

func (i *infraClientMock) CreateEvent(
	ctx context.Context,
	serviceID int,
	environmentID int,
	title string,
	startTime time.Time,
	duration time.Duration,
) (infra.EventID, error) {
	i.LogInfo(ctx, "[INFRA] CreateEvent: %v %v %v", environmentID, title, duration)

	i.lastEventID += 1

	return i.lastEventID, nil
}

func (i *infraClientMock) ExtendEvent(
	ctx context.Context,
	eventID infra.EventID,
	finishTime time.Time,
) error {
	return nil
}

////////////////////////////////////////////////////////////////////////////////

type walleClientMock struct {
	logutil.WithLog

	hostToLocation map[string]string
}

func (w *walleClientMock) SplitByLocation(
	ctx context.Context,
	hosts []string,
) (map[string][]string, error) {
	w.LogInfo(ctx, "[WALLE] SplitByLocation: %v", hosts)

	r := make(map[string][]string)

	for _, host := range hosts {
		loc := w.hostToLocation[host]
		r[loc] = append(r[loc], host)
	}

	for loc := range r {
		sort.Strings(r[loc])
	}

	return r, nil
}

////////////////////////////////////////////////////////////////////////////////

func TestAdd(t *testing.T) {
	canonDir, err := ioutil.TempDir(os.TempDir(), "canon_")
	require.NoError(t, err)
	defer func() { _ = os.RemoveAll(canonDir) }()

	canonFileName := func(t *testing.T) string {
		testName := strings.Replace(t.Name(), "/", ".", -1)
		return filepath.Join(canonDir, testName)
	}

	tmpDir, err := ioutil.TempDir("", "add_agents_")
	require.NoError(t, err)

	logFile, err := os.Create(canonFileName(t))
	require.NoError(t, err)
	defer func() { _ = logFile.Close() }()

	logWriter := bufio.NewWriter(logFile)

	logger := nbs.NewLog(
		log.New(
			io.MultiWriter(logWriter, os.Stdout),
			"",
			0,
		),
		nbs.LOG_INFO,
	)

	psshLog := &psshLogWriter{}

	psshLogger := nbs.NewLog(
		log.New(
			io.MultiWriter(psshLog, os.Stdout),
			"",
			0,
		),
		nbs.LOG_DEBUG,
	)

	pssh := &psshMock{
		WithLog: logutil.WithLog{Log: psshLogger},
	}

	zone := ZoneConfig{
		Infra: &release.InfraConfig{
			ServiceID:     322,
			EnvironmentID: 42,
		},
		Juggler: &release.JugglerConfig{
			Host: "solomon-alert-cloud_testing_nbs_sas",
			Alerts: []string{
				"solomon_alert_nbs_server_restarts",
				"solomon_alert_nbs_server_errors",
			},
			Namespace: "test_namespace",
		},
		SchemaShardDir: "/testing_sas/NBS",
		ConfigHost:     "C@cloud_testing_nbs-control_sas[0]",
		CMSHost:        "C@cloud_testing_compute_sas_az[0]",
		KikimrHost:     "C@cloud_testing_compute_sas_az[0]",
		AllocationUnit: 93 * 1024 * 1024 * 1024,
		LimitFraction:  0.85,
	}

	opts := Options{
		Yes:              true,
		DryRun:           false,
		NoRestart:        false,
		NoUpdateLimit:    false,
		SkipMutes:        false,
		EnableRDMA:       true,
		Parallelism:      1,
		ClusterName:      "test-cluster",
		ZoneName:         "test-zone",
		UserName:         "vasya",
		TempDir:          tmpDir,
		Ticket:           "NBSOPS-100500",
		Cookie:           "test-add",
		IAMToken:         "iam-token",
		RestartDelay:     5 * time.Second,
		RackRestartDelay: time.Millisecond,
		DeviceCount:      15,
	}

	hostToLocation := map[string]string{
		"host-1": "foo",
		"host-2": "foo",
		"host-3": "foo",
		"host-4": "foo",
		//"broken": "bar", // TODO
		"host-5": "bar",
		"host-6": "bar",
	}

	var newHosts []string
	for host := range hostToLocation {
		newHosts = append(newHosts, host)
	}
	sort.Strings(newHosts)

	dr, agents, err := addAgents(
		context.TODO(),
		logger,
		opts,
		&ServiceConfig{
			Clusters: map[string]ClusterConfig{
				"test-cluster": map[string]ZoneConfig{
					"test-zone": zone,
				},
			},
		},
		newHosts,
		&infraClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&jugglerClientMock{
			WithLog: logutil.WithLog{Log: logger},
			downhosts: []string{
				"broken",
			},
		},
		&walleClientMock{
			WithLog:        logutil.WithLog{Log: logger},
			hostToLocation: hostToLocation,
		},
		pssh,
		nil, // st
	)
	require.NoError(t, err)

	_, err = fmt.Fprintln(logWriter, "======= DR =======")
	require.NoError(t, err)
	_, err = fmt.Fprintln(logWriter, proto.MarshalTextString(dr))
	require.NoError(t, err)

	_, err = fmt.Fprintln(logWriter, "======= DAs =======")
	require.NoError(t, err)
	for _, agent := range agents {
		_, err = fmt.Fprintln(logWriter, proto.MarshalTextString(agent))
		require.NoError(t, err)
	}

	err = psshLog.dump(logWriter)
	require.NoError(t, err)

	_ = logWriter.Flush()
	canon.SaveFile(t, canonFileName(t), canon.WithLocal(false))
}
