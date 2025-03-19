package release

import (
	"bufio"
	"context"
	_ "embed"
	"errors"
	"fmt"
	"math/rand"
	"os"
	"sort"
	"strconv"
	"strings"
	"text/template"
	"time"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	logutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/log"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/messenger"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"
	st "a.yandex-team.ru/cloud/storage/core/tools/common/go/startrack"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/walle"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/z2"
)

////////////////////////////////////////////////////////////////////////////////

//go:embed restart.template
var psshTemplate string

////////////////////////////////////////////////////////////////////////////////

type PackageVersions map[string]string
type HostsFilter map[string]bool

type Options struct {
	Force             bool
	NoRestart         bool
	SkipInfraAndMutes bool
	UpdateMetaGroups  bool
	Yes               bool

	ClusterName string
	ZoneName    string
	TargetName  string

	HostsFilter HostsFilter

	ConfigVersion            string
	CommonPackageVersion     string
	PackageVersions          PackageVersions // { name: version, ... }
	PrevConfigVersion        string
	PrevCommonPackageVersion string
	PrevPackageVersions      PackageVersions // { name: version, ... }

	Description string
	Ticket      string

	MaxRestartProblems int
	Parallelism        int
	PsshAttempts       int           // TODO: move to pssh client params
	Z2Attempts         int           // TODO: move to z2 client params
	Z2Backoff          time.Duration // TODO: move to z2 client params
	RackRestartDelay   time.Duration
}

type TelegramConfig struct {
	ChatID string `yaml:"chat_id"`
}

type QmssngrConfig struct {
	ChatID string `yaml:"chat_id"`
}

type InfraConfig struct {
	ServiceID     int `yaml:"service_id"`
	EnvironmentID int `yaml:"environment_id"`
}

type JugglerConfig struct {
	Host      string   `yaml:"host"`
	Alerts    []string `yaml:"alerts"`
	Namespace string   `yaml:"namespace"`
	Project   string   `yaml:"project"`
}

type Z2Config struct {
	Group           string `yaml:"group"`
	MetaGroup       string `yaml:"meta_group"`
	CanaryGroup     string `yaml:"canary_group"`
	ConfigMetaGroup string `yaml:"config_meta_group"`
}

type Duration time.Duration

func (d *Duration) UnmarshalYAML(unmarshal func(interface{}) error) error {
	var s string
	err := unmarshal(&s)
	if err != nil {
		return err
	}

	val, err := time.ParseDuration(s)
	if err != nil {
		return err
	}

	*d = Duration(val)

	return nil
}

type UnitConfig struct {
	Name         string   `yaml:"name"`
	RestartDelay Duration `yaml:"restart_delay"`
	RestartsFile string   `yaml:"restarts_file"`
}

type TargetConfig struct {
	Name           string       `yaml:"name"`
	BinaryPackages []string     `yaml:"binary_packages"`
	ConfigPackage  string       `yaml:"config_package"`
	Services       []UnitConfig `yaml:"services"`
	Z2             *Z2Config    `yaml:"z2"`
	Tags           []string     `yaml:"tags"`
	MaxParallelism int          `yaml:"max_parallelism"`
}

func (t *TargetConfig) hasTag(s string) bool {
	for _, tag := range t.Tags {
		if tag == s {
			return true
		}
	}
	return false
}

type ZoneConfig struct {
	Telegram *TelegramConfig `yaml:"telegram"`
	Qmssngr  *QmssngrConfig  `yaml:"qmssngr"`
	Infra    *InfraConfig    `yaml:"infra"`
	Juggler  *JugglerConfig  `yaml:"juggler"`
	Targets  []*TargetConfig `yaml:"targets"`
}

type ClusterConfig map[string]ZoneConfig

type ServiceConfig struct {
	Name        string `yaml:"name"`
	Description string `yaml:"description"`

	Clusters map[string]ClusterConfig `yaml:"clusters"`
}

func getChatID(
	mssngrType messenger.MessengerType,
	zone *ZoneConfig,
) (string, error) {
	if mssngrType == messenger.Telegram {
		return zone.Telegram.ChatID, nil
	}
	if mssngrType == messenger.QMssngr {
		return zone.Qmssngr.ChatID, nil
	}
	return "", fmt.Errorf("unsupported messenger type %v", mssngrType)
}

////////////////////////////////////////////////////////////////////////////////

type durableZ2Client struct {
	logutil.WithLog

	impl        z2.Z2ClientIface
	maxAttempts int
	backoff     time.Duration
	onFailure   func(int, time.Duration, error)
}

