package yaga

import (
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"fmt"
	"math/big"
	"os"
	"path/filepath"
	"time"

	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/mitchellh/go-homedir"
	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	p := filepath.Join(filepath.Dir(config.DefaultConfigPath), KeyFilename)
	resetCmd.Flags().StringVar(&keypath, "key", p, fmt.Sprintf("by default is: %v", p))
	resetCmd.Flags().StringVar(&username, "username", "Administrator", "by default is Administrator")
	resetCmd.Flags().StringVar(&instanceID, "instance-id", "", "")

	_ = resetCmd.MarkFlagRequired("instance-id")

	yagaCmd.AddCommand(resetCmd)
}

var resetCmd = &cobra.Command{
	Use:  "reset",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()

		p, err := homedir.Expand(keypath)
		if err != nil {
			logger.FatalCtx(ctx, "expand filepath", zap.String("keypath", keypath), zap.Error(err))
		}

		t, err := os.Stat(p)
		if err != nil {
			logger.FatalCtx(ctx, "failed to test filepath",
				zap.Error(err),
				zap.String("filepath", p))
		}
		if t.IsDir() {
			logger.FatalCtx(ctx, "dir, not a file",
				zap.Error(err),
				zap.String("filepath", p))
		}

		f, err := os.ReadFile(p)
		if err != nil {
			logger.FatalCtx(ctx, "read key file", zap.Error(err), zap.String("filepath", keypath))
		}

		block, _ := pem.Decode(f)
		if block == nil || block.Type != "RSA PRIVATE KEY" {
			logger.FatalCtx(ctx, "failed to decode PEM block containing rsa private key", zap.Error(err))
		}

		pk, err := x509.ParsePKCS1PrivateKey(block.Bytes)
		if err != nil {
			logger.FatalCtx(ctx, "parse pem key", zap.Error(err))
		}

		m := base64.URLEncoding.
			EncodeToString(pk.PublicKey.N.Bytes())
		e := base64.URLEncoding.
			EncodeToString(big.NewInt(int64(pk.PublicKey.E)).Bytes())
		exp := time.Now().Unix()

		req := struct {
			Expires  int64  `json:"expires"`
			Username string `json:"username"`
			Exponent string `json:"exponent"`
			Modulus  string `json:"modulus"`
		}{
			Expires:  exp,
			Username: username,
			Exponent: e,
			Modulus:  m,
		}

		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplaceConsoleEndpoint())
		resp, err := mkt.NewRequest().
			SetBody(req).
			SetPathParam("instance_id", instanceID).
			Post("/yaga/{instance_id}/passwordReset")
		if err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}

		err = pretty.Print(resp.Body(), cfg.Format)
		if err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}
	},
}
