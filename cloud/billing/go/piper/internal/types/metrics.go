package types

import (
	"encoding/json"
	"fmt"

	"github.com/mailru/easyjson"

	"a.yandex-team.ru/cloud/billing/go/pkg/decimal"
)

//go:generate easyjson -disable_members_unescape -disallow_unknown_fields .

type commonMetricFields struct {
	ID     string `json:"id"`
	Schema string `json:"schema"`

	CloudID     string `json:"cloud_id"`
	FolderID    string `json:"folder_id"`
	AbcID       int64  `json:"abc_id"`
	AbcFolderID string `json:"abc_folder_id"`

	BillingAccountID string        `json:"billing_account_id"`
	Usage            MetricUsage   `json:"usage"`
	Tags             JSONAnything  `json:"tags"`
	SkuID            string        `json:"sku_id"`
	ResourceID       string        `json:"resource_id"`
	SourceWriteTime  JSONTimestamp `json:"source_wt"`
	SourceID         string        `json:"source_id"`
	Version          string        `json:"version"`
}

//easyjson:json
type SourceMetric struct {
	commonMetricFields
	UserLabels JSONLabels `json:"labels"`
}

//easyjson:json
type ReshardedQueueMetric struct {
	commonMetricFields

	IsExpired  bool   `json:"is_expired"`
	SequenceID uint64 `json:"sequence_id"`

	ReshardingKey string        `json:"resharding_key"`
	UsageTime     JSONTimestamp `json:"usage_time"`

	UserLabels          map[string]string `json:"labels"`
	IsUserLabelsAllowed bool              `json:"is_user_labels_allowed"`

	MessageWriteTS JSONTimestamp `json:"message_write_ts"`

	PricingQuantity JSONDecimal   `json:"pricing_quantity"`
	PricingUnit     string        `json:"pricing_unit"`
	SkuName         string        `json:"sku_name"`
	StartTime       JSONTimestamp `json:"start_time"`
	EndTime         JSONTimestamp `json:"end_time"`
	MasterAccountID string        `json:"master_account_id"`
}

type MetricUsage struct {
	Quantity decimal.Decimal128 `json:"quantity"`
	Type     string             `json:"type"`
	Unit     string             `json:"unit"`
	Start    JSONTimestamp      `json:"start"`
	Finish   JSONTimestamp      `json:"finish"`
}

type MetricLabels struct {
	// NOTE: Sometimes in the past labels were list.
	// if this data still should be processed than add UnmarshalJSON with skip list value
	User   map[string]JSONAnyString `json:"user_labels"`
	System map[string]JSONAnyString `json:"system_labels"` // TODO: only 1 field in common, maybe struct
}

//easyjson:json
type QueueError struct {
	SequenceID       uint64        `json:"sequence_id"`
	Reason           string        `json:"reason"`
	MetricSourceID   string        `json:"metric_source_id"`
	SourceID         string        `json:"source_id"`
	Hostname         string        `json:"hostname"`
	ReasonComment    string        `json:"reason_comment"`
	MetricResourceID string        `json:"metric_resource_id"`
	SourceName       string        `json:"source_name"`
	MetricSchema     string        `json:"metric_schema"`
	Metric           string        `json:"metric"`
	UploadedAt       JSONTimestamp `json:"uploaded_at"`
	MetricID         string        `json:"metric_id"`
	RawMetric        string        `json:"raw_metric"`
}

//easyjson:json
type EnrichedQueueMetric struct {
	commonMetricFields

	IsExpired  bool   `json:"is_expired"`
	SequenceID uint64 `json:"sequence_id"`

	ReshardingKey string        `json:"resharding_key"`
	UsageTime     JSONTimestamp `json:"usage_time"`

	Labels              MetricLabels `json:"labels"`
	IsUserLabelsAllowed bool         `json:"is_user_labels_allowed"`

	MessageWriteTS JSONTimestamp `json:"message_write_ts"`

	PricingQuantity JSONDecimal   `json:"pricing_quantity"`
	PricingUnit     string        `json:"pricing_unit"`
	SkuName         string        `json:"sku_name"`
	StartTime       JSONTimestamp `json:"start_time"`
	EndTime         JSONTimestamp `json:"end_time"`
	MasterAccountID string        `json:"master_account_id"`

	LabelsHash            uint64      `json:"labels_hash"`
	Cost                  JSONDecimal `json:"cost"`
	Credit                JSONDecimal `json:"credit"`
	CudCredit             JSONDecimal `json:"cud_credit"`
	MonetaryGrantCredit   JSONDecimal `json:"monetary_grant_credit"`
	VolumeIncentiveCredit JSONDecimal `json:"volume_incentive_credit"`

	CloudName                     string             `json:"cloud_name"`
	FolderName                    string             `json:"folder_name"`
	ServiceID                     string             `json:"service_id"`
	LabelsJSON                    JSONAnyString      `json:"labels_json"`
	PricingVersionID              string             `json:"pricing_version_id"`
	RateID                        int                `json:"rate_id"`
	SkuOverridden                 bool               `json:"sku_overridden"`
	Currency                      string             `json:"currency"`
	UnitPrice                     JSONDecimal        `json:"unit_price"`
	CurrencyMultiplier            JSONDecimal        `json:"currency_multiplier"`
	Expense                       JSONDecimal        `json:"expense"`
	RewardedExpense               JSONDecimal        `json:"rewarded_expense"`
	Revenue                       JSONDecimal        `json:"revenue"`
	CudCompensatedPricingQuantity JSONDecimal        `json:"cud_compensated_pricing_quantity"`
	ServiceCredit                 JSONDecimal        `json:"service_credit"`
	TrialCredit                   JSONDecimal        `json:"trial_credit"`
	DisabledCredit                JSONDecimal        `json:"disabled_credit"`
	VarIncentiveCredit            JSONDecimal        `json:"var_incentive_credit"`
	VolumeReward                  JSONDecimal        `json:"volume_reward"`
	Reward                        JSONDecimal        `json:"reward"`
	CreditCharges                 []CreditCharges    `json:"credit_charges"`
	VolumeRewardInfo              []VolumeRewardInfo `json:"volume_reward_info"`
	TiredPricingQuantity          JSONDecimal        `json:"tiered_pricing_quantity"`
	PublisherAccountID            string             `json:"publisher_account_id"`
	PublisherBalanceAccountID     string             `json:"publisher_balance_client_id"`
	PublisherCurrency             string             `json:"publisher_currency"`
}