func (c *durableZ2Client) retry(
	ctx context.Context,
	call func(ctx context.Context) error,
) error {
	attempts := 0
	backoff := c.backoff
	for {
		attempts++
		err := call(ctx)
		if err == nil || attempts >= c.maxAttempts {
			return err
		}
		c.onFailure(attempts, backoff, err)

		select {
		case <-ctx.Done():
			return ctx.Err()
		case <-time.After(backoff):
			backoff *= 2
			continue
		}
	}
}

func (c *durableZ2Client) editItems(
	ctx context.Context,
	configID string,
	items []z2.Z2Item,
) error {
	return c.retry(ctx, func(ctx context.Context) error {
		return c.impl.EditItems(ctx, configID, items)
	})
}

func (c *durableZ2Client) listWorkers(
	ctx context.Context,
	configID string,
) ([]string, error) {

	var result []string

	err := c.retry(ctx, func(ctx context.Context) error {
		var err error
		result, err = c.impl.ListWorkers(ctx, configID)
		return err
	})

	return result, err
}

func (c *durableZ2Client) updateSyncImpl(
	ctx context.Context,
	configID string,
	forceYes bool,
) error {

	c.LogDbg(ctx, "Z2 update: %v", configID)

	err := c.impl.Update(ctx, configID, forceYes)
	if err != nil {
		return fmt.Errorf("can't update group: %w", err)
	}

	for {
		r, err := c.impl.UpdateStatus(ctx, configID)
		if err != nil {
			return fmt.Errorf("can't fetch update status: %w", err)
		}

		if r.Status != "FINISHED" {
			n := 5 + rand.Intn(10)

			select {
			case <-ctx.Done():
				return ctx.Err()
			case <-time.After(time.Duration(n) * time.Second):
				continue
			}
		}

		if r.Result == "SUCCESS" {
			break
		}

		return fmt.Errorf("failed to update workers: %v", r.FailedWorkers)
	}

	return nil
}

func (c *durableZ2Client) updateSync(
	ctx context.Context,
	configID string,
	forceYes bool,
) error {
	return c.retry(ctx, func(ctx context.Context) error {
		return c.updateSyncImpl(ctx, configID, forceYes)
	})
}

////////////////////////////////////////////////////////////////////////////////

func parseServiceRestartsCount(lines []string) (bool, int, error) {
	if len(lines) < 2 {
		return false, 0, errors.New("bad input")
	}

	if strings.TrimPrefix(lines[0], "ActiveState=") != "active" {
		return false, 0, nil
	}

	count, err := strconv.Atoi(strings.TrimPrefix(lines[1], "RestartsCount="))
	if err != nil {
		return false, 0, fmt.Errorf("can't parse restarts count: %w", err)
	}

	return true, count, nil
}

////////////////////////////////////////////////////////////////////////////////

func parseServiceActiveTime(lines []string) (bool, time.Duration, error) {
	if len(lines) < 3 {
		return false, 0, errors.New("bad input")
	}

	if strings.TrimPrefix(lines[0], "ActiveState=") != "active" {
		return false, 0, nil
	}

	activeEnter, err := strconv.Atoi(
		strings.TrimPrefix(lines[1], "ActiveEnterTimestampMonotonic="))

	if err != nil {
		return true, 0, fmt.Errorf(
			"can't parse ActiveEnterTimestampMonotonic: %w", err)
	}

	current, err := strconv.Atoi(
		strings.TrimPrefix(lines[2], "CurrentTimestampMonotonic="))

	if err != nil {
		return true, 0, fmt.Errorf(
			"can't parse CurrentTimestampMonotonic: %w", err)
	}

	uptime := time.Duration(current-activeEnter) * time.Microsecond

	return true, uptime, nil
}

////////////////////////////////////////////////////////////////////////////////

type Release struct {
	logutil.WithLog

	opts   *Options
	config *ServiceConfig

	zone      *ZoneConfig
	targets   []*TargetConfig
	downhosts sort.StringSlice

	psshTmpl *template.Template

	infra   infra.InfraClientIface
	juggler juggler.JugglerClientIface
	pssh    pssh.PsshIface
	st      st.StarTrackClientIface
	mssngr  messenger.MessengerClientIface
	walle   walle.WalleClientIface
	z2      z2.Z2ClientIface
}

func (r *Release) waitForConfirmation(ctx context.Context, format string, args ...interface{}) error {
	if r.opts.Yes {
		return nil
	}

	message := fmt.Sprintf(format, args...)

	reader := bufio.NewReader(os.Stdin)

	for {
		fmt.Printf("%v, continue? (Y/n) ", message)
		text, _ := reader.ReadString('\n')
		text = strings.TrimRight(text, "\n")

		if text == "n" {
			return errors.New("abort")
		}

		if text == "Y" {
			break
		}

		fmt.Println("unsupported reaction, try again")
	}

	return nil
}

