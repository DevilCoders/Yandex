package yaga

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/x509"
	"encoding/pem"
	"os"
	"path/filepath"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"

	"github.com/mitchellh/go-homedir"
	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	p := filepath.Join(filepath.Dir(config.DefaultConfigPath), KeyFilename)
	keyCmd.Flags().StringVar(&keypath, "out", p, "")

	yagaCmd.AddCommand(keyCmd)
}

const RSAKeyLength = 2048

var keyCmd = &cobra.Command{
	Use:     "key",
	Short:   "create rsa key, default location could be overrided with --out ~/path/to/key",
	Example: "$ mktctl yaga key --out /path/to/key",
	Args:    cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		k, err := rsa.GenerateKey(rand.Reader, RSAKeyLength)
		if err != nil {
			logger.FatalCtx(ctx, "generate rsa key", zap.Error(err))
		}

		p := pem.EncodeToMemory(
			&pem.Block{
				Type:  "RSA PRIVATE KEY",
				Bytes: x509.MarshalPKCS1PrivateKey(k),
			},
		)

		f, err := homedir.Expand(keypath)
		if err != nil {
			logger.FatalCtx(ctx, "expand filepath", zap.String("out", keypath), zap.Error(err))
		}

		err = os.WriteFile(f, p, 0700)
		if err != nil {
			logger.FatalCtx(ctx, "write private key", zap.String("out", f), zap.Error(err))
		}
	},
}
