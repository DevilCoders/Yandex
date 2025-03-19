package main

import (
	"bufio"
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/exec"
	"path"
	"strings"
	"text/template"
	"time"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/blockstore/config"
	"a.yandex-team.ru/cloud/blockstore/public/api/protos"
	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"
	secutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/secrets"
	st "a.yandex-team.ru/cloud/storage/core/tools/common/go/startrack"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/walle"
	"a.yandex-team.ru/library/go/yatool"

	"github.com/golang/protobuf/jsonpb"
	"github.com/golang/protobuf/proto"
)

////////////////////////////////////////////////////////////////////////////////

type options struct {
	Options

	ServiceConfig string
	PatcherDir    string
	Hosts         string
	PsshPath      string
	YcpPath       string
	Secrets       string

	PsshAttempts int

	NoAuth  bool
	Verbose bool
}

////////////////////////////////////////////////////////////////////////////////

func loadServiceConfig(path string) (*ServiceConfig, error) {
	configTmpl, err := ioutil.ReadFile(path)
	if err != nil {
		return nil, fmt.Errorf("can't read service config: %w", err)
	}

	funcs := template.FuncMap{
		"seq": func(args ...string) []string {
			var seq []string
			return append(seq, args...)
		},
		"ToUpper": strings.ToUpper,
	}

	tmpl, err := template.New("config").Funcs(funcs).Parse(string(configTmpl))
	if err != nil {
		return nil, fmt.Errorf("can't parse service config: %w", err)
	}

	var configYAML strings.Builder

	err = tmpl.Execute(&configYAML, nil)
	if err != nil {
		return nil, fmt.Errorf("can't execute service config: %w", err)
	}

	config := &ServiceConfig{}
	if err := yaml.Unmarshal([]byte(configYAML.String()), config); err != nil {
		return nil, fmt.Errorf("can't unmarshal service config: %w", err)
	}

	return config, nil
}

func loadHosts(ctx context.Context, path string) ([]string, error) {
	file, err := os.Open(path)
	if err != nil {
		return nil, err
	}
	defer file.Close()

	var hosts []string

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		hosts = append(hosts, scanner.Text())
	}

	return hosts, scanner.Err()
}

func updateDRConfigs(path string, dr *protos.TUpdateDiskRegistryConfigRequest) error {
	f, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("can't open '%v': %w", path, err)
	}
	defer f.Close()

	err = proto.MarshalText(f, dr)
	if err != nil {
		return fmt.Errorf("can't write DR config to file '%v': %w", path, err)
	}

	return nil
}