func (r *Release) testPssh(ctx context.Context) error {
	var failedHosts []string

	z2Client := r.newDurableZ2Client(ctx)

	for _, target := range r.targets {
		if target.Z2 == nil {
			continue
		}

		hosts, err := z2Client.listWorkers(ctx, target.Z2.Group)
		if err != nil {
			r.LogError(
				ctx,
				"can't get workers for Z2 group '%v' for target '%v': %v",
				target.Z2.Group,
				target.Name,
				err,
			)
			continue
		}

		sort.Strings(hosts)

		for _, host := range hosts {
			lines, err := r.pssh.Run(ctx, "echo a", host)
			if err != nil {
				failedHosts = append(failedHosts, host)
				continue
			}

			if len(lines) != 1 || lines[0] != "a" {
				r.LogError(
					ctx,
					"unexpected output from host '%v' for target '%v': %v",
					host,
					target.Name,
					lines,
				)
				failedHosts = append(failedHosts, host)
				continue
			}

			return nil
		}
	}

	if len(failedHosts) == 0 {
		return errors.New("misconfigured cluster - no hosts found")
	}

	return fmt.Errorf("test pssh run failed at hosts %v", failedHosts)
}

func (r *Release) report(ctx context.Context, format string, args ...interface{}) {
	var err error

	message := fmt.Sprintf(format, args...)

	if r.st != nil {
		err = r.st.Comment(ctx, message)
		if err != nil {
			r.LogError(ctx, "[ERROR] Can't comment to StartTrack: %v", err)
		}
	}

	if r.mssngr != nil {
		chatID, err := getChatID(r.mssngr.GetMessengerType(), r.zone)
		if err == nil && chatID != "" {
			err = r.mssngr.SendMessage(ctx, chatID, message)
			if err != nil {
				r.LogError(ctx, "[ERROR] Can't send message to messenger: %v", err)
			}
		}
	}

	r.LogInfo(ctx, "[REPORT] %v", message)
}

func (r *Release) createInfraEvent(
	ctx context.Context,
	t0 time.Time,
	duration time.Duration,
) (infra.EventID, error) {

	eventID, err := r.infra.CreateEvent(
		ctx,
		r.zone.Infra.ServiceID,
		r.zone.Infra.EnvironmentID,
		fmt.Sprintf("%v release %v", r.config.Description, r.opts.Description),
		t0,
		duration,
	)
	if err != nil {
		return 0, fmt.Errorf("can't create Infra event: %w", err)
	}

	return eventID, err
}

type muteDescr struct {
	alertName string
	muteID    juggler.MuteID
}

func (r *Release) setupMutes(
	ctx context.Context,
	t0 time.Time,
	duration time.Duration,
) ([]muteDescr, error) {

	if r.zone.Juggler == nil {
		return nil, nil
	}

	var mutes []muteDescr

	for _, alertName := range r.zone.Juggler.Alerts {
		muteID, err := r.juggler.SetMutes(
			ctx,
			alertName,
			r.zone.Juggler.Host,
			t0,
			duration,
			r.zone.Juggler.Namespace,
			r.zone.Juggler.Project,
		)
		if err != nil {
			return nil, fmt.Errorf("can't set mute for %v: %w", alertName, err)
		}

		mutes = append(mutes, muteDescr{alertName, muteID})
	}

	return mutes, nil
}

func (r *Release) extendMutes(
	ctx context.Context,
	t0 time.Time,
	eventID infra.EventID,
) {
	var err error

	now := time.Now()
	duration := now.Add(25 * time.Minute).Sub(t0)

	err = r.infra.ExtendEvent(ctx, eventID, t0.Add(duration))
	if err != nil {
		r.LogError(
			ctx,
			"fail to extend Infra event: %v",
			err,
		)
	}

	mutes, err := r.setupMutes(ctx, t0, duration)
	if err != nil {
		r.LogError(
			ctx,
			"fail to extend Juggler mutes: %v",
			err,
		)
	}
	if len(mutes) != 0 {
		updateMutesMessage := "Mutes updated: "
		for i := range mutes {
			if i != 0 {
				updateMutesMessage += " "
			}
			updateMutesMessage += fmt.Sprintf(
				"((https://juggler.yandex-team.ru/mutes/?query=mute_id=%v %v))",
				mutes[i].muteID,
				mutes[i].alertName,
			)
		}
		r.LogInfo(ctx, updateMutesMessage)
	}
}

