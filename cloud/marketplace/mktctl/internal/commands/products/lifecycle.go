package products

import (
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/config"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/logger"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/marketplace"
	"a.yandex-team.ru/cloud/marketplace/mktctl/internal/pretty"

	"github.com/spf13/cobra"
	"go.uber.org/zap"
)

func init() {
	lifecycle.Flags().StringVar(&productID, "product-id", "", "")
	_ = lifecycle.MarkFlagRequired("product-id")

	lifecycle.Flags().BoolVar(&autoApprove, "auto-approve", false, "")
	lifecycle.Flags().BoolVar(&autoApply, "auto-apply", false, "")
	lifecycle.Flags().BoolVar(&validBaseImage, "valid-base-image", false, "make image availible to build product from")

	productsCmd.AddCommand(lifecycle)
}

type lifecycleParamsRequest struct {
	LifecycleParams struct {
		AutoApprove   bool `json:"autoApprove"`
		AutoApply     bool `json:"autoApply"`
		PayloadParams struct {
			ComputeImage struct {
				ValidBaseImage bool `json:"validBaseImage"`
			} `json:"computeImage"`
		} `json:"payloadParams"`
	} `json:"lifecycleParams"`
}

var lifecycle = &cobra.Command{
	Use:  "lifecycle",
	Args: cobra.NoArgs,
	Run: func(cmd *cobra.Command, args []string) {

		ctx := cmd.Context()
		cfg := config.FromContext(ctx)
		mkt := marketplace.NewClient(cfg.GetToken(), cfg.GetMarketplacePrivateEndpoint())

		r := &lifecycleParamsRequest{}
		r.LifecycleParams.AutoApply = autoApply
		r.LifecycleParams.AutoApprove = autoApprove
		r.LifecycleParams.PayloadParams.ComputeImage.ValidBaseImage = validBaseImage

		resp, err := mkt.NewRequest().
			SetBody(r).
			SetPathParam("publisher_id", publisherID).
			SetPathParam("product_id", productID).
			Post("/publishers/{publisher_id}/products/{product_id}:setLifecycle")
		if err != nil {
			logger.FatalCtx(ctx, "lifecycle", zap.Error(err))
		}

		if err = pretty.Print(resp.Body(), cfg.Format); err != nil {
			logger.FatalCtx(ctx, "print", zap.Error(err))
		}
	},
}
