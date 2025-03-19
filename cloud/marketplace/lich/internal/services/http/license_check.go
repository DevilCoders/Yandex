package http

import (
	"encoding/json"
	"fmt"
	"net/http"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/actions"
	"a.yandex-team.ru/cloud/marketplace/pkg/logging"
)

func (s *Service) licenseCheckHandler(w http.ResponseWriter, r *http.Request) error {
	logging.Logger().Debug("http: starting license check handler")

	request := struct {
		CloudID    string   `json:"cloud_id"`
		ProductIDs []string `json:"product_ids"`
	}{}

	if err := json.NewDecoder(r.Body).Decode(&request); err != nil {
		return fmt.Errorf("failed to process request body: %s, %w", err, errRequestValidation)
	}

	params := actions.LicenseCheckParams{
		CloudID:     request.CloudID,
		ProductsIDs: request.ProductIDs,
	}

	response, err := actions.NewLicenseCheckAction(s.Env).Do(r.Context(), params)
	if err != nil {
		return err
	}

	result := struct {
		LicensedInstancePool string `json:"licensedInstancePool,omitempty"`
	}{
		LicensedInstancePool: response.LicensedInstancePool,
	}

	logging.Logger().Debug("http: license check completed")

	return s.sendJSONResponseOk(r.Context(), w, result)
}