func (r *Release) setupInfraAndMutes(ctx context.Context, releaseDescr string) error {
	if r.opts.SkipInfraAndMutes {
		return nil
	}

	duration := 25 * time.Minute
	t0 := time.Now()

	eventID, err := r.createInfraEvent(
		ctx,
		t0,
		duration,
	)
	if err != nil {
		return err
	}

	startReleaseMessage := fmt.Sprintf(
		"Starting %v; \nInfra event: https://infra.yandex-team.ru/event/%v ",
		releaseDescr,
		eventID,
	)

	mutes, err := r.setupMutes(ctx, t0, duration)
	if err != nil {
		return err
	}

	if len(mutes) != 0 {
		startReleaseMessage += "; \nMutes: "
	}

	for i := range mutes {
		if i != 0 {
			startReleaseMessage += " "
		}
		startReleaseMessage += fmt.Sprintf(
			"((https://juggler.yandex-team.ru/mutes/?query=mute_id=%v %v))",
			mutes[i].muteID,
			mutes[i].alertName,
		)
	}

	r.report(ctx, startReleaseMessage)

	go func() {
		for {
			select {
			case <-time.After(10 * time.Minute):
				r.extendMutes(ctx, t0, eventID)
			case <-ctx.Done():
				return
			}
		}
	}()

	return nil
}

func (r *Release) makeRestartString(
	serviceName string,
	delay time.Duration,
	restartsFile string,
) (string, error) {

	var tmplName = "generic"
	if r.psshTmpl.Lookup(serviceName) != nil {
		tmplName = serviceName
	}

	type tmplData struct {
		Delay        int
		Name         string
		RestartsFile string
	}

	var output strings.Builder
	err := r.psshTmpl.ExecuteTemplate(&output, tmplName, tmplData{
		int(delay.Seconds()),
		serviceName,
		restartsFile,
	})
	if err != nil {
		return "", fmt.Errorf("failed to execute template '%v': %w", tmplName, err)
	}

	var commands []string

	// skip empty strings
	for _, c := range strings.Split(output.String(), "\n") {
		if len(c) != 0 {
			commands = append(commands, c)
		}
	}

	return strings.Join(commands, " && "), nil
}

func (r *Release) restartService(
	ctx context.Context,
	service UnitConfig,
	host string,
	hasDowntime bool,
) error {

	delay := time.Duration(service.RestartDelay)
	if delay == 0 {
		delay = 5 * time.Second
	}

	cmd, err := r.makeRestartString(service.Name, delay, service.RestartsFile)
	if err != nil {
		return fmt.Errorf("can't make pssh command string: %w", err)
	}

	if service.Name == "nbs" {
		rmCmd := "sudo rm -f /tmp/yc-nbs-restart.out /tmp/nbs2.err /tmp/nbs2.out"
		_, err = r.pssh.Run(ctx, rmCmd, host)
		if err != nil {
			return fmt.Errorf("failed to execute command '%v': %w", cmd, err)
		}
	}

	lines, err := r.pssh.Run(ctx, cmd, host)
	if err != nil {
		return fmt.Errorf("failed to execute command '%v': %w", cmd, err)
	}

	systemdLines := lines
	if service.Name == "nbs" {
		systemdLines = nil
		for i, line := range lines {
			if strings.HasPrefix(line, "ActiveState=") {
				systemdLines = lines[i:]
				break
			}
		}
	}

	if service.RestartsFile != "" {
		// [ActiveState, RestartsCount]

		alive, restarts, err := parseServiceRestartsCount(systemdLines)
		if err != nil {
			return fmt.Errorf(
				"failed to parse service restarts count'%v': %w",
				strings.Join(systemdLines, "\\n"),
				err,
			)
		}

		if !alive {
			return errors.New("service is dead after restart")
		}

		if restarts > 1 {
			r.LogError(
				ctx,
				"service %v on %v (downtime: %v) restarts: %v",
				service.Name,
				host,
				hasDowntime,
				restarts,
			)

			return fmt.Errorf(
				"service seems to be unstable, restarted (%v) > 1 times",
				restarts,
			)
		}

		systemdLines = systemdLines[2:]

	} else {

		// [ActiveState, ActiveEnterTimestamp, CurrentTimestamp]

		alive, uptime, err := parseServiceActiveTime(systemdLines)
		if err != nil {
			return fmt.Errorf(
				"failed to parse service active time'%v': %w",
				strings.Join(systemdLines, "\\n"),
				err,
			)
		}

		if !alive {
			return errors.New("service is dead after restart")
		}

		if uptime < delay-2*time.Second {
			r.LogError(
				ctx,
				"service %v on %v (downtime: %v) uptime: %v",
				service.Name,
				host,
				hasDowntime,
				uptime,
			)

			return fmt.Errorf(
				"service seems to be unstable after restart, uptime (%v) < %v",
				uptime,
				delay,
			)
		}

		systemdLines = systemdLines[3:]
	}

	if service.Name == "nbs" {
		// [ping title, ping result]

		if len(systemdLines) < 2 {
			return fmt.Errorf("incomplete output: %v", systemdLines)
		}

		if !strings.Contains(systemdLines[0], "=== Ping NBS ===") || systemdLines[1] != "OK" {
			return fmt.Errorf("failed to ping nbs: %v %v", systemdLines[0], systemdLines[1])
		}
	}

	return nil
}

