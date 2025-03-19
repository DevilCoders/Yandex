package main

import (
	"bufio"
	"context"
	_ "embed"
	"fmt"
	"io/ioutil"
	"log"
	"os"
	"os/user"
	"strings"
	"text/template"
	"time"

	"github.com/spf13/cobra"
	"gopkg.in/yaml.v2"

	nbs "a.yandex-team.ru/cloud/blockstore/public/sdk/go/client"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/infra"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/juggler"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/messenger"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/pssh"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/release"
	secutil "a.yandex-team.ru/cloud/storage/core/tools/common/go/secrets"
	st "a.yandex-team.ru/cloud/storage/core/tools/common/go/startrack"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/walle"
	"a.yandex-team.ru/cloud/storage/core/tools/common/go/z2"
)

////////////////////////////////////////////////////////////////////////////////

//go:embed telegram.users
var telegramKnownUsers string

//go:embed qmssngr.users
var qmssngrKnownUsers string

const yavSecretID = "sec-01f8w546n277r3fbnqvs1hkhnn"

////////////////////////////////////////////////////////////////////////////////

type packageVersions map[string]string

func (pv *packageVersions) String() string {
	return ""
}

func (pv *packageVersions) Type() string {
	return "package-name=version"
}

func (pv *packageVersions) Set(value string) error {
	parts := strings.Split(value, "=")

	if len(parts) != 2 {
		return fmt.Errorf("unexpected value: %v", value)
	}

	if len(*pv) == 0 {
		*pv = make(packageVersions)
	}

	name := parts[0]
	version := parts[1]

	(*pv)[name] = version

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type hostsFilter map[string]bool

func (hf *hostsFilter) String() string {
	return ""
}

func (hf *hostsFilter) Type() string {
	return "path|list"
}

func (hf *hostsFilter) Set(value string) error {
	*hf = make(hostsFilter)

	if file, err := os.Open(value); err == nil {
		defer file.Close()

		scanner := bufio.NewScanner(file)
		for scanner.Scan() {
			(*hf)[scanner.Text()] = true
		}

		if err := scanner.Err(); err != nil {
			return err
		}
	} else {
		// assume it's comma separated list of hosts
		for _, host := range strings.Split(value, ",") {
			(*hf)[host] = true
		}
	}

	return nil
}

////////////////////////////////////////////////////////////////////////////////

type options struct {
	release.Options

	Verbose         bool
	NoAuth          bool
	UseRobotAccount bool

	Username      string
	PsshPath      string
	YcpPath       string
	Secrets       string
	ServiceConfig string
	Messenger     string
	YavToken      string

	versions     packageVersions
	prevVersions packageVersions
	filter       hostsFilter
}

////////////////////////////////////////////////////////////////////////////////

func loadServiceConfig(path string) (*release.ServiceConfig, error) {
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

	config := &release.ServiceConfig{}
	if err := yaml.Unmarshal([]byte(configYAML.String()), config); err != nil {
		return nil, fmt.Errorf("can't unmarshal service config: %w", err)
	}

	return config, nil
}

////////////////////////////////////////////////////////////////////////////////

func ApplyYavSecrets(
	secrets *secutil.Secrets,
	yavSecrets secutil.YavSecrets,
) *secutil.Secrets {

	output := secrets
	if output == nil {
		output = &secutil.Secrets{}
	}

	for k, v := range yavSecrets {
		switch k {
		case "oauth_token":
			output.OAuthToken = v
		case "telegram_token":
			output.TelegramToken = v
		case "z2_api_key":
			output.Z2ApiKey = v
		case "bot_token":
			output.BotOAuthToken = v
		}
	}
	return output
}

////////////////////////////////////////////////////////////////////////////////

func run(ctx context.Context, opts *options) error {
	opts.Options.PackageVersions = release.PackageVersions(
		opts.versions,
	)

	opts.Options.PrevPackageVersions = release.PackageVersions(
		opts.prevVersions,
	)

	opts.Options.HostsFilter = release.HostsFilter(
		opts.filter,
	)

	config, err := loadServiceConfig(opts.ServiceConfig)
	if err != nil {
		return err
	}

	var secrets *secutil.Secrets
	if _, err = os.Stat(opts.Secrets); err == nil {
		secrets, err = secutil.LoadFromFile(opts.Secrets)
		if err != nil {
			return err
		}
	}

	if len(opts.YavToken) == 0 {
		yavToken, yavTokenIsSet := os.LookupEnv("YAV_TOKEN")
		if yavTokenIsSet {
			opts.YavToken = yavToken
		}
	}
	if len(opts.YavToken) != 0 {
		yavSecrets, err := secutil.ReadSecretsFromYav(yavSecretID, opts.YavToken)
		if err != nil {
			return err
		}
		secrets = ApplyYavSecrets(secrets, yavSecrets)
	}

	logLevel := nbs.LOG_ERROR
	if opts.Verbose {
		logLevel = nbs.LOG_DEBUG
	}

	replacer := secutil.NewReplacer(secrets, "")

	logger := nbs.NewLog(
		log.New(
			secutil.NewFilter(os.Stderr, replacer),
			"",
			log.Ltime,
		),
		logLevel,
	)

	var stClient st.StarTrackClientIface

	if len(opts.Ticket) != 0 {
		stClient = st.NewClient(logger, opts.Ticket, secrets.OAuthToken, replacer)
	}

	var mssngr messenger.MessengerClientIface

	if opts.Messenger == messenger.QMssngrID {
		mssngr = messenger.NewQMssngrClient(
			logger,
			secrets.BotOAuthToken,
			replacer,
			qmssngrKnownUsers,
			"qmssngr.users",
		)
	} else {
		mssngr = messenger.NewTelegramClient(
			logger,
			secrets.TelegramToken,
			replacer,
			telegramKnownUsers,
			"telegram.users",
		)
	}

	var yubikeyAction pssh.YubikeyAction

	chatID := mssngr.GetUserChatID(opts.Username)
	if len(chatID) != 0 {
		yubikeyAction = func(ctx context.Context) error {
			return mssngr.SendMessage(
				ctx,
				chatID,
				"Please touch yubikey",
			)
		}
	} else {
		fmt.Fprintln(os.Stderr, "Can't get messenger chat id. See ", mssngr.GetUsersFile())
	}

	var psshParams []string
	if opts.UseRobotAccount {
		psshParams = []string{"--no-yubikey", "--user", "robot-yc-nbs-oncall", "--bastion-user", "robot-yc-nbs-oncall"}
	}

	r := release.NewRelease(
		logger,
		&opts.Options,
		config,
		infra.NewClient(logger, opts.Ticket, secrets.OAuthToken),
		juggler.NewClient(logger, opts.Ticket, secrets.OAuthToken),
		pssh.NewDurable(
			logger,
			pssh.NewWithParams(logger, opts.PsshPath, psshParams, yubikeyAction),
			opts.PsshAttempts,
		),
		stClient,
		mssngr,
		walle.NewClient(logger),
		z2.NewClient(logger, secrets.Z2ApiKey),
	)
	return r.Run(ctx)
}

////////////////////////////////////////////////////////////////////////////////

func main() {
	var opts options

	var rootCmd = &cobra.Command{
		Use:   "release",
		Short: "Release tool",
		Run: func(cmd *cobra.Command, args []string) {
			ctx, cancelCtx := context.WithCancel(context.Background())
			defer cancelCtx()

			if err := run(ctx, &opts); err != nil {
				log.Fatalf("Error: %v", err)
			}
		},
	}

	rootCmd.Flags().StringVarP(
		&opts.Description,
		"description",
		"d",
		"",
		"release description, e.g. 20-6-8",
	)

	rootCmd.Flags().StringVarP(
		&opts.ServiceConfig,
		"service-config",
		"c",
		"",
		"service config YAML file path",
	)

	rootCmd.Flags().StringVar(
		&opts.ClusterName,
		"cluster-name",
		"",
		"cluster name for deploy release, e.g. prod",
	)

	rootCmd.Flags().StringVar(
		&opts.ZoneName,
		"zone-name",
		"",
		"zone name for deploy release, e.g. sas",
	)

	rootCmd.Flags().StringVar(
		&opts.TargetName,
		"target-name",
		"",
		"target name for deploy release, e.g. nbs-control",
	)

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

	rootCmd.Flags().StringVarP(
		&opts.Username,
		"user",
		"u",
		func() string {
			u, _ := user.Current()
			return u.Username
		}(),
		"user name",
	)

	rootCmd.Flags().BoolVar(&opts.NoAuth, "no-auth", false, "don't use IAM token")
	rootCmd.Flags().BoolVarP(&opts.Verbose, "verbose", "v", false, "verbose mode")
	rootCmd.Flags().BoolVarP(&opts.Yes, "yes", "y", false, "skip all confirmations")
	rootCmd.Flags().BoolVarP(&opts.NoRestart, "no-restart", "n", false, "do not restart nodes")
	rootCmd.Flags().BoolVarP(
		&opts.UpdateMetaGroups,
		"update-meta-groups",
		"U",
		false,
		"run collective update for meta groups",
	)
	rootCmd.Flags().BoolVarP(
		&opts.SkipInfraAndMutes,
		"skip-infra-and-mutes",
		"S",
		false,
		"go directly to the update process",
	)
	rootCmd.Flags().BoolVarP(&opts.Force, "force", "f", false, "z2 force update")

	rootCmd.Flags().StringVar(
		&opts.CommonPackageVersion,
		"package-version",
		"",
		"binary package version for this release",
	)
	rootCmd.Flags().Var(
		&opts.versions,
		"package-versions",
		"overrides package version",
	)
	rootCmd.Flags().StringVar(
		&opts.ConfigVersion,
		"config-version",
		"",
		"config package version for this release",
	)
	rootCmd.Flags().StringVar(
		&opts.PrevCommonPackageVersion,
		"prev-package-version",
		"",
		"binary package version for previous release",
	)
	rootCmd.Flags().Var(
		&opts.prevVersions,
		"prev-package-versions",
		"overrides previous package version",
	)
	rootCmd.Flags().StringVar(
		&opts.PrevConfigVersion,
		"prev-config-version",
		"",
		"config package version for previous release",
	)
	rootCmd.Flags().StringVarP(&opts.Ticket, "ticket", "T", "", "ticket for this release")
	rootCmd.Flags().IntVarP(&opts.Parallelism, "parallelism", "p", 10, "like pssh -p")
	rootCmd.Flags().IntVarP(&opts.PsshAttempts, "pssh-attempts", "A", 3, "pssh attempts")
	rootCmd.Flags().IntVarP(
		&opts.MaxRestartProblems,
		"max-restart-problems",
		"R",
		3,
		"if restart error count gets bigger than this value, the release is aborted",
	)
	rootCmd.Flags().IntVarP(&opts.Z2Attempts, "z2-attempts", "Z", 10, "z2 attempts")
	rootCmd.Flags().DurationVar(&opts.Z2Backoff, "z2-backoff", 5*time.Second, "z2 initial timeout between attempts")

	rootCmd.Flags().StringVar(&opts.PsshPath, "pssh-path", "yc-pssh", "path to pssh binary")
	rootCmd.Flags().StringVar(&opts.YcpPath, "ycp-path", "ycp", "path to ycp binary")

	rootCmd.Flags().StringVar(
		&opts.Messenger,
		"messenger",
		messenger.QMssngrID,
		fmt.Sprintf("messenger to use (%v, %v)", messenger.TelegramID, messenger.QMssngrID))

	rootCmd.Flags().BoolVarP(&opts.UseRobotAccount, "use-robot", "r", false, "release is done under robot account")
	rootCmd.Flags().StringVarP(&opts.YavToken, "yav-token", "", "", "token to read secrets from yav")

	rootCmd.Flags().Var(
		&opts.filter,
		"hosts-filter",
		"either file or comma-separated hosts list")

	requiredFlags := []string{
		"cluster-name",
		"description",
		"service-config",
		"zone-name",
	}

	for _, flag := range requiredFlags {
		if err := rootCmd.MarkFlagRequired(flag); err != nil {
			log.Fatalf("can't mark flag %v as required: %v", flag, err)
		}
	}

	if err := rootCmd.Execute(); err != nil {
		log.Fatalf("can't execute root command: %v", err)
	}
}
