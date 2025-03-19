package billing

import (
	"encoding/json"
	"fmt"
	"strconv"
)

type billingAccountManager interface {
	ResolveBillingAccounts(params ResolveBillingAccountsParams) ([]BillingAccount, error)
	ResolveBillingAccountByCloudIDFull(cloudID string, timeStamp int64) (*ExtendedBillingAccountCamelCaseView, error)
}

type BillingAccount struct {
	ID              string `json:"id"`
	MasterAccountID string `json:"masterAccountId"`
	Name            string `json:"name"`

	AvailablePaymentTypes []string `json:"availablePaymentTypes"`
	Balance               string   `json:"balance,omitempty"`
	BalanceClientID       string   `json:"balanceClientId,omitempty"`
	BalanceContractID     string   `json:"balanceContractId,omitempty"`
	BillingThreshold      string   `json:"billingThreshold,omitempty"`
	Country               string   `json:"country,omitempty"`
	CountryCode           string   `json:"countryCode,omitempty"`

	CreatedAt int64 `json:"createdAt"`
	UpdatedAt int64 `json:"updatedAt"`

	Currency      string          `json:"currency,omitempty"`
	DisplayStatus string          `json:"displayStatus,omitempty"`
	FeatureFlags  map[string]bool `json:"featureFlags,omitempty"`

	OwnerID string `json:"ownerID"`

	PaymentMethodID string `json:"paymentMethodId"`
	PaymentType     string `json:"paymentType"`

	PersonID   string `json:"personId"`
	PersonType string `json:"personType"`

	State string `json:"state"`
	Type  string `json:"type"`

	UsageStatus string `json:"usageStatus"`

	Metadata BillingAccountMetadata `json:"metadata"`

	PersonMeta []BillingAccountPersonMeta `json:"personMeta,omitempty"`

	BillingAccountPublicFlags
}

type BillingAccountMetadata struct {
	// # paysystem
	AutopayFailures int64 `json:"autopayFailures"`

	// # idempotency && grants issuing
	Verified          bool `json:"verified"`
	FloatingThreshold bool `json:"floatingThreshold"`
	IdempotencyChecks bool `json:"idempotencyChecks"`
	FraudDetectedBy   bool `json:"fraudDetectedBy"`

	// TODO: check API key.
	RegistrationIP          string   `json:"registrationIp"`
	RegistrationUserIAM     string   `json:"registrationUserIam"`
	BlockReason             string   `json:"blockReason"`
	UnblockReason           string   `json:"unblockReason"`
	AutoGrantPolicies       []string `json:"autoGrantPolicies,omitempty"`
	ISVSince                int64    `json:"isvSince"`
	VolumeIncentivePolicies []string `json:"volumeIncentivePolicies,omitempty"`
	PaidAt                  int64    `json:"paidAt"`
	BlockComment            string   `json:"blockComment"`
	BlockTicket             string   `json:"blockTicket"`

	SupportNotificationRule string `json:"supportNotificationRule"`

	// # var stuff
	VarPartnershipLevel int64 `json:"varPartnershipLevel"`

	// TODO
	// # online fraud detector
	// fraud_check = JsonSchemalessDictType()
}

type BillingAccountPersonMeta struct {
	Control string `json:"control"`
	ID      string `json:"id"`
	Title   string `json:"title"`
}

type BillingAccountPublicFlags struct {
	HasPartnerAccess             bool `json:"hasPartnerAccess,omitempty"`
	CommittedUseDiscountsAllowed bool `json:"isCommittedUseDiscountsAllowed,omitempty"`
	PaidSupportAllowed           bool `json:"isPaidSupportAllowed,omitempty"`

	IsReferral   bool `json:"isReferral,omitempty"`
	IsReferrer   bool `json:"isReferrer,omitempty"`
	IsSubaccount bool `json:"isSubaccount,omitempty"`
	IsVer        bool `json:"isVar,omitempty"`
}

type ResolveBillingAccountsParams struct {
	CloudID             string `json:"cloudId,omitempty"`
	FolderID            string `json:"folderId,omitempty"`
	PassportUUID        string `json:"passportUid,omitempty"`
	PassportLogin       string `json:"passportLogin,omitempty"`
	BalanceContractID   string `json:"balanceContractId,omitempty"`
	BalanceClientID     string `json:"balanceClientId,omitempty"`
	BillingAccountID    string `json:"billingAccountId,omitempty"`
	ServiceInstanceType string `json:"serviceInstanceType,omitempty"`
	ServiceInstanceID   string `json:"serviceInstanceId,omitempty"`
}

