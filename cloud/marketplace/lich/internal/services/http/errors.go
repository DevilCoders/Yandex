package http

import (
	"fmt"
	"net/http"
	"strings"

	"google.golang.org/grpc/codes"
)

type apiError struct {
	status int

	Code    codes.Code `json:"code"`
	Message string     `json:"message"`

	Type string `json:"type"`

	i18nPrefix string
}

var errCodes = map[string]codes.Code{
	"":                                   codes.Unknown,
	"RequestValidationError":             codes.InvalidArgument,
	"BAD_CLOUD_ID":                       codes.NotFound,
	"LICENSE_CHECK":                      codes.PermissionDenied,
	"LICENSE_CHECK_EXTERNAL":             codes.PermissionDenied,
	"LICENSED_INSTANCE_POOL_VALUE_ERROR": codes.PermissionDenied,
	"AuthorizationError":                 codes.PermissionDenied,
	"AuthFailure":                        codes.Unauthenticated,
	"INTERNAL_ERROR":                     codes.Internal,
	"DOES_NOT_EXIST":                     codes.NotFound,
}

func newAPIError(errType string, status int, format string, args ...interface{}) *apiError {
	return &apiError{
		status: status,

		Code:    errCodes[errType],
		Message: fmt.Sprintf(format, args...),

		Type: errType,

		i18nPrefix: "api.errors.entities.license",
	}
}

func newErrCloudIDNotFound(cloudID string) *apiError {
	return newAPIError("DOES_NOT_EXIST", http.StatusNotFound, fmt.Sprintf("Cloud '%s' was not found", cloudID))
}

func newErrLicenseCheckExternal(cloudID string, productsIds []string, messages []string) *apiError {
	var idsStr []string
	for i := range productsIds {
		idsStr = append(idsStr, fmt.Sprintf("'%s'", productsIds[i]))
	}

	var b strings.Builder
	fmt.Fprintf(&b, "Product license prohibits usage of product(s) [%s] within cloud %s",
		strings.Join(idsStr, ", "),
		cloudID,
	)

	if len(messages) != 0 {
		fmt.Fprintf(&b, ". Details: %s", strings.Join(messages, ". "))
	}

	return newAPIError("LICENSE_CHECK_EXTERNAL", http.StatusForbidden, b.String())
}

func (e *apiError) Error() string {
	return fmt.Sprintf("code: %d, message: %s", e.Code, e.Message)
}

var (
	errLicensedInstancePoolValue = newAPIError(
		"LICENSED_INSTANCE_POOL_VALUE_ERROR",
		http.StatusForbidden,
		"Licensed instance pool collision",
	)

	errAPIAuthFailure    = newAPIError("AuthFailure", http.StatusUnauthorized, "Unauthenticated.")
	errAPIUnauthorized   = newAPIError("AuthorizationError", http.StatusForbidden, "Unauthorized")
	errEmptyProductsIDs  = newAPIError("RequestValidationError", http.StatusBadRequest, "productIds: This field is required")
	errLicenseCheck      = newAPIError("LICENSE_CHECK", http.StatusForbidden, "Product license prohibits to use the product within this cloud")
	errNoCloudID         = newAPIError("RequestValidationError", http.StatusBadRequest, "cloudId: Invalid resource ID")
	errRequestValidation = newAPIError("RequestValidationError", http.StatusBadRequest, "Request validation error: The specified request data is not a valid JSON object.")

	errInternal       = newAPIError("INTERNAL_ERROR", http.StatusInternalServerError, "Internal service error")
	errBackendTimeout = newAPIError("INTERNAL_ERROR", http.StatusGatewayTimeout, "Internal service error")
)
