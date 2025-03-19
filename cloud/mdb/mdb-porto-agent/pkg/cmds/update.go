package cmds

import (
	"bufio"
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io/ioutil"
	"net/http"
	"os"
	"path"
	"strconv"
	"strings"
	"time"

	"github.com/spf13/cobra"

	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/config"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/dns"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto/network"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/porto/runner"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/statestore/fs"
	"a.yandex-team.ru/cloud/mdb/mdb-porto-agent/pkg/supp"
	portoapi "a.yandex-team.ru/infra/tcp-sampler/pkg/porto"
	l "a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

var (
	cmdUpdate = initUpdate()

	flagTarget      string
	flagShowChanges bool
	flagSecrets     bool
)

const (
	flagNameAll         = "all"
	flagNameTarget      = "target"
	flagNameShortTarget = "t"
	flagNameShowChanges = "show-changes"
	flagNameSecrets     = "secrets"

	bootstrapDefaultPath = "/usr/local/yandex/porto/mdb_bionic.sh"

	resolveWaitTimeout = 5 * time.Minute
	resolveWaitPeriod  = 10 * time.Second
)

func initUpdate() *cli.Command {
	cmd := &cobra.Command{
		Use:   "update porto state for specified target",
		Short: "update porto state",
		Long:  "update porto state for specified target",
	}

	cmd.Flags().StringVarP(
		&flagTarget,
		flagNameTarget,
		flagNameShortTarget,
		"",
		"set target FQDN container",
	)

	cmd.Flags().BoolVar(
		&flagSecrets,
		flagNameSecrets,
		false,
		"first line from input line should be a Json with secrets",
	)

	cmd.Flags().BoolVar(
		&flagShowChanges,
		flagNameShowChanges,
		false,
		"show update changes in json format",
	)

	if err := cmd.MarkFlagRequired(flagNameTarget); err != nil {
		panic(err)
	}

	return &cli.Command{Cmd: cmd, Run: update}
}

type secretInfo struct {
	Mode    string `json:"mode,omitempty"`
	Content string `json:"content,omitempty"`
}
type secrets map[string]secretInfo

type updateCtx struct {
	log       l.Logger
	container string
	drm       bool
	changes   []string
}

// update porto state for specified target in option (flagTarget)
func update(ctx context.Context, env *cli.Env, cmd *cobra.Command, args []string) {
	conf := config.FromEnv(env)
	ss, err := fs.New(conf.StoreCachePath, conf.StoreLibPath, env.Logger)
	if err != nil {
		env.Logger.Fatalf("failed to create statestore: %v", err)
	}
	var secrets secrets
	if flagSecrets {
		rd := bufio.NewReader(os.Stdin)
		inline, err := rd.ReadBytes('\n')
		if err != nil {
			env.Logger.Fatalf("failed to read line from input stream: %v", err)
		}
		err = json.Unmarshal(inline, &secrets)
		if err != nil {
			env.Logger.Fatalf("failed to parse secrets json from input: %v", err)
		}
		env.Logger.Debugf("read secrets: %v", secrets)
	}

	api, err := portoapi.Connect()
	if err != nil {
		env.Logger.Fatalf("failed to connect porto API: %v", err)
	}

	net := network.New(env.Logger)
	ps, err := porto.New(env.Logger, env.IsDryRunMode(), api, net)
	if err != nil {
		env.Logger.Fatalf("failed to create porto support: %v", err)
	}

	env.Logger.Infof("update porto state for target %s", flagTarget)
	container, st, err := ss.GetTargetState(flagTarget)
	if err != nil {
		if !env.IsDryRunMode() {
			env.Logger.Fatalf("failed get target %s state: %+v", flagTarget, err)
		}
		fmt.Printf("you must prepare state for mdb-porto-agent, failed get target %s state: %s\n", flagTarget, err)
		return
	}

	pr := runner.New(env.Logger)
	uctx := updateCtx{
		log:       env.Logger,
		container: container,
		drm:       env.IsDryRunMode(),
	}
	changes, err := updateContainer(ctx, uctx, ps, pr, ss, secrets, st, conf.DbmURL, dns.NewDefaultResolver(), resolveWaitTimeout, resolveWaitPeriod)
	if flagShowChanges {
		for _, chg := range changes {
			fmt.Println(chg)
		}
	}
	if err != nil {
		env.Logger.Fatalf("fatal update error: %+v", err)
	}
}

// updateContainer porto state for specified container
func updateContainer(
	ctx context.Context,
	uctx updateCtx,
	ps *porto.Support,
	pr porto.Runner,
	ss statestore.Storage,
	secrets secrets,
	st statestore.State,
	dbmURL string,
	resolver dns.Resolver,
	resolveWaitTimeout time.Duration,
	resolveWaitPeriod time.Duration,
) ([]string, error) {
	if st.Options.PendingDelete {
		return ensureDeleted(ctx, uctx, ss, ps, st, dbmURL)
	}

	var err error
	beforeEnsureContainer := len(uctx.changes)
	uctx.changes, err = ps.EnsureContainer(uctx.changes, uctx.container)
	isContainer := len(uctx.changes) == beforeEnsureContainer
	if err != nil {
		return uctx.changes, err
	}

	var volMap map[string]string
	uctx.changes, volMap, err = ps.EnsureVolumes(uctx.changes, uctx.container, st.Volumes, pr)
	if err != nil {
		return uctx.changes, err
	}

	isStopped := false
	if isContainer || !uctx.drm {
		uctx.changes, isStopped, err = ps.EnsureContainerProperties(uctx.changes, uctx.container, st, volMap, isContainer)
		if err != nil {
			return uctx.changes, err
		}
	}

	uctx.changes, err = ensureBootstrapped(uctx, pr, st)
	if err != nil {
		return uctx.changes, err
	}

	uctx.changes, err = ensureSecrets(uctx, st, secrets)
	if err != nil {
		return uctx.changes, err
	}

	uctx.changes, err = ps.EnsureRunning(uctx.changes, uctx.container, isContainer, isStopped)
	if err != nil {
		return uctx.changes, err
	}

	if !uctx.drm {
		// do not check dns in dry run mode
		checker := dns.NewChecker(uctx.container,
			dns.WithResolver(resolver),
			dns.WithTester(func(_ context.Context, err error) bool {
				uctx.log.Warnf("can not resolve %q in dns: %+v", uctx.container, err)
				msg := fmt.Sprintf("run selfdns-client update for container %s", uctx.container)
				err = supp.DoActionWithoutChangelist(uctx.log, uctx.drm, msg, func() error {
					return pr.RunCommandOnPortoContainer(uctx.container, "/usr/bin/sudo", "-u", "selfdns", "/usr/bin/selfdns-client", "--debug")
				})
				if err != nil {
					uctx.log.Warnf("selfdns run failed: %+v", err)
				}

				return false
			}),
		)
		if err = checker.EnsureDNS(ctx, resolveWaitTimeout, resolveWaitPeriod); err != nil {
			uctx.log.Errorf("can not resolve %q in dns during timeout %s", uctx.container, resolveWaitTimeout)
		}
	}

	return uctx.changes, nil
}

// retireVolumeData remove or backup data from volume by dom0path
func retireVolumeData(uctx updateCtx, vo statestore.VolumeOptions) ([]string, error) {
	bf := []l.Field{
		l.String("module", "cmds"),
		l.String("func", "retireVolumeData"),
		l.String("container", uctx.container),
		l.String("path", vo.Path),
		l.String("volume", vo.Dom0Path),
	}
	if _, err := os.Stat(vo.Dom0Path); os.IsNotExist(err) {
		uctx.log.Info("volume not exists", bf...)
		return uctx.changes, nil
	}
	var err error
	if vo.PendingBackup {
		now := time.Now()
		nt := now.Format(statestore.DatetimeTemplate)
		bp := fmt.Sprintf("%s.%s.bak", vo.Dom0Path, nt)
		msg := fmt.Sprintf("backup volume %s", vo.Dom0Path)
		uctx.changes, err = supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
			if err := os.Rename(vo.Dom0Path, bp); err != nil {
				return xerrors.Errorf("failed to rename %s to %s: %w", vo.Dom0Path, bp, err)
			}
			return nil
		})
	} else {
		msg := fmt.Sprintf("remove volume %s", vo.Dom0Path)
		uctx.changes, err = supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
			if err := os.RemoveAll(vo.Dom0Path); err != nil {
				return xerrors.Errorf("failed to remove path %s: %w", vo.Dom0Path, err)
			}
			return nil
		})
	}
	return uctx.changes, err
}

