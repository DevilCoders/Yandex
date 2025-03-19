package main

import (
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/qtool"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/db/ydb/reports"
	"a.yandex-team.ru/cloud/billing/go/piper/internal/types"
)

type resourceReportsRowJSON struct {
	BillingAccountID      string              `db:"billing_account_id"`
	Date                  types.JSONTimestamp `db:"date"`
	CloudID               string              `db:"cloud_id"`
	SkuID                 string              `db:"sku_id"`
	FolderID              string              `db:"folder_id"`
	ResourceID            string              `db:"resource_id"`
	LabelsHash            uint64              `db:"labels_hash"`
	PricingQuantity       types.JSONDecimal   `db:"pricing_quantity"`
	Cost                  types.JSONDecimal   `db:"cost"`
	Credit                types.JSONDecimal   `db:"credit"`
	CudCredit             types.JSONDecimal   `db:"cud_credit"`
	MonetaryGrantCredit   types.JSONDecimal   `db:"monetary_grant_credit"`
	VolumeIncentiveCredit types.JSONDecimal   `db:"volume_incentive_credit"`
	FreeCredit            types.JSONDecimal   `db:"free_credit"`
}

func mapFromResourceReportsRowJSON(v resourceReportsRowJSON) reports.ResourceReportsRow {
	return reports.ResourceReportsRow{
		BillingAccountID:      v.BillingAccountID,
		Date:                  qtool.DateString(v.Date),
		CloudID:               v.CloudID,
		SkuID:                 v.SkuID,
		FolderID:              v.FolderID,
		ResourceID:            v.ResourceID,
		LabelsHash:            v.LabelsHash,
		PricingQuantity:       qtool.DefaultDecimal(v.PricingQuantity),
		Cost:                  qtool.DefaultDecimal(v.Cost),
		Credit:                qtool.DefaultDecimal(v.Credit),
		CudCredit:             qtool.DefaultDecimal(v.CudCredit),
		MonetaryGrantCredit:   qtool.DefaultDecimal(v.MonetaryGrantCredit),
		VolumeIncentiveCredit: qtool.DefaultDecimal(v.VolumeIncentiveCredit),
		FreeCredit:            qtool.DefaultDecimal(v.FreeCredit),
	}
}

func mapToResourceReportsRowJSON(v reports.ResourceReportsRow) resourceReportsRowJSON {
	return resourceReportsRowJSON{
		BillingAccountID:      v.BillingAccountID,
		Date:                  types.JSONTimestamp(v.Date),
		CloudID:               v.CloudID,
		SkuID:                 v.SkuID,
		FolderID:              v.FolderID,
		ResourceID:            v.ResourceID,
		LabelsHash:            v.LabelsHash,
		PricingQuantity:       types.JSONDecimal(v.PricingQuantity),
		Cost:                  types.JSONDecimal(v.Cost),
		Credit:                types.JSONDecimal(v.Credit),
		CudCredit:             types.JSONDecimal(v.CudCredit),
		MonetaryGrantCredit:   types.JSONDecimal(v.MonetaryGrantCredit),
		VolumeIncentiveCredit: types.JSONDecimal(v.VolumeIncentiveCredit),
		FreeCredit:            types.JSONDecimal(v.FreeCredit),
	}
}