type CreditCharges struct {
	Type                       string      `json:"type"`
	ID                         string      `json:"id"`
	CompensatedPricingQuantity JSONDecimal `json:"compensated_pricing_quantity"`
	Credit                     JSONDecimal `json:"credit"`
	UnusedCredit               JSONDecimal `json:"unused_credit"`
}

type VolumeRewardInfo struct {
	VarIncentiveID     string      `json:"var_incentive_id"`
	PreviousMultiplier JSONDecimal `json:"previous_multiplier"`
	CurrentMultiplier  JSONDecimal `json:"current_multiplier"`
	PreviousExpense    JSONDecimal `json:"previous_expense"`
	CurrentExpense     JSONDecimal `json:"current_expense"`
	Reward             JSONDecimal `json:"reward"`
	Adjustment         JSONDecimal `json:"adjustment"`
	Total              JSONDecimal `json:"total"`
}

type fullMktMetric struct { // example with fields sample (from clickhouse)
	EnrichedQueueMetric

	Cost                  JSONDecimal `json:"cost"`
	Credit                JSONDecimal `json:"credit"`
	MonetaryGrantCredit   JSONDecimal `json:"monetary_grant_credit"`
	CudCredit             JSONDecimal `json:"cud_credit"`
	VolumeIncentiveCredit JSONDecimal `json:"volume_incentive_credit"`

	BillingAccountName         string   `json:"billing_account_name"`
	BillingAccountState        string   `json:"billing_account_state"`
	BillingAccountPersontype   string   `json:"billing_account_person_type"`
	BillingAccountPaymentType  string   `json:"billing_account_payment_type"`
	BillingAccountCountryCode  string   `json:"billing_account_country_code"`
	BillingAccountCurrency     string   `json:"billing_account_currency"`
	BillingAccountUsageStatus  string   `json:"billing_account_usage_status"`
	BillingAccountFeatureFlags []string `json:"billing_account_feature_flags"`

	MktProductID               string      `json:"mkt_product_id"`
	MktProductSLUG             string      `json:"mkt_product_slug"`
	MktProductName             string      `json:"mkt_product_name"`
	MktProductState            string      `json:"mkt_product_state"`
	MktProductType             string      `json:"mkt_product_type"`
	MktPublisherID             string      `json:"mkt_publisher_id"`
	MktPublisherName           string      `json:"mkt_publisher_name"`
	MktVersionID               string      `json:"mkt_version_id"`
	MktVersionState            string      `json:"mkt_version_state"`
	MktVersionName             string      `json:"mkt_version_name"`
	MktProductLatestVersionID  string      `json:"mkt_product_latest_version_id"`
	MktProductAssociatedCost   JSONDecimal `json:"mkt_product_associated_cost"`
	MktProductAssociatedCredit JSONDecimal `json:"mkt_product_associated_credit"`

	RateID               int     `json:"rate_id"`
	PricingVersionID     string  `json:"pricing_version_id"`
	InstanceCores        int     `json:"instance_cores"`
	InstanceMemory       int     `json:"instance_memory"`
	InstanceFip          int     `json:"instance_fip"`
	InstancePlatform     string  `json:"instance_platform"`
	InstanceCoreFraction float64 `json:"instance_core_fraction"`
	InstancePreemptible  int     `json:"instance_preemptible"`
	InstanceGPUs         int     `json:"instance_gpus"`
	InstanceNVMEDisks    int     `json:"instance_nvme_disks"`

	LabelsMapKey   []string          `json:"labels_map.key"`
	LabelsMapValue []string          `json:"labels_map.value"`
	LabelsZip      map[string]string `json:"labels_zip"`

	SkuNameRu      string `json:"sku_name_ru"`
	SkuNameEn      string `json:"sku_name_en"`
	PricingUnit    string `json:"sku_pricing_unit"`
	SkuUsageUnit   string `json:"sku_usage_unit"`
	SkuServiceID   string `json:"sku_service_id"`
	SkuServiceName string `json:"sku_service_name"`
	SkuServiceDesc string `json:"sku_service_desc"`
}

type SourceMetrics []SourceMetric

func (sm *SourceMetrics) UnmarshalJSON(data []byte) error {
	if len(data) < 2 {
		return fmt.Errorf("incorrect value for souce metric: %s", string(data))
	}
	if data[0] == '"' { // very slow path for rare errors
		var str string
		if err := json.Unmarshal(data, &str); err != nil {
			return err
		}
		return sm.UnmarshalJSON([]byte(str))
	}
	if data[0] != '[' {
		var s SourceMetric
		err := easyjson.Unmarshal(data, &s)
		*sm = append((*sm)[:0], s)
		return err
	}
	s := (*[]SourceMetric)(sm)
	return json.Unmarshal(data, s)
}
