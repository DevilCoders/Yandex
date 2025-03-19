package yaga

import (
	"crypto/rand"
	"crypto/rsa"
	"crypto/sha256"
	"crypto/x509"
	"encoding/base64"
	"encoding/pem"
	"fmt"
	"os"
	"path/filepath"

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
	getCmd.Flags().StringVar(&keypath, "key", p, fmt.Sprintf("by default is: %v", p))
	getCmd.Flags().StringVar(&instanceID, "instance-id", "", "")
	getCmd.Flags().StringVar(&requestID, "request-id", "", "sent in response to reset")

	_ = getCmd.MarkFlagRequired("instance-id")
	_ = getCmd.MarkFlagRequired("request-id")
	yagaCmd.AddCommand(getCmd)
}

var getCmd = &cobra.Command{
	Use:  "get",
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
				zap.String("path", p))
		}
		if t.IsDir() {
			logger.FatalCtx(ctx, "dir, not a file",
				zap.Error(err),
				zap.String("path", p))
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

		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplaceConsoleEndpoint())

		var res struct {
			Result struct {
				Error             string `json:"error"`
				Exponent          string `json:"exponent"`
				Username          string `json:"username"`
				EncryptedPassword string `json:"encryptedPassword"`
				Success           bool   `json:"success"`
				Modulus           string `json:"modulus"`
			} `json:"result"`
		}

		resp, err := mkt.NewRequest().
			SetPathParam("instance_id", instanceID).
			SetQueryParam("requestId", requestID).
			SetResult(&res).
			Get("/yaga/{instance_id}/passwordReset")
		if err != nil {
			logger.FatalCtx(ctx, "get", zap.Error(err))
		}

		err = pretty.Print(resp.Body(), cfg.Format)
		if err != nil {
			logger.FatalCtx(ctx, "failed to pretty print", zap.Error(err))
		}

		encBytes, err := base64.StdEncoding.
			DecodeString(res.Result.EncryptedPassword)
		if err != nil {
			logger.FatalCtx(ctx, "failed to decode", zap.Error(err))
		}

		decBytes, err := rsa.DecryptOAEP(sha256.New(), rand.Reader, pk, encBytes, nil)
		if err != nil {
			logger.FatalCtx(ctx, "failed to decrypt", zap.Error(err))
		}

		fmt.Printf("\npassword is: %v\n", string(decBytes))
	},
}
