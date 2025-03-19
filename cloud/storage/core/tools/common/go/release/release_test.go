package release

import (
	"bufio"
	"context"
	"errors"
	"fmt"
	"io"
	"io/ioutil"
	"log"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
	"gopkg.in/yaml.v2"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/messenger"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/z2"
	"a.yandex-team.ru/library/go/test/canon"
)

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

type jugglerClientMock struct {
	logutil.WithLog

	mutes          int
	downhosts      []string
	brokenRecently []string
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

	for _, h := range j.brokenRecently {
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
}

func (p *psshMock) Run(
	ctx context.Context,
	cmd string,
	target string,
) ([]string, error) {
	p.LogInfo(ctx, "[PSSH] Run: '%v' on host: %v", cmd, target)

	p.requestCount++

	if strings.Contains(target, "broken") {
		return nil, errors.New("broken host")
	}

	if p.requestCount%3 == 0 {
		return nil, errors.New("routine error")
	}

	lines := strings.Split(strings.ReplaceAll(cmd, " || ", " && "), "&&")

	var response []string

	for _, line := range lines {
		s := strings.TrimSpace(line)

		if strings.HasPrefix(s, "sudo service") && strings.HasSuffix(s, "restart") {
			continue
		}

		if strings.HasPrefix(s, "sleep ") {
			continue
		}

		if strings.HasPrefix(s, "sudo systemctl show ") {
			response = append(response, "ActiveState=active")

			if strings.Contains(s, "ActiveEnter") {
				response = append(response, "ActiveEnterTimestampMonotonic=4151879060634")

			} else {
				response = append(response, "RestartsCount=1")
			}

			continue
		}

		if strings.Contains(s, "CurrentTimestamp") {
			response = append(response, "CurrentTimestampMonotonic=4151884103863")

			continue
		}

		if strings.HasPrefix(s, "sudo rm -f ") {
			continue
		}

		if strings.HasPrefix(s, "mkdir -p") {
			continue
		}

		if strings.HasPrefix(s, "chmod ") {
			continue
		}

		if strings.HasPrefix(s, "echo ") {
			// skip 'echo text > file'

			if !strings.Contains(s, " > ") {
				response = append(response, strings.TrimPrefix(s, "echo "))
			}

			continue
		}

		if strings.HasPrefix(s, "{ sudo nohup ") && strings.HasSuffix(s, " & }") {
			continue
		}

		if strings.HasPrefix(s, "wait ") {
			continue
		}

		if s == "cat /tmp/yc-nbs-restart.out" {
			response = append(
				response,
				"04/22/22 11:07:52.171283 === Check IAM-token ===",
				"04/22/22 11:07:52.449817 === Load blockstore-server to the page cache ===",
				"04/22/22 11:07:54.133209 === Configure volume balancer ===",
				"{\"OpStatus\":\"DISABLE\"}",
				"04/22/22 11:07:54.438360 === Rebind volumes ===",
				"{}",
				"04/22/22 11:07:59.669333 === Run NBS2 ===",
				"PID has been saved to /tmp/nbs2.pid.2242b92e-ebdf-4d59-be56-48f5e3632504",
				"04/22/22 11:07:59.764905 === Ping NBS2 ===",
				"OK",
				"04/22/22 11:07:59.853933 === Wait for NBS2 endpoints ===",
				"04/22/22 11:08:00.938183 === Restart NBS ===",
				"ActiveState=active",
				"RestartsCount=1",
				"04/22/22 11:08:01.611780 === Ping NBS ===",
				"OK",
				"04/22/22 11:08:01.701145 === Wait for NBS endpoints ===",
				"04/22/22 11:08:01.812606 === Kill NBS2 (730493) ===",
				"04/22/22 11:08:01.814375 === Restart Completed ===",
			)

			continue
		}

		response = append(response, "unknown command")
	}

	return response, nil
}

func (p *psshMock) CopyFile(
	ctx context.Context,
	source string,
	target string,
) error {
	panic("not implemented")
}

////////////////////////////////////////////////////////////////////////////////

type starTrackClientMock struct {
	logutil.WithLog
}

func (st *starTrackClientMock) Comment(
	ctx context.Context,
	text string,
) error {
	st.LogInfo(ctx, "[ST] Comment: %v", text)

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type telegramClientMock struct {
	logutil.WithLog
}

func (tg *telegramClientMock) SendMessage(
	ctx context.Context,
	chatID string,
	text string,
) error {
	tg.LogInfo(ctx, "[TELEGRAM] SendMessage: %v %v", chatID, text)

	return nil
}

func (tg *telegramClientMock) GetMessengerType() messenger.MessengerType {
	return messenger.Telegram
}

func (tg *telegramClientMock) GetUserChatID(username string) string {
	return "xyz"
}

func (tg *telegramClientMock) GetUsersFile() string {
	return ""
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

type z2ClientMock struct {
	logutil.WithLog

	workers      map[string][]string
	requestCount int
}

func (c *z2ClientMock) EditItems(
	ctx context.Context,
	configID string,
	items []z2.Z2Item,
) error {
	c.LogInfo(ctx, "[Z2] EditItems: %v %v", configID, items)

	c.requestCount++

	if c.requestCount%3 == 0 {
		return errors.New("routine error")
	}

	return nil
}

func (c *z2ClientMock) Update(
	ctx context.Context,
	configID string,
	forceYes bool,
) error {

	c.LogInfo(ctx, "[Z2] Update: %v force: %t", configID, forceYes)

	c.requestCount++

	if c.requestCount%3 == 0 {
		return errors.New("routine error")
	}

	return nil
}

func (c *z2ClientMock) UpdateStatus(
	ctx context.Context,
	configID string,
) (*z2.Z2UpdateStatus, error) {
	c.LogInfo(ctx, "[Z2] UpdateStatus: %v", configID)

	c.requestCount++

	if c.requestCount%3 == 0 {
		return nil, errors.New("routine error")
	}

	return &z2.Z2UpdateStatus{
		Status: "FINISHED", // IN_PROGRESS
		Result: "SUCCESS",
	}, nil
}

func (c *z2ClientMock) ListWorkers(
	ctx context.Context,
	configID string,
) ([]string, error) {
	c.LogInfo(ctx, "[Z2] ListWorkers: %v", configID)

	c.requestCount++

	if c.requestCount%3 == 0 {
		return nil, errors.New("routine error")
	}

	return c.workers[configID], nil
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

func RunTestRelease(t *testing.T, checkBackwardCompatibility bool) {
	opts := &Options{
		Force:                true,
		NoRestart:            false,
		SkipInfraAndMutes:    false,
		UpdateMetaGroups:     false,
		Yes:                  true,
		ClusterName:          "test-cluster",
		ZoneName:             "test-zone",
		TargetName:           "",
		ConfigVersion:        "3.2.1",
		CommonPackageVersion: "stable-1.2.3",
		PackageVersions: PackageVersions{
			"yandex-cloud-blockstore-plugin": "stable-0.0.1",
		},
		Description:        "stable-1.2.3/3.2.1",
		Ticket:             "NBSOPS-TEST",
		MaxRestartProblems: 2,
		Parallelism:        1,
		PsshAttempts:       5,
		Z2Attempts:         5,
		Z2Backoff:          time.Second,
		RackRestartDelay:   time.Millisecond,
	}
	if checkBackwardCompatibility {
		opts.PrevConfigVersion = "3.2.0"
		opts.PrevCommonPackageVersion = "stable-1.2.0"
		opts.PrevPackageVersions = PackageVersions{
			"yandex-cloud-blockstore-plugin": "stable-0.0.0",
		}
	}

	config := &ServiceConfig{
		Name:        "blockstore",
		Description: "NBS",
		Clusters: map[string]ClusterConfig{
			"test-cluster": map[string]ZoneConfig{
				"test-zone": ZoneConfig{
					Telegram: &TelegramConfig{
						ChatID: "test-chat-id",
					},
					Qmssngr: &QmssngrConfig{
						ChatID: "test-chat-id",
					},
					Infra: &InfraConfig{
						ServiceID:     322,
						EnvironmentID: 42,
					},
					Juggler: &JugglerConfig{
						Host: "test_juggler_host",
						Alerts: []string{
							"solomon_alert_nbs_server_restarts",
							"solomon_alert_nbs_server_errors",
							"solomon_alert_nbs_server_version",
						},
						Namespace: "test_namespace",
					},
					Targets: []*TargetConfig{
						&TargetConfig{
							Name: "nbs",
							BinaryPackages: []string{
								"yandex-cloud-blockstore-server",
								"yandex-cloud-blockstore-client",
								"yandex-cloud-blockstore-plugin",
							},
							ConfigPackage: "test-config-package",
							Services: []UnitConfig{
								UnitConfig{"nbs", Duration(5 * time.Second), "nbs.restart"},
							},
							Z2: &Z2Config{
								Group:           "TEST_GROUP",
								MetaGroup:       "TEST_META_GROUP",
								CanaryGroup:     "TEST_CANARY_GROUP",
								ConfigMetaGroup: "TEST_CONFIG_META_GROUP",
							},
						},
						&TargetConfig{
							Name:           "nbs-control",
							Tags:           []string{"svm", "check-backward-compatibility"},
							MaxParallelism: 1,
							BinaryPackages: []string{
								"yandex-cloud-blockstore-server",
							},
							ConfigPackage: "test-control-config-package",
							Services: []UnitConfig{
								UnitConfig{"nbs", Duration(5 * time.Second), "nbs.restart"},
								UnitConfig{"nginx", Duration(time.Second), ""},
							},
							Z2: &Z2Config{
								Group:           "TEST_GROUP_CONTROL",
								MetaGroup:       "TEST_META_GROUP_CONTROL",
								ConfigMetaGroup: "TEST_CONFIG_META_GROUP_CONTROL",
							},
						},
					},
				},
			},
		},
	}

	tmpDir, err := ioutil.TempDir(os.TempDir(), "canon_")
	require.NoError(t, err)
	defer func() { _ = os.RemoveAll(tmpDir) }()

	canonFileName := func(t *testing.T) string {
		testName := strings.Replace(t.Name(), "/", ".", -1)
		return filepath.Join(tmpDir, testName)
	}

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
		//nbs.LOG_DEBUG,
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

	svms := []string{
		"svm-1",
		"svm-2",
		"svm-3",
	}

	hostToLocation := map[string]string{
		"host-1":             "foo",
		"host-2":             "foo",
		"host-3":             "foo",
		"host-4":             "foo",
		"broken-after-start": "foo",
		"broken":             "bar",
		"host-5":             "bar",
		"host-6":             "bar",
	}

	var workers []string
	for host := range hostToLocation {
		workers = append(workers, host)
	}
	sort.Strings(workers)

	r := NewRelease(
		logger,
		opts,
		config,
		&infraClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&jugglerClientMock{
			WithLog: logutil.WithLog{Log: logger},
			downhosts: []string{
				"broken",
			},
			brokenRecently: []string{
				"broken-after-start",
			},
		},
		pssh.NewDurable(
			logger,
			&psshMock{
				WithLog: logutil.WithLog{Log: psshLogger},
			},
			opts.PsshAttempts,
		),
		&starTrackClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&telegramClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&walleClientMock{
			WithLog:        logutil.WithLog{Log: logger},
			hostToLocation: hostToLocation,
		},
		&z2ClientMock{
			WithLog: logutil.WithLog{Log: logger},
			workers: map[string][]string{
				"TEST_GROUP":                     workers,
				"TEST_CONFIG_META_GROUP":         workers,
				"TEST_CANARY_GROUP":              workers[3:5],
				"TEST_GROUP_CONTROL":             svms,
				"TEST_META_GROUP_CONTROL":        svms,
				"TEST_CONFIG_META_GROUP_CONTROL": svms,
			},
		},
	)

	err = r.Run(context.TODO())
	require.NoError(t, err)

	err = psshLog.dump(logWriter)
	require.NoError(t, err)

	_ = logWriter.Flush()
	canon.SaveFile(t, canonFileName(t), canon.WithLocal(false))
}

////////////////////////////////////////////////////////////////////////////////

func TestRelease(t *testing.T) {
	RunTestRelease(t, false)
}

func TestReleaseWithBackwardCompatibility(t *testing.T) {
	RunTestRelease(t, true)
}

func TestHostsFilter(t *testing.T) {
	opts := &Options{
		Force:             true,
		NoRestart:         false,
		SkipInfraAndMutes: false,
		UpdateMetaGroups:  false,
		Yes:               true,
		ClusterName:       "test-cluster",
		ZoneName:          "test-zone",
		TargetName:        "",
		HostsFilter: map[string]bool{
			"host-1":     true,
			"host-2":     true,
			"host-3":     true,
			"unexisting": true,
		},
		Description:        "restart hosts",
		Ticket:             "NBSOPS-TEST",
		MaxRestartProblems: 2,
		Parallelism:        1,
		PsshAttempts:       5,
		Z2Attempts:         5,
		Z2Backoff:          time.Second,
		RackRestartDelay:   time.Millisecond,
	}
	config := &ServiceConfig{
		Name:        "blockstore",
		Description: "NBS",
		Clusters: map[string]ClusterConfig{
			"test-cluster": map[string]ZoneConfig{
				"test-zone": ZoneConfig{
					Telegram: &TelegramConfig{
						ChatID: "test-chat-id",
					},
					Qmssngr: &QmssngrConfig{
						ChatID: "test-chat-id",
					},
					Infra: &InfraConfig{
						ServiceID:     322,
						EnvironmentID: 42,
					},
					Juggler: &JugglerConfig{
						Host: "test_juggler_host",
						Alerts: []string{
							"solomon_alert_nbs_server_restarts",
							"solomon_alert_nbs_server_errors",
							"solomon_alert_nbs_server_version",
						},
						Namespace: "test_namespace",
					},
					Targets: []*TargetConfig{
						&TargetConfig{
							Name: "nbs",
							BinaryPackages: []string{
								"yandex-cloud-blockstore-server",
								"yandex-cloud-blockstore-client",
								"yandex-cloud-blockstore-plugin",
							},
							ConfigPackage: "test-config-package",
							Services: []UnitConfig{
								UnitConfig{"nbs", Duration(5 * time.Second), "nbs.restart"},
							},
							Z2: &Z2Config{
								Group:           "TEST_GROUP",
								MetaGroup:       "TEST_META_GROUP",
								CanaryGroup:     "TEST_CANARY_GROUP",
								ConfigMetaGroup: "TEST_CONFIG_META_GROUP",
							},
						},
						&TargetConfig{
							Name:           "nbs-control",
							Tags:           []string{"svm"},
							MaxParallelism: 1,
							BinaryPackages: []string{
								"yandex-cloud-blockstore-server",
							},
							ConfigPackage: "test-control-config-package",
							Services: []UnitConfig{
								UnitConfig{"nbs", Duration(5 * time.Second), "nbs.restart"},
								UnitConfig{"nginx", Duration(time.Second), ""},
							},
							Z2: &Z2Config{
								Group:           "TEST_GROUP_CONTROL",
								MetaGroup:       "TEST_META_GROUP_CONTROL",
								ConfigMetaGroup: "TEST_CONFIG_META_GROUP_CONTROL",
							},
						},
					},
				},
			},
		},
	}

	tmpDir, err := ioutil.TempDir(os.TempDir(), "canon_")
	require.NoError(t, err)
	defer func() { _ = os.RemoveAll(tmpDir) }()

	canonFileName := func(t *testing.T) string {
		testName := strings.Replace(t.Name(), "/", ".", -1)
		return filepath.Join(tmpDir, testName)
	}

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
		nbs.LOG_DEBUG,
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

	svms := []string{
		"svm-1",
		"svm-2",
		"svm-3",
	}

	hostToLocation := map[string]string{
		"host-1":             "foo",
		"host-2":             "foo",
		"host-3":             "foo",
		"host-4":             "foo",
		"broken":             "bar",
		"broken-after-start": "bar",
		"host-5":             "bar",
		"host-6":             "bar",
	}

	var workers []string
	for host := range hostToLocation {
		workers = append(workers, host)
	}
	sort.Strings(workers)

	r := NewRelease(
		logger,
		opts,
		config,
		&infraClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&jugglerClientMock{
			WithLog: logutil.WithLog{Log: logger},
			downhosts: []string{
				"broken",
			},
			brokenRecently: []string{
				"broken-after-start",
			},
		},
		pssh.NewDurable(
			logger,
			&psshMock{
				WithLog: logutil.WithLog{Log: psshLogger},
			},
			opts.PsshAttempts,
		),
		&starTrackClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&telegramClientMock{
			WithLog: logutil.WithLog{Log: logger},
		},
		&walleClientMock{
			WithLog:        logutil.WithLog{Log: logger},
			hostToLocation: hostToLocation,
		},
		&z2ClientMock{
			WithLog: logutil.WithLog{Log: logger},
			workers: map[string][]string{
				"TEST_GROUP":                     workers,
				"TEST_CONFIG_META_GROUP":         workers,
				"TEST_CANARY_GROUP":              workers[3:5],
				"TEST_GROUP_CONTROL":             svms,
				"TEST_META_GROUP_CONTROL":        svms,
				"TEST_CONFIG_META_GROUP_CONTROL": svms,
			},
		},
	)

	err = r.Run(context.TODO())
	require.NoError(t, err)

	err = psshLog.dump(logWriter)
	require.NoError(t, err)

	_ = logWriter.Flush()
	canon.SaveFile(t, canonFileName(t), canon.WithLocal(false))
}

func TestUnmarshalDuration(t *testing.T) {
	configYAML := []byte("restart_delay: 2m")

	config := &UnitConfig{}

	err := yaml.Unmarshal(configYAML, config)
	require.NoError(t, err)

	assert.Equal(t, Duration(2*time.Minute), config.RestartDelay)
}

// TODO
// func TestBadRelease(t *testing.T) { ... }