func deleteRequestToDBM(ctx context.Context, uctx updateCtx, dbmURL string, token string) ([]string, error) {
	type deleteReqest struct {
		Token string `json:"token"`
	}

	if token == "" {
		uctx.log.Infof("skipping request to DBM due to missing token")
		return uctx.changes, nil
	}

	d := deleteReqest{token}

	body, err := json.Marshal(d)
	if err != nil {
		return uctx.changes, xerrors.Errorf("failed to prepare token body: %w", err)
	}

	rbody := bytes.NewReader(body)
	url := fmt.Sprintf("%s/api/v2/dom0/delete-report/%s", dbmURL, uctx.container)
	uctx.log.Infof("send update request DNS to url %s, for container %s", url, uctx.container)

	req, err := http.NewRequest("POST", url, rbody)
	if err != nil {
		return uctx.changes, xerrors.Errorf("failed to prepare request to DBM: %w", err)
	}

	ct := "application/json"
	req.Header.Set("Accept", ct)
	req.Header.Set("Content-Type", ct)

	msg := fmt.Sprintf("delete request to DBM for container %s", uctx.container)
	return supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
		resp, err := http.DefaultTransport.RoundTrip(req)
		if err != nil {
			return xerrors.Errorf("failed to send request to DBM: %w", err)
		}
		defer func() { _ = resp.Body.Close() }()

		rc := resp.StatusCode
		if rc != http.StatusOK && rc != http.StatusBadRequest && rc != http.StatusNotFound {
			return xerrors.Errorf("delete request to DBM bad response code %d, text %s, url %s", rc, http.StatusText(rc), url)
		}
		uctx.log.Debugf("delete request to DBM response code %d", rc)
		return nil
	})
}