func (r *Release) hasDowntime(
	ctx context.Context,
	host string,
	useCache bool,
) (bool, error) {
	if useCache {
		i := r.downhosts.Search(host)
		return i < r.downhosts.Len() && r.downhosts[i] == host, nil
	}

	return r.juggler.HasDowntime(ctx, host)
}

func (r *Release) restartHost(
	ctx context.Context,
	target *TargetConfig,
	host string,
) error {
	hasDowntime, downtimeErr := r.hasDowntime(ctx, host, true)
	if downtimeErr != nil {
		return downtimeErr
	}

	for _, s := range target.Services {
		r.LogDbg(ctx, "restart service %v on host %v", s.Name, host)

		err := r.restartService(ctx, s, host, hasDowntime)
		if err != nil {
			hasDowntime, downtimeErr = r.hasDowntime(ctx, host, false)
			if downtimeErr != nil {
				return downtimeErr
			}

			msg := fmt.Sprintf(
				"failed to restart %v on host %v (downtime: %v)",
				s.Name,
				host,
				hasDowntime,
			)
			if hasDowntime {
				r.LogWarn(ctx, "%v: %v", msg, err)
			} else {
				return fmt.Errorf("%v: %w", msg, err)
			}
		}
	}

	return nil
}

func (r *Release) updateAndDeployCanaryGroup(
	ctx context.Context,
	z2Client durableZ2Client,
	target *TargetConfig,
	forceYes bool,
) error {
	if target.Z2.CanaryGroup == "" {
		return nil
	}

	err := z2Client.updateSync(ctx, target.Z2.CanaryGroup, forceYes)
	if err != nil {
		return fmt.Errorf(
			"can't update Z2 canary group '%v' for target '%v': %w",
			target.Z2.CanaryGroup,
			target.Name,
			err,
		)
	}
	r.report(
		ctx,
		"[%v/control_panel?configId=%v] Updated canary group",
		z2.Z2Url,
		target.Z2.CanaryGroup,
	)
	canaryHosts, err := z2Client.listWorkers(ctx, target.Z2.CanaryGroup)
	if err != nil {
		return fmt.Errorf(
			"can't get workers for Z2 canary group '%v' for target '%v': %w",
			target.Z2.CanaryGroup,
			target.Name,
			err,
		)
	}

	r.LogDbg(ctx, "Canary hosts: %v", canaryHosts)

	canaryHosts = r.filterHosts(canaryHosts)
	if r.opts.NoRestart || len(canaryHosts) == 0 {
		return nil
	}

	if len(canaryHosts) == 0 {
		r.report(ctx, "all canary hosts in downtime, waiting for confirmation to continue")
		return r.waitForConfirmation(ctx, "all canary hosts in downtime")
	}

	for _, host := range canaryHosts {
		err = r.restartCanary(ctx, target, host)
		if err != nil {
			return err
		}
	}

	return nil
}

func (r *Release) buildItems(
	target *TargetConfig,
	commonPackageVersion string,
	packageVersions *PackageVersions,
) []z2.Z2Item {
	var items []z2.Z2Item

	for _, pkgName := range target.BinaryPackages {
		version, found := (*packageVersions)[pkgName]
		if !found {
			version = commonPackageVersion
		}

		items = append(items, z2.Z2Item{Name: pkgName, Version: version})
	}

	return items
}

func (r *Release) restartCanary(
	ctx context.Context,
	target *TargetConfig,
	canaryHost string,
) error {
	err := r.restartHost(ctx, target, canaryHost)
	if err != nil {
		return fmt.Errorf(
			"can't restart canary host '%v' for target '%v': %w",
			canaryHost,
			target.Name,
			err,
		)
	}

	r.report(
		ctx,
		"restarted canary host %v for target %v, waiting for confirmation to continue",
		canaryHost,
		target.Name,
	)
	return r.waitForConfirmation(ctx, "Canary host %v restarted", canaryHost)
}

func (r *Release) filterHosts(
	allHosts []string,
) []string {
	if len(r.opts.HostsFilter) == 0 {
		return allHosts
	}

	var hosts []string
	for _, host := range allHosts {
		if _, found := r.opts.HostsFilter[host]; found {
			hosts = append(hosts, host)
		}
	}

	return hosts
}