func updateCMSPatch(
	path string,
	agents []*config.TDiskAgentConfig,
	ticket string,
) error {
	if len(agents) == 0 {
		return nil
	}

	patchJSON, err := ioutil.ReadFile(path)
	if err != nil && !os.IsNotExist(err) {
		return fmt.Errorf("can't read patch file %v: %w", path, err)
	}

	type patch struct {
		Hosts  []string               `json:"hosts"`
		Patch  map[string]interface{} `json:"patch"`
		Ticket string                 `json:"ticket"`
	}

	var patches []*patch
	if len(patchJSON) != 0 {
		err = json.Unmarshal([]byte(patchJSON), &patches)
		if err != nil {
			return fmt.Errorf("can't unmarshal patches from %v: %w", path, err)
		}
	}

	for _, agent := range agents {
		// dump agent to json & read back to sort keys
		// jsonpb can dump enums to strings (e.g. "Backend": "DISK_AGENT_BACKEND_AIO")
		m := jsonpb.Marshaler{}
		agentJSON, err := m.MarshalToString(agent)
		if err != nil {
			return fmt.Errorf("can't dump agent config to json %w", err)
		}

		var opaque interface{}
		err = json.Unmarshal([]byte(agentJSON), &opaque)
		if err != nil {
			return fmt.Errorf("can't load agent config from json %w", err)
		}

		patches = append(
			patches,
			&patch{
				Hosts: []string{
					*agent.AgentId,
				},
				Ticket: ticket,
				Patch: map[string]interface{}{
					"DiskAgentConfig": opaque,
				},
			},
		)
	}

	f, err := os.Create(path)
	if err != nil {
		return fmt.Errorf("can't overwrite patch file %v: %w", path, err)
	}
	defer f.Close()

	enc := json.NewEncoder(f)
	enc.SetIndent("", "    ")
	err = enc.Encode(patches)
	if err != nil {
		return fmt.Errorf("can't marshal patch to %v: %w", path, err)
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

func run(ctx context.Context, opts *options) error {
	if len(opts.PatcherDir) == 0 {
		return errors.New("empty path to patcher's folder")
	}

	drConfigFile := path.Join(
		opts.PatcherDir,
		opts.ClusterName,
		opts.ZoneName,
		"disk_registry_config.txt",
	)

	patchFile := path.Join(
		opts.PatcherDir,
		opts.ClusterName,
		opts.ZoneName,
		"patch.json",
	)

	log.Printf("DR config: %v, patch: %v", drConfigFile, patchFile)

	config, err := loadServiceConfig(opts.ServiceConfig)
	if err != nil {
		return err
	}

	secrets, err := secutil.LoadFromFile(opts.Secrets)
	if err != nil {
		return err
	}

	logLevel := nbs.LOG_ERROR
	if opts.Verbose {
		logLevel = nbs.LOG_DEBUG
	}

	hosts, err := loadHosts(ctx, opts.Hosts)
	if err != nil {
		return fmt.Errorf("can't load hosts: %w", err)
	}

	if !opts.NoAuth {
		opts.IAMToken, err = secutil.TryGetIAMToken(
			ctx,
			nbs.NewStderrLog(logLevel),
			secrets.Ycp,
			opts.ClusterName,
			opts.YcpPath,
		)
		if err != nil {
			return fmt.Errorf("can't get IAM token: %w", err)
		}
	}

	replacer := secutil.NewReplacer(secrets, opts.IAMToken)

	logger := nbs.NewLog(
		log.New(
			secutil.NewFilter(os.Stderr, replacer),
			"",
			log.Ltime,
		),
		logLevel,
	)

	dr, agents, err := addAgents(
		ctx,
		logger,
		opts.Options,
		config,
		hosts,
		infra.NewClient(logger, opts.Ticket, secrets.OAuthToken),
		juggler.NewClient(logger, opts.Ticket, secrets.OAuthToken),
		walle.NewClient(logger),
		pssh.NewDurable(
			logger,
			pssh.New(logger, opts.PsshPath, nil),
			opts.PsshAttempts,
		),
		st.NewClient(logger, opts.Ticket, secrets.OAuthToken, replacer),
	)
	if err != nil {
		return err
	}

	err = updateDRConfigs(drConfigFile, dr)
	if err != nil {
		return fmt.Errorf("can't update DR config file: %w", err)
	}

	err = updateCMSPatch(patchFile, agents, opts.Ticket)
	if err != nil {
		return fmt.Errorf("can't update patch file: %w", err)
	}

	return nil
}

func readUserFromPsshConfig(psshPath string) (string, error) {
	cmd := exec.Command(psshPath, "config", "show")
	stdout, err := cmd.Output()
	if err != nil {
		return "", fmt.Errorf("output: %w", err)
	}

	lines := strings.Split(string(stdout), "\n")
	for _, line := range lines {
		if strings.HasPrefix(line, "user: ") {
			arr := strings.Split(line, ": ")
			if len(arr) == 2 {
				return arr[1], nil
			}
		}
	}

	return "", fmt.Errorf("can't find user name")
}

func main() {
	var opts options

	var rootCmd = &cobra.Command{
		Use:   "blockstore-add-agents",
		Short: "Add Disk Agents tool",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	markFlagRequired := func(flag string) {
		if err := rootCmd.MarkFlagRequired(flag); err != nil {
			log.Fatalf("can't mark flag %v as required: %v", flag, err)
		}
	}

	rootCmd.Flags().StringVar(
		&opts.Ticket,
		"ticket",
		"",
		"ticket for this operation",
	)

	rootCmd.Flags().StringVar(
		&opts.ServiceConfig,
		"service-config",
		"",
		"service config YAML file path",
	)
	markFlagRequired("service-config")

	rootCmd.Flags().StringVar(
		&opts.ClusterName,
		"cluster-name",
		"",
		"cluster name",
	)
	markFlagRequired("cluster-name")

	rootCmd.Flags().StringVar(
		&opts.ZoneName,
		"zone-name",
		"",
		"zone name",
	)
	markFlagRequired("zone-name")

	rootCmd.Flags().StringVarP(
		&opts.PatcherDir,
		"patcher-dir",
		"",
		func() string {
			arcadiaRoot, err := yatool.ArcadiaRoot()
			if err != nil {
				return ""
			}
			return path.Join(arcadiaRoot, "cloud", "blockstore", "tools", "cms", "patcher")
		}(),
		"path to patcher's dir",
	)

	rootCmd.Flags().StringVar(
		&opts.Hosts,
		"hosts",
		"",
		"hosts file path",
	)
	markFlagRequired("hosts")

	rootCmd.Flags().StringVarP(
		&opts.Secrets,
		"secrets",
		"s",
		func() string {
			path, _ := secutil.GetDefaultSecretsPath()
			return path
		}(),
		"secrets JSON file path",
	)

	rootCmd.Flags().StringVar(&opts.PsshPath, "pssh-path", "yc-pssh", "path to pssh binary")
	rootCmd.Flags().StringVar(&opts.YcpPath, "ycp-path", "ycp", "path to ycp binary")

	rootCmd.Flags().BoolVar(&opts.DryRun, "dry-run", false, "only generate configs")
	rootCmd.Flags().BoolVar(&opts.NoRestart, "no-restart", false, "do not restart nodes")
	rootCmd.Flags().BoolVar(&opts.NoUpdateLimit, "no-update-limit", false, "do not update NRD limit")
	rootCmd.Flags().BoolVar(&opts.SkipMutes, "skip-mutes", false, "do not setup mutes")
	rootCmd.Flags().BoolVar(&opts.AllowNamedConfigs, "allow-named-configs", false, "allow NamedConfigsItem")
	rootCmd.Flags().BoolVar(&opts.NoAuth, "no-auth", false, "don't use IAM token")
	rootCmd.Flags().BoolVarP(&opts.Yes, "yes", "y", false, "skip all confirmations")
	rootCmd.Flags().BoolVarP(&opts.Verbose, "verbose", "v", false, "verbose mode")

	rootCmd.Flags().IntVar(&opts.DeviceCount, "device-count", 0, "devices per agent")
	markFlagRequired("device-count")

	rootCmd.Flags().BoolVar(
		&opts.UseBlockDevices,
		"use-block-devices",
		false,
		"use block devices instead of partitions",
	)

	rootCmd.Flags().BoolVar(
		&opts.EnableRDMA,
		"enable-rdma",
		false,
		"enable RDMA server",
	)

	rootCmd.Flags().IntVarP(&opts.Parallelism, "parallelism", "p", 5, "like pssh -p")
	rootCmd.Flags().IntVarP(&opts.PsshAttempts, "pssh-attempts", "A", 3, "pssh attempts")

	rootCmd.Flags().DurationVar(&opts.RackRestartDelay, "rack-restart-delay", 10*time.Second, "delay between rack restarts")
	rootCmd.Flags().DurationVar(&opts.RestartDelay, "restart-delay", 5*time.Second, "delay between nbs restarts")

	userName, err := readUserFromPsshConfig(opts.PsshPath)
	if err != nil {
		log.Fatalf("can't get user name: %v", err)
	}

	hostname, err := os.Hostname()
	if err != nil {
		log.Fatalf("can't get hostname: %v", err)
	}

	opts.Cookie = fmt.Sprintf(
		"add_agents:%s@%v/%v",
		userName,
		hostname,
		opts.Ticket,
	)

	opts.UserName = userName

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("can't execute root command: %v", err)
	}
}