func (s *Session) ResolveBillingAccounts(params ResolveBillingAccountsParams) ([]BillingAccount, error) {
	var result struct {
		BillingAccounts []BillingAccount `json:"billingAccounts"`
	}

	response, err := s.ctxAuthRequest().
		SetBody(params).
		SetResult(&result).
		Post("/billing/v1/private/billingAccounts:resolve")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return result.BillingAccounts, nil
}

type SchemelessInt struct {
	Value string
}

func (s SchemelessInt) String() string {
	return string(s.Value)
}

func (s *SchemelessInt) UnmarshalJSON(rawBytes []byte) error {
	var abstract interface{}

	if err := json.Unmarshal(rawBytes, &abstract); err != nil {
		return err
	}

	if abstract == nil {
		return nil
	}

	switch v := abstract.(type) {
	case float64:
		s.Value = string(rawBytes)
	case string:
		s.Value = v
	default:
		return fmt.Errorf("unsupported type %T, with value %+v", v, v)
	}

	return nil
}

func (s *SchemelessInt) MarshalJSON() ([]byte, error) {
	return json.Marshal(s.Value)
}

type ExtendedBillingAccountView struct {
	ID string `json:"id,omitempty"`

	Balance          string `json:"balance,omitempty"`
	BillingThreshold string `json:"billing_threshold,omitempty"`
	CountryCode      string `json:"country_code,omitempty"`

	Currency         string `json:"currency,omitempty"`
	Name             string `json:"name,omitempty"`
	OwnerIamID       string `json:"owner_iam_id,omitempty"`
	OwnerID          string `json:"owner_id,omitempty"`
	PaymentCycleType string `json:"payment_cycle_type,omitempty"`
	PaymentType      string `json:"payment_type,omitempty"`
	State            string `json:"state,omitempty"`
	Type             string `json:"type,omitempty"`
	UsageStatus      string `json:"usage_status,omitempty"`

	CreatedAt int64 `json:"created_at,omitempty"`
	UpdatedAt int64 `json:"updated_at,omitempty"`

	PersonData struct {
		ID          string `json:"id,omitempty"`
		Partner     bool   `json:"is_partner,omitempty"`
		Type        string `json:"type,omitempty"`
		DisplayName string `json:"display_name,omitempty"`
	} `json:"person,omitempty"`

	BalanceContractResponse struct {
		BalanceClientID string `json:"balance_client_id,omitempty"`

		Spendable bool `json:"is_spendable,omitempty"`
		Suspended bool `json:"is_suspendable,omitempty"`
		Active    bool `json:"is_active,omitempty"`

		Services         []int  `json:"services,omitempty"`
		BillingAccountID string `json:"billing_account_id,omitempty"`
		PersonID         string `json:"person_id,omitempty"`
		PaymentType      string `json:"payment_type,omitempty"`
		PaymentCycleType string `json:"payment_cycle_type,omitempty"`

		Faxed       bool `json:"is_faxed,omitempty"`
		Signed      bool `json:"is_signed,omitempty"`
		Deactivated bool `json:"is_deactivated,omitempty"`
		Cancelled   bool `json:"is_cancelled,omitempty"`

		ID  string `json:"id,omitempty"`
		Vat string `json:"vat,omitempty"`

		ExternalID string `json:"external_id,omitempty"`

		// TODO: check date format, probably unsupported.
		EffectiveDate string `json:"effective_date,omitempty"`

		Currency    string `json:"currency,omitempty"`
		Type        string `json:"type,omitempty"`
		ManagerCode string `json:"manager_code,omitempty"`
	} `json:"contract,omitempty"`

	ExportsReportsAllowed bool `json:"is_export_reports_allowed,omitempty"`

	SalesManager struct {
		BillingAccountID string `json:"billing_account_id,omitempty"`
		Login            string `json:"login,omitempty"`
		WorkEmail        string `json:"work_email,omitempty"`
		FirstName        string `json:"first_name,omitempty"`
		LastName         string `json:"last_name,omitempty"`
		Position         string `json:"position,omitempty"`

		CreatedAt int64 `json:"createdAt,omitempty"`
		UpdatedAt int64 `json:"updatedAt,omitempty"`
	} `json:"sales_manager,omitempty"`

	Referrer struct {
		ReferrerID   string `json:"referrer_id,omitempty"`
		ReferrerName string `json:"referrer_name,omitempty"`
	} `json:"referrer,omitempty"`

	EmployeeGrantProgram struct {
		StaffLoging string `json:"staff_login,omitempty"`
		GrantID     string `json:"grant_id,omitempty"`
	} `json:"employee_grant_program,omitempty"`

	Passport struct {
		UID int64 `json:"uid,omitempty"`

		Login string `json:"login,omitempty"`
		Name  string `json:"name,omitempty"`
	} `json:"passport,omitempty"`

	Metadata struct {
		AutoGrantPolicies []string `json:"auto_grant_policies,omitempty"`
	} `json:"metadata,omitempty"`
}