func (r *Release) restartHosts(
	ctx context.Context,
	z2Client durableZ2Client,
	target *TargetConfig,
	useCanaryGroup bool,
) error {
	if r.opts.NoRestart {
		return nil
	}

	var err error

	rackRestartDelay := r.opts.RackRestartDelay
	if rackRestartDelay == 0 {
		rackRestartDelay = 10 * time.Second
	}

	allHosts, err := z2Client.listWorkers(ctx, target.Z2.Group)
	if err != nil {
		return fmt.Errorf(
			"can't get workers for Z2 group '%v' for target '%v': %w",
			target.Z2.Group,
			target.Name,
			err,
		)
	}

	allHosts = r.filterHosts(allHosts)
	if len(allHosts) == 0 {
		r.report(ctx, "no hosts for '%v', nothing to do", target.Name)
		return nil
	}

	if len(allHosts) == 0 {
		r.report(ctx, "all hosts in downtime for '%v', nothing to do", target.Name)
		return nil
	}

	var restartedCount = 0
	var allHostsCount = len(allHosts)

	if len(target.Z2.CanaryGroup) == 0 && useCanaryGroup {
		num := -1
		for i, host := range allHosts {
			hasDowntime, downtimeErr := r.hasDowntime(ctx, host, true)
			if downtimeErr != nil {
				return downtimeErr
			}

			if hasDowntime {
				continue
			}

			err = r.restartCanary(ctx, target, host)
			if err != nil {
				hasDowntime, downtimeErr = r.hasDowntime(ctx, host, false)
				if downtimeErr != nil {
					return downtimeErr
				}

				if hasDowntime {
					continue
				}

				return err
			}

			num = i
			break
		}

		if num != -1 {
			restartedCount = 1
			allHosts[num] = allHosts[len(allHosts)-1]
			allHosts = allHosts[:len(allHosts)-1]
		}
	}

	if len(allHosts) == 0 {
		return nil
	}

	var hostsByLocation map[string][]string

	if target.hasTag("svm") {
		hostsByLocation = map[string][]string{
			"svm": allHosts,
		}
	} else {
		hostsByLocation, err = r.walle.SplitByLocation(ctx, allHosts)
		if err != nil || len(hostsByLocation) == 0 {
			return fmt.Errorf(
				"unable to get host locations for target '%v': %w",
				target.Name,
				err,
			)
		}
	}

	par := r.opts.Parallelism
	if target.MaxParallelism != 0 && target.MaxParallelism < par {
		par = target.MaxParallelism
	}

	var tasks = make(chan string, par)
	var results = make(chan error)
	var fin = make(chan error)

	for i := 0; i < par; i++ {
		go func() {
			for host := range tasks {
				results <- r.restartHost(ctx, target, host)
			}
		}()
	}

	var restartProblems []string

	restartMessage := func() string {
		return fmt.Sprintf(
			"[%v] Restarted %v/%v",
			target.Z2.Group,
			restartedCount,
			allHostsCount,
		)
	}

	processResults := func(count int) {
		for i := 0; i != count; i++ {
			err := <-results
			restartedCount++

			if err != nil {
				restartProblems = append(restartProblems, err.Error())

				if len(restartProblems) > r.opts.MaxRestartProblems {
					fin <- errors.New(strings.Join(restartProblems, "\n"))
					return
				}
				r.report(ctx, "problem with restart: %v", err)
			}

			n := allHostsCount / 3
			if n < 8 {
				n = 8
			}

			if restartedCount%n == 0 {
				r.report(ctx, restartMessage())
			}
		}
		fin <- nil
	}

	restartLocation := func(location string) error {
		hosts := hostsByLocation[location]

		r.LogInfo(ctx, "restart location '%v' (%v hosts)", location, len(hosts))

		go processResults(len(hosts))

		for _, host := range hosts {
			tasks <- host

			select {
			case <-ctx.Done():
				return ctx.Err()
			case err := <-fin:
				return err
			default:
			}
		}

		return <-fin
	}

	defer close(tasks)

	var locations []string
	for location := range hostsByLocation {
		locations = append(locations, location)
	}
	sort.Strings(locations)

	for i, location := range locations {
		err = restartLocation(location)
		if err != nil {
			return err
		}

		if i != len(locations)-1 {
			select {
			case <-ctx.Done():
				return ctx.Err()
			case <-time.After(rackRestartDelay):
			}
		}
	}

	r.report(ctx, restartMessage())

	return nil
}

func (r *Release) newDurableZ2Client(ctx context.Context) durableZ2Client {
	return durableZ2Client{
		logutil.WithLog{Log: r.Log},
		r.z2,
		r.opts.Z2Attempts,
		r.opts.Z2Backoff,
		func(attempts int, backoff time.Duration, err error) {
			msg := fmt.Sprintf(
				"failed to execute z2 request: %v, remaining attempts: %v, backoff: %v",
				err,
				r.opts.Z2Attempts-attempts,
				backoff,
			)

			r.LogWarn(ctx, msg)

			if r.mssngr != nil {
				chatID, err := getChatID(r.mssngr.GetMessengerType(), r.zone)
				if err == nil && chatID != "" {
					_ = r.mssngr.SendMessage(
						ctx,
						chatID,
						msg,
					)
				}
			}
		},
	}
}

