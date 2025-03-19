package helpers

import (
	"context"
	"encoding/json"

	"gopkg.in/yaml.v2"

	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi"
	"a.yandex-team.ru/cloud/mdb/deploy/api/pkg/deployapi/restapi"
	"a.yandex-team.ru/cloud/mdb/internal/cli"
	"a.yandex-team.ru/cloud/mdb/internal/conductor"
	"a.yandex-team.ru/cloud/mdb/internal/conductor/httpapi"
	fqdn "a.yandex-team.ru/cloud/mdb/internal/fqdn/impl"
	"a.yandex-team.ru/cloud/mdb/internal/httputil"
	"a.yandex-team.ru/cloud/mdb/internal/tilde"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/config"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/federation"
	"a.yandex-team.ru/cloud/mdb/mdb-cli/pkg/common/token"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/xerrors"
)

const deployAPITokenURL = "https://oauth.yandex-team.ru/authorize?response_type=token&client_id=84ffe5ac974e4b8baa2cdab9fcac56bd"

func NewDeployAPI(env *cli.Env) deployapi.Client {
	cfg := config.FromEnv(env)
	iamCreds := federation.NewIamCredentials(env)
	caPath := cfg.CAPath
	if cfg.DeploymentConfig().CAPath != "" {
		caPath = cfg.DeploymentConfig().CAPath
	}
	caPath, err := tilde.Expand(caPath)
	if err != nil {
		env.L().Fatalf("failed to get CA path: %s", err)
	}
	if cfg.DeployAPIToken == "" {
		t, err := token.Get(env, token.Description{
			URL:  deployAPITokenURL,
			Name: "Deploy API",
		})
		if err != nil {
			env.L().Fatalf("failed to get Deploy token: %s", err)
		}
		cfg.DeployAPIToken = t
		if err = config.WriteConfig(cfg, env.GetConfigPath()); err != nil {
			env.L().Fatalf("failed to write config: %s", err)
		}
		env.RootCmd.Cmd.Printf("Write updated config to %s\n", env.GetConfigPath())
	}
	dapi, err := restapi.New(
		cfg.DeploymentConfig().DeployAPIURL,
		cfg.DeployAPIToken,
		iamCreds,
		httputil.TLSConfig{CAFile: caPath},
		httputil.LoggingConfig{LogRequestBody: cfg.LogHTTPBody, LogResponseBody: cfg.LogHTTPBody},
		env.L(),
	)
	if err != nil {
		env.L().Fatalf("failed to initialize deploy api client: %s", err)
	}

	return dapi
}

func NewConductorAPI(l log.Logger) conductor.Client {
	cfg := httpapi.DefaultConductorConfig()
	capi, err := httpapi.New(cfg, l)
	if err != nil {
		l.Fatalf("failed to initialize conductor api client: %s", err)
	}

	return capi
}

func ParseMultiTargets(ctx context.Context, args []string, l log.Logger) []string {
	capi := NewConductorAPI(l)
	targets, err := conductor.ParseMultiTargets(ctx, capi, args...)
	if err != nil {
		l.Fatal(err.Error())
	}

	l.Debugf("Parsed targets: %+v", targets)

	fqdns := conductor.CollapseTargets(targets)
	if len(fqdns) == 0 {
		l.Fatalf("No targets left after collapsing")
	}

	l.Debugf("Collapsed targets: %+v", fqdns)
	return fqdns
}

// MarshalableError enables marshaling of error values to JSON/YAML
type MarshalableError struct {
	error
}

var (
	_ json.Marshaler = &MarshalableError{}
	_ yaml.Marshaler = &MarshalableError{}
)

func (me MarshalableError) MarshalJSON() ([]byte, error) {
	return json.Marshal(me.Error())
}

func (me MarshalableError) MarshalYAML() (interface{}, error) {
	return yaml.Marshal(me.Error())
}

type TargetError struct {
	Target string           `json:",omitempty" yaml:",omitempty"`
	Err    MarshalableError `json:",omitempty" yaml:",omitempty"`
}

type MultiTargetResult struct {
	Done  []string      `json:",omitempty" yaml:",omitempty"`
	Noop  []string      `json:",omitempty" yaml:",omitempty"`
	Error []TargetError `json:",omitempty" yaml:",omitempty"`
}

func RunMultiTarget(ctx context.Context, targets []string, f func(target string) (bool, error)) MultiTargetResult {
	var res MultiTargetResult
	for _, target := range targets {
		ok, err := f(target)
		if err != nil {
			res.Error = append(res.Error, TargetError{Target: target, Err: MarshalableError{error: err}})
			continue
		}

		if !ok {
			res.Noop = append(res.Noop, target)
			continue
		}

		res.Done = append(res.Done, target)
	}

	return res
}

func OutputMultiTargetResult(env *cli.Env, res MultiTargetResult) {
	m, err := env.OutMarshaller.Marshal(res)
	if err != nil {
		env.Logger.Fatalf("failed to marshal result %+v: %s", res, err)
	}

	env.Logger.Info(string(m))
}

func ManagedFQDNsToUnamanged(fqdns []string, dc config.DeploymentConfig, l log.Logger) []string {
	res, err := managedFQDNsToUnamanged(fqdns, dc, l)
	if err != nil {
		l.Fatal(err.Error())
	}

	return res
}

func managedFQDNsToUnamanged(fqdns []string, dc config.DeploymentConfig, l log.Logger) ([]string, error) {
	if dc.ManagedDataplaneFQDNSuffix == "" &&
		dc.UnamangedDataplaneFQDNSuffix == "" {
		l.Debug("Deployment config does not have dataplane fqdn suffixes - not converting fqdns")
		return fqdns, nil
	}

	converter := fqdn.NewConverter(dc.ControlplaneFQDNSuffix, dc.UnamangedDataplaneFQDNSuffix, dc.ManagedDataplaneFQDNSuffix)
	res := make([]string, 0, len(fqdns))
	for _, f := range fqdns {
		r, err := converter.ManagedToUnmanaged(f)
		if err != nil {
			return nil, xerrors.Errorf("failed to convert fqdn %q: %w", f, err)
		}

		res = append(res, r)
	}

	l.Debugf("Targets after fqdn conversion: %+v", res)
	return res, nil
}