type ExtendedBillingAccountCamelCaseView struct {
	ID string `json:"id,omitempty"`

	Balance          string `json:"balance,omitempty"`
	BillingThreshold string `json:"billingThreshold,omitempty"`

	AvailablePaymantTypes []string `json:"availablePaymentTypes"`

	Country     string `json:"country"`
	CountryCode string `json:"countryCode,omitempty"`

	Currency   string `json:"currency,omitempty"`
	Name       string `json:"name,omitempty"`
	OwnerIamID string `json:"owner_iam_id,omitempty"`
	OwnerID    string `json:"ownerId,omitempty"`

	PaymentCycleType string `json:"paymentCycleType,omitempty"`
	PaymentType      string `json:"paymentType,omitempty"`
	PaymentMethodID  string `json:"paymentMethodId"`

	State       string `json:"state,omitempty"`
	Type        string `json:"type,omitempty"`
	UsageStatus string `json:"usageStatus,omitempty"`

	CreatedAt int64 `json:"createdAt,omitempty"`
	UpdatedAt int64 `json:"updatedAt,omitempty"`

	PersonData struct {
		ID          string `json:"id,omitempty"`
		Partner     bool   `json:"isPartner,omitempty"`
		Type        string `json:"type,omitempty"`
		DisplayName string `json:"displayName,omitempty"`

		Individual *struct {
			UID string `json:"uid"`

			Email      string `json:"email"`
			FirstName  string `json:"firstName"`
			LastName   string `json:"lastName"`
			MiddleName string `json:"middleName"`

			INN   SchemelessInt `json:"inn"`
			SNILS string        `json:"pfr"`
			Phone string        `json:"phone"`

			PostAddress      string `json:"postAddress"`
			PostAddressSnake string `json:"post_address"`

			Partner bool `json:"isPartner,omitempty"`
		} `json:"individual,omitempty"`

		Company *struct {
			Name     string `json:"name"`
			LongName string `json:"longname"`

			Phone string `json:"phone"`
			Email string `json:"email"`

			PostCode      SchemelessInt `json:"postCode"`
			PostCodeSnake SchemelessInt `json:"post_code"`

			PostAddress      string `json:"postAddress"`
			PostAddressSnake string `json:"post_address"`

			LegalAddress      string `json:"legalAddress"`
			LegalAddressSnake string `json:"legal_address"`

			INN SchemelessInt `json:"inn"`

			// FIXME: String, Int ?
			KPP string `json:"kpp"`
			BIK string `json:"bik"`

			Account string `json:"account"`

			Partner bool `json:"isPartner,omitempty"`
		} `json:"company,omitempty"`

		Internal *struct {
			Name string `json:"name"`
		} `json:"internal,omitempty"`

		KAZIndividual *struct {
			FirstName string `json:"firstName"`
			LastName  string `json:"lastName"`

			Email string `json:"email"`
			Phone string `json:"phone"`

			PostAddress      string `json:"postAddress"`
			PostAddressSnake string `json:"post_address"`
		} `json:"kazakhstan_individual,omitempty"`

		KAZCompany *struct {
			Name string `json:"name"`

			Email string `json:"email"`
			Phone string `json:"phone"`
			City  string `json:"city"`

			LegalAddress string `json:"legalAddress"`
			PostAddress  string `json:"postAddress"`

			LegalAddressSnake string `json:"legal_Address"`
			PostAddressSnake  string `json:"post_address"`

			PostCode string `json:"postCode"`

			KBE   string `json:"kbe"`
			KZInn string `json:"kzInn"`
			IIK   string `json:"ikk"`
			BIK   string `json:"bik"`
		} `json:"kazakhstan_company,omitempty"`

		CHEResidentCompany *struct {
			Name     string `json:"name"`
			Phone    string `json:"phone"`
			Email    string `json:"email"`
			PostCode string `json:"postCode"`

			PostAddress      string `json:"postAddress"`
			PostAddressSnake string `json:"post_address"`
		} `json:"switzerland_resident_company"`

		CHENonResidentCompant *struct {
			Name     string `json:"name"`
			Phone    string `json:"phone"`
			Email    string `json:"email"`
			PostCode string `json:"postCode"`

			PostAddress      string `json:"postAddress"`
			PostAddressSnake string `json:"post_address"`
		} `json:"switzerland_nonresident_company"`

		VerifiedDocs bool `json:"verifiedDocs"`
	} `json:"person,omitempty"`

	Contract *struct {
		ID string `json:"id,omitempty"`

		ExternalID string `json:"externalId,omitempty"`
		Type       string `json:"type,omitempty"`
		Vat        string `json:"vat,omitempty"`

		BalanceClientID  string `json:"balanceClientId,omitempty"`
		BillingAccountID string `json:"billingAccountId,omitempty"`

		PersonID         string `json:"personId,omitempty"`
		PaymentType      string `json:"paymentType,omitempty"`
		PaymentCycleType string `json:"paymentCycleType,omitempty"`
		ManagerCode      string `json:"managerCode"`
		Currency         string `json:"currency,omitempty"`

		Services []int `json:"services,omitempty"`

		Active      bool `json:"isActive,omitempty"`
		Cancelled   bool `json:"isCancelled,omitempty"`
		Deactivated bool `json:"isDeactivated,omitempty"`
		Faxed       bool `json:"isFaxed,omitempty"`
		Signed      bool `json:"isSigned,omitempty"`
		Spendable   bool `json:"isSpendable,omitempty"`
		Suspended   bool `json:"isSuspendable,omitempty"`

		// TODO: check date format, probably unsupported.
		EffectiveDate string `json:"effectiveDate,omitempty"`
	} `json:"contract,omitempty"`

	ExportsReportsAllowed bool `json:"isExportReportsAllowed,omitempty"`

	SalesManager struct {
		BillingAccountID string `json:"billingAccountId,omitempty"`
		Login            string `json:"login,omitempty"`
		WorkEmail        string `json:"workEmail,omitempty"`
		FirstName        string `json:"firstName,omitempty"`
		LastName         string `json:"lastName,omitempty"`
		Position         string `json:"position,omitempty"`

		CreatedAt int64 `json:"createdAt,omitempty"`
		UpdatedAt int64 `json:"updatedAt,omitempty"`
	} `json:"sales_manager,omitempty"`

	Referrer struct {
		ReferrerID   string `json:"referrerId,omitempty"`
		ReferrerName string `json:"referrerName,omitempty"`
	} `json:"-"` // `json:"referrer,omitempty"`

	EmployeeGrantProgram struct {
		StaffLoging string `json:"staffLogin,omitempty"`
		GrantID     string `json:"grantId,omitempty"`
	} `json:"-"` // `json:"employeeGrantProgram,omitempty"`

	Passport struct {
		UID   int64  `json:"Uid,omitempty"`
		Login string `json:"Login,omitempty"`
		Name  string `json:"Name,omitempty"`
	} `json:"passport,omitempty"`

	PersonType string `json:"personType"`
	PersonID   string `json:"personID"`

	Metadata struct {
		AutoGrantPolicies []string `json:"autoGrantPolicies,omitempty"`
	} `json:"metadata,omitempty"`
}

type ViewType string

const (
	ShortViewType ViewType = "short"
	FullViewType  ViewType = "full"
)

func (v ViewType) String() string {
	return string(v)
}

// ResolveBillingAccountByCloudID
// NOTE: 'view' query parameter switch response keys case encoding:
//  "short" - "snake_case"
//  "full"  - "camel_case"
func (s *Session) ResolveBillingAccountByCloudIDFull(cloudID string, timeStamp int64) (*ExtendedBillingAccountCamelCaseView, error) {
	var result ExtendedBillingAccountCamelCaseView

	effectiveTime := strconv.FormatInt(timeStamp, 10)

	response, err := s.ctxAuthRequest().
		SetResult(&result).
		SetQueryParam("effective_time", effectiveTime).
		SetQueryParam("view", FullViewType.String()).
		SetPathParam("cloudID", cloudID).
		Get("/billing/v1/private/clouds/{cloudID}/resolveBillingAccount")

	if err := s.mapError(s.ctx, response, err); err != nil {
		return nil, err
	}

	return &result, nil
}
