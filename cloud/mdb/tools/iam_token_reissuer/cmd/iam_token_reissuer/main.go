package main

import (
	"context"
	"fmt"
	"io/ioutil"
	"os"

	"github.com/heetch/confita"
	"github.com/heetch/confita/backend/file"
	"github.com/spf13/pflag"

	"a.yandex-team.ru/cloud/mdb/internal/compute/iam"
	iamgrpc "a.yandex-team.ru/cloud/mdb/internal/compute/iam/grpc"
	crypto "a.yandex-team.ru/cloud/mdb/internal/crypto"
	"a.yandex-team.ru/cloud/mdb/internal/grpcutil"
	"a.yandex-team.ru/library/go/core/log"
	"a.yandex-team.ru/library/go/core/log/zap"
)

const appName = "token-reissuer"

type Config struct {
	IAMID             string `yaml:"iam_id"`
	IAMKeyID          string `yaml:"iam_key_id"`
	IAMPrivateKeyPath string `yaml:"iam_private_key_path"`

	DestinationPath string `yaml:"destination_path"`

	TSEndpoint string `yaml:"ts_endpoint"`

	caPath string `yanl:"ca_path"`

	grpc grpcutil.ClientConfig
}

type tokenReissuer struct {
	logFile string `yaml:"log_file"`
	//iam_id:

	cfg Config `yaml:"config"`

	l log.Logger
}

func (tr *tokenReissuer) Reissue() error {

	tr.l.Debugf("starting token reissue\n")

	ctx := context.Background()
	tokenService, err := iamgrpc.NewTokenServiceClient(ctx, tr.cfg.TSEndpoint, appName, tr.cfg.grpc, &grpcutil.PerRPCCredentialsStatic{}, tr.l)
	if err != nil {
		tr.l.Errorf("reissue: failed to get token service client: %s", err)
		return err
	}

	keyBin, err := ioutil.ReadFile(tr.cfg.IAMPrivateKeyPath)
	if err != nil {
		return err
	}

	prKey, err := crypto.NewPrivateKey(keyBin)
	if err != nil {
		return err
	}

	pemData, err := prKey.EncodeToPEM()
	if err != nil {
		return err
	}

	tok, err := tokenService.ServiceAccountToken(ctx, iam.ServiceAccount{
		ID:    tr.cfg.IAMID,
		KeyID: tr.cfg.IAMKeyID,
		Token: pemData,
	})
	if err != nil {
		tr.l.Errorf("reissue: failed to do request in token service: %v", err)
		return err
	}

	err = ioutil.WriteFile(tr.cfg.DestinationPath, []byte(tok.Value), 0600)
	if err != nil {
		tr.l.Errorf("reissue: failed to write jwt token to destination file %v", err)
		return err
	}

	return nil
}

var DefaultConfig = Config{
	TSEndpoint: "ts.private-api.cloud.yandex.net:4282", // some default endpoint for token service
	grpc: grpcutil.ClientConfig{
		Security: grpcutil.SecurityConfig{
			TLS: grpcutil.TLSConfig{},
		},
	},
}

// ReadFromFile reads config from file, performing all necessary checks
func ReadFromFile(configFile string) (Config, error) {
	config := DefaultConfig
	loader := confita.NewLoader(file.NewBackend(configFile))
	if err := loader.Load(context.Background(), &config); err != nil {
		err = fmt.Errorf("failed to load config from %s: %s", configFile, err.Error())
		return Config{}, err
	}

	return config, nil
}

func NewReissuer(cfgFile string) (*tokenReissuer, error) {
	logConfig := zap.JSONConfig(log.InfoLevel)
	logConfig.OutputPaths = []string{"stderr"}
	logger, err := zap.New(logConfig)
	if err != nil {
		return nil, err
	}

	tr := tokenReissuer{
		cfg: DefaultConfig,
		l:   logger,
	}

	cfg, err := ReadFromFile(cfgFile)
	if err != nil {
		return nil, err
	}

	tr.cfg = cfg

	return &tr, nil
}

var configPath string

const defaultConfigPath = "/root/conf.yaml"

func init() {
	flags := pflag.NewFlagSet("IAM token Reissuer", pflag.ExitOnError)
	flags.StringVarP(&configPath, "config", "c", defaultConfigPath, "Config path")

	pflag.CommandLine.AddFlagSet(flags)

	pflag.Usage = func() {
		fmt.Fprintf(os.Stderr,
			`Token_reissuer is simple jwt token manipulation tool.
Usage: %s --config=/path/to/conf.yaml
Running this will reissue jwt token and place it to "destination_path".      
`, os.Args[0])
		pflag.PrintDefaults()
	}
}

func main() {

	pflag.Parse()

	tr, err := NewReissuer(configPath)
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
	err = tr.Reissue()
	if err != nil {
		fmt.Println(err)
		os.Exit(1)
	}
}