func (r *Release) updateAndDeploy(
	ctx context.Context,
	target *TargetConfig,
	usePreviousVersion bool,
	useCanaryGroup bool,
) error {
	var err error

	var configVersion string
	var commonPackageVersion string
	var packageVersions *PackageVersions
	var forceYes bool

	if usePreviousVersion {
		configVersion = r.opts.PrevConfigVersion
		commonPackageVersion = r.opts.PrevCommonPackageVersion
		packageVersions = &r.opts.PrevPackageVersions
		forceYes = true
	} else {
		configVersion = r.opts.ConfigVersion
		commonPackageVersion = r.opts.CommonPackageVersion
		packageVersions = &r.opts.PackageVersions
		forceYes = r.opts.Force
	}

	if target.Z2 == nil {
		// XXX
		return nil
	}

	updateMessage := ""

	z2Client := r.newDurableZ2Client(ctx)

	if commonPackageVersion == "" && configVersion == "" {
		r.report(ctx, "Nothing to update in Z2")

		return r.restartHosts(ctx, z2Client, target, useCanaryGroup)
	}

	if commonPackageVersion != "" {
		items := r.buildItems(target, commonPackageVersion, packageVersions)
		err = z2Client.editItems(ctx, target.Z2.MetaGroup, items)
		if err != nil {
			return fmt.Errorf(
				"can't edit Z2 meta group '%v' for target '%v': %w",
				target.Z2.MetaGroup,
				target.Name,
				err,
			)
		}

		updateMessage += fmt.Sprintf(
			"[%v/meta_control_panel?configId=%v] Updated packages to version %v",
			z2.Z2Url,
			target.Z2.MetaGroup,
			commonPackageVersion,
		)

		if r.opts.UpdateMetaGroups {
			err = z2Client.updateSync(ctx, target.Z2.MetaGroup, forceYes)
			if err != nil {
				return fmt.Errorf(
					"can't update Z2 meta group '%v' for target '%v': %w",
					target.Z2.MetaGroup,
					target.Name,
					err,
				)
			}

			updateMessage += fmt.Sprintf(
				"; \n[%v/control_panel?configId=%v] Updated group",
				z2.Z2Url,
				target.Z2.MetaGroup,
			)
		}
	}

	if configVersion != "" {
		err = z2Client.editItems(ctx, target.Z2.ConfigMetaGroup, []z2.Z2Item{
			z2.Z2Item{
				Name:    target.ConfigPackage,
				Version: configVersion,
			},
		})
		if err != nil {
			return fmt.Errorf(
				"can't edit Z2 config meta group '%v' for target '%v': %w",
				target.Z2.ConfigMetaGroup,
				target.Name,
				err,
			)
		}
		if len(updateMessage) != 0 {
			updateMessage += "; \n"
		}
		updateMessage += fmt.Sprintf(
			"[%v/meta_control_panel?configId=%v] Updated packages to version %v",
			z2.Z2Url,
			target.Z2.ConfigMetaGroup,
			configVersion,
		)

		if r.opts.UpdateMetaGroups {
			err = z2Client.updateSync(ctx, target.Z2.ConfigMetaGroup, forceYes)
			if err != nil {
				return fmt.Errorf(
					"can't update Z2 config meta group '%v' for target '%v': %w",
					target.Z2.ConfigMetaGroup,
					target.Name,
					err,
				)
			}
			updateMessage += fmt.Sprintf(
				"; \n[%v/control_panel?configId=%v] Updated group",
				z2.Z2Url,
				target.Z2.ConfigMetaGroup,
			)
		}
	}

	err = r.updateAndDeployCanaryGroup(ctx, z2Client, target, forceYes)
	if err != nil {
		return err
	}

	err = z2Client.updateSync(ctx, target.Z2.Group, forceYes)
	if err != nil {
		return fmt.Errorf(
			"can't update Z2 group '%v' for target '%v': %w",
			target.Z2.Group,
			target.Name,
			err,
		)
	}

	updateMessage += fmt.Sprintf(
		"; \n[%v/control_panel?configId=%v] Updated group",
		z2.Z2Url,
		target.Z2.Group,
	)

	r.report(ctx, updateMessage)

	return r.restartHosts(ctx, z2Client, target, useCanaryGroup)
}