func ensureDeleted(ctx context.Context, uctx updateCtx, ss statestore.Storage, ps *porto.Support, st statestore.State, dbmURL string) ([]string, error) {
	var err error
	uctx.changes, err = ps.EnsureAbsent(uctx.changes, uctx.container)
	if err != nil {
		return uctx.changes, xerrors.Errorf("failed to ensure absent: %w", err)
	}
	for _, vo := range st.Volumes {
		uctx.changes, err = retireVolumeData(uctx, vo)
		if err != nil {
			return uctx.changes, xerrors.Errorf("failed to ensure absent: %w", err)
		}
	}
	uctx.changes, err = deleteRequestToDBM(ctx, uctx, dbmURL, st.Options.DeleteToken)
	if err != nil {
		return uctx.changes, xerrors.Errorf("failed delete %s to DBM: %w", uctx.container, err)
	}

	uctx.changes, err = removeState(uctx, ss)
	return uctx.changes, err
}

func removeState(uctx updateCtx, ss statestore.Storage) ([]string, error) {
	rsMsg := fmt.Sprintf("remove state for container %s", uctx.container)
	return supp.DoAction(uctx.log, uctx.drm, uctx.changes, rsMsg, func() error {
		return ss.RemoveState(uctx.container)
	})
}

func ensureBootstrapped(uctx updateCtx, pr porto.Runner, st statestore.State) ([]string, error) {
	var rootPath string
	for _, vo := range st.Volumes {
		if vo.Path == "/" {
			rootPath = vo.Dom0Path
			break
		}
	}
	if rootPath == "" {
		return uctx.changes, xerrors.Errorf("unknown root volume for container %s, %v", uctx.container, st.Volumes)
	}
	chkPath := path.Join(rootPath, "bin/ls")
	_, err := os.Stat(chkPath)
	if err == nil || os.IsExist(err) {
		return uctx.changes, nil
	}

	bs := bootstrapDefaultPath
	if st.Options.BootstrapCmd != "" {
		parts := strings.Split(st.Options.BootstrapCmd, " ")
		if len(parts) == 1 || (len(parts) == 2 && parts[1] == uctx.container) {
			bs = parts[0]
		} else {
			return uctx.changes, xerrors.Errorf("invalid bootstrap cmd (expected .sh script) %s", st.Options.BootstrapCmd)
		}
	}
	msg := fmt.Sprintf("bootstrap script '%s %s'", bs, uctx.container)
	return supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
		err = pr.RunCommandOnDom0(bs, uctx.container)
		if err != nil {
			return xerrors.Errorf("%s, exec failed: %+v", msg, err)
		}
		return nil
	})
}

func ensureSecrets(uctx updateCtx, st statestore.State, secrets secrets) ([]string, error) {
	var rootPath string
	for _, vo := range st.Volumes {
		if vo.Path == "/" {
			rootPath = vo.Dom0Path
			break
		}
	}
	for sec, options := range secrets {
		fm := os.FileMode(0600)
		if options.Mode != "" {
			pfm, err := strconv.ParseUint(options.Mode, 0, 32)
			if err != nil {
				return uctx.changes, xerrors.Errorf("failed to parse mode '%s' for secret %s container %s: %w", options.Mode, sec, uctx.container, err)
			}
			fm = os.FileMode(pfm)
		}

		secPath := path.Join(rootPath, sec)
		stat, err := os.Stat(secPath)
		if os.IsNotExist(err) {
			msg := fmt.Sprintf("create new secret %s for container %s", sec, uctx.container)
			uctx.changes, err = supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
				if err := os.MkdirAll(path.Dir(secPath), os.ModeDir|0755); err != nil {
					return err
				}
				return ioutil.WriteFile(secPath, []byte(options.Content), fm)
			})
			if err != nil {
				return uctx.changes, err
			}
			continue
		}

		cfm := stat.Mode()
		if cfm != fm {
			msg := fmt.Sprintf("fixed secret %s filemode: 0%s -> 0%s for container %s", secPath, strconv.FormatUint(uint64(cfm), 8), strconv.FormatUint(uint64(fm), 8), uctx.container)
			uctx.changes, err = supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
				return os.Chmod(secPath, fm)
			})
			if err != nil {
				return uctx.changes, err
			}
		}

		fc, err := ioutil.ReadFile(secPath)
		if err != nil {
			return uctx.changes, xerrors.Errorf("failed to read secret %s for container %s: %w", secPath, uctx.container, err)
		}
		if !bytes.Equal(fc, []byte(options.Content)) {
			msg := fmt.Sprintf("fixed secret %s content for container %s", secPath, uctx.container)
			uctx.changes, err = supp.DoAction(uctx.log, uctx.drm, uctx.changes, msg, func() error {
				return ioutil.WriteFile(secPath, []byte(options.Content), fm)
			})
			if err != nil {
				return uctx.changes, err
			}
		}
	}
	return uctx.changes, nil
}