func (r *Release) runImpl(ctx context.Context) error {
	var err error

	releaseDescr := fmt.Sprintf(
		"release: https://st.yandex-team.ru/%v (%v, %v, %v)",
		r.opts.Ticket,
		r.opts.Description,
		r.opts.ClusterName,
		r.opts.ZoneName,
	)

	err = r.setupInfraAndMutes(ctx, releaseDescr)
	if err != nil {
		return err
	}

	checkBackwardCompatibility := false

	for _, target := range r.targets {
		if err = r.updateAndDeploy(ctx, target, false, true); err != nil {
			return err
		}

		if target.hasTag("check-backward-compatibility") &&
			r.opts.CommonPackageVersion != "" &&
			r.opts.PrevCommonPackageVersion != "" {

			checkBackwardCompatibility = true
		}
	}

	if checkBackwardCompatibility {
		r.report(ctx, "new version '%v' is deployed, waiting for confirmation to continue", r.opts.CommonPackageVersion)
		if err = r.waitForConfirmation(ctx, "new version is deployed"); err != nil {
			return err
		}

		// TODO: (NBS-1533) run corruption tests for hw-nbs-stable-lab

		for _, target := range r.targets {
			if target.hasTag("check-backward-compatibility") {
				if err = r.updateAndDeploy(ctx, target, true, false); err != nil {
					return err
				}
			}
		}

		r.report(ctx, "old version '%v' is deployed, waiting for confirmation to continue", r.opts.PrevCommonPackageVersion)
		if err = r.waitForConfirmation(ctx, "old version is deployed"); err != nil {
			return err
		}

		// TODO: (NBS-1533) run corruption tests for hw-nbs-stable-lab

		for _, target := range r.targets {
			if target.hasTag("check-backward-compatibility") {
				if err = r.updateAndDeploy(ctx, target, false, false); err != nil {
					return err
				}
			}
		}
	}

	r.report(ctx, "Finished %v", releaseDescr)

	return nil
}

func (r *Release) Run(ctx context.Context) error {
	var err error

	funcs := template.FuncMap{
		"seq": func(args ...string) []string {
			var seq []string
			return append(seq, args...)
		},
		"sum": func(args ...int) int {
			var r = 0
			for _, v := range args {
				r += v
			}
			return r
		},
	}

	r.psshTmpl, err = template.New("pssh").Funcs(funcs).Parse(psshTemplate)
	if err != nil {
		return fmt.Errorf("failed to parse template command '%v': %w", psshTemplate, err)
	}

	if r.opts.ClusterName == "" {
		return errors.New("cluster name is not specified")
	}

	if r.opts.ZoneName == "" {
		return errors.New("zone name is not specified")
	}

	cluster, found := r.config.Clusters[r.opts.ClusterName]

	if !found {
		return fmt.Errorf("unknown cluster name '%v'", r.opts.ClusterName)
	}

	zone, found := cluster[r.opts.ZoneName]
	if !found {
		return fmt.Errorf(
			"unknown zone name '%v' for cluster '%v'",
			r.opts.ZoneName,
			r.opts.ClusterName,
		)
	}

	if r.opts.TargetName != "" {
		found = func() bool {
			for _, t := range zone.Targets {
				if t.Name == r.opts.TargetName {
					r.targets = append(r.targets, t)
					return true
				}
			}
			return false
		}()

		if !found {
			return fmt.Errorf(
				"unknown target name '%v' for cluster '%v' and zone '%v'",
				r.opts.TargetName,
				r.opts.ZoneName,
				r.opts.ClusterName,
			)
		}
	} else {
		r.targets = zone.Targets
	}

	r.zone = &zone

	if r.opts.Ticket == "" || r.zone.Telegram == nil || r.zone.Telegram.ChatID == "" {
		err = r.waitForConfirmation(ctx, "ticket and/or chat-id not specified")
		if err != nil {
			return err
		}
	}

	err = r.testPssh(ctx)
	if err != nil {
		return err
	}

	r.downhosts, err = r.juggler.GetDownHosts(ctx)
	if err != nil {
		return fmt.Errorf("can't get downhosts: %w", err)
	}
	r.downhosts.Sort()

	ctx, cancel := context.WithCancel(ctx)
	defer cancel()

	err = r.runImpl(ctx)
	if err != nil {
		r.report(ctx, "release failed with error: %v", err)
	}

	return err
}

////////////////////////////////////////////////////////////////////////////////

func NewRelease(
	log nbs.Log,
	opts *Options,
	config *ServiceConfig,
	infra infra.InfraClientIface,
	juggler juggler.JugglerClientIface,
	pssh pssh.PsshIface,
	st st.StarTrackClientIface,
	mssngr messenger.MessengerClientIface,
	walle walle.WalleClientIface,
	z2 z2.Z2ClientIface,
) *Release {

	return &Release{
		WithLog: logutil.WithLog{
			Log: log,
		},
		opts:    opts,
		config:  config,
		infra:   infra,
		juggler: juggler,
		pssh:    pssh,
		st:      st,
		mssngr:  mssngr,
		walle:   walle,
		z2:      z2,
	}
}
