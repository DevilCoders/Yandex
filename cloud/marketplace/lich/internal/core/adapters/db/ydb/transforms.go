package ydb

import (
	"encoding/json"
	"fmt"

	"a.yandex-team.ru/cloud/marketplace/pkg/ydb"

	"a.yandex-team.ru/cloud/marketplace/lich/internal/core/model"

	ydb_internal "a.yandex-team.ru/cloud/marketplace/lich/internal/db/ydb"
)

func productVersionFromYDB(in *ydb_internal.ProductVersion) (out *model.ProductVersion, err error) {
	if in == nil {
		return nil, fmt.Errorf("product version parameter is nil")
	}

	out = &model.ProductVersion{
		ID: in.ID,
	}

	out.Payload, err = payloadFromAnyJSON(in.Payload)
	if err != nil {
		return nil, err
	}

	out.LicenseRules, err = licenseRulesFromAnyJSON(in.LicenseRules)
	if err != nil {
		return nil, err
	}

	return
}

func payloadFromAnyJSON(in *ydb.AnyJSON) (out model.Payload, err error) {
	if in == nil {
		return
	}

	type resourceSpecView struct {
		Min *int64 `json:"min"`
		Max *int64 `json:"max"`

		Recommended *int64 `json:"recommended"`
	}

	var view struct {
		ComputImage *struct {
			FolderID string `json:"folder_id"`
			PoolSize int    `json:"pool_size"`

			Codes []string `json:"codes"`

			FormID string `json:"form_id"`
			Family string `json:"family"`

			ImageID string `json:"image_id"`

			ResourceSpec struct {
				ComputPlatforms      []string `json:"comput_platforms"`
				ServiceAccountsRoles []string `json:"service_account_roles,omitempty"`

				CPU         *resourceSpecView `json:"cpu"`
				CPUFraction *resourceSpecView `json:"cpu_fraction"`
				GPU         *resourceSpecView `json:"gpu"`
				Memory      *resourceSpecView `json:"memory"`

				DiskSize *resourceSpecView `json:"disk_size"`

				NewtorkInterfaces *resourceSpecView `json:"network_interfaces"`

				LicensedInstancePool string `json:"licensed_instance_pool"`
			} `json:"resource_spec"`

			PackageInfo struct {
				OS struct {
					Family  string `json:"family"`
					Name    string `json:"name"`
					Version string `json:"version"`
				} `json:"os"`

				Contents []struct {
					Name    string `json:"name"`
					Version string `json:"version"`
				} `json:"package_contents"`
			} `json:"package_info"`

			ValidBaseImage bool `json:"valid_base_image"`
		} `json:"compute_image,omitempty"`
	}

	if err = json.Unmarshal(in.Bytes(), &view); err != nil {
		return
	}

	fillInt64 := func(p *int64) (i int64) {
		if p == nil {
			return
		}

		return *p
	}

	fillResourceSpec := func(r *resourceSpecView) (spec model.ResourceIntervalRestriction) {
		if r == nil {
			return spec
		}

		spec.Min = fillInt64(r.Min)
		spec.Max = fillInt64(r.Max)

		spec.Recommended = fillInt64(r.Recommended)

		return
	}

	if view.ComputImage != nil {
		dbComputeImageView := view.ComputImage

		out.ComputeImage = &model.ComputeImageProductPayload{
			ImageID:  dbComputeImageView.ImageID,
			FolderID: dbComputeImageView.FolderID,

			ResourceSpec: model.ResourceSpec{
				ServiceAccountsRoles: dbComputeImageView.ResourceSpec.ServiceAccountsRoles,
				ComputePlatforms:     dbComputeImageView.ResourceSpec.ComputPlatforms,

				CPU:         fillResourceSpec(dbComputeImageView.ResourceSpec.CPU),
				CPUFraction: fillResourceSpec(dbComputeImageView.ResourceSpec.CPUFraction),
				Memory:      fillResourceSpec(dbComputeImageView.ResourceSpec.Memory),
				GPU:         fillResourceSpec(dbComputeImageView.ResourceSpec.GPU),

				DiskSize: fillResourceSpec(dbComputeImageView.ResourceSpec.DiskSize),

				NetworkInterfaces: fillResourceSpec(dbComputeImageView.ResourceSpec.NewtorkInterfaces),

				LicenseInstancePool: dbComputeImageView.ResourceSpec.LicensedInstancePool,
			},

			FormID:         dbComputeImageView.FormID,
			Codes:          dbComputeImageView.Codes,
			ValidBaseImage: dbComputeImageView.ValidBaseImage,
		}

		out.ComputeImage.PackageInfo.OS.Family = view.ComputImage.PackageInfo.OS.Family
		out.ComputeImage.PackageInfo.OS.Name = view.ComputImage.PackageInfo.OS.Name
		out.ComputeImage.PackageInfo.OS.Version = view.ComputImage.PackageInfo.OS.Version

		for i := range view.ComputImage.PackageInfo.Contents {
			out.ComputeImage.PackageInfo.Content = append(out.ComputeImage.PackageInfo.Content, model.PackageInfoContent{
				Name:    view.ComputImage.PackageInfo.Contents[i].Name,
				Version: view.ComputImage.PackageInfo.Contents[i].Version,
			})
		}
	}

	return
}

func licenseRulesFromAnyJSON(in *ydb.AnyJSON) (rulesSpecs []model.RuleSpec, err error) {
	if in == nil {
		return nil, nil
	}

	var view []struct {
		Path string `json:"path"`

		Category model.RuleSpecCategory `json:"category"`
		Entity   model.RuleSpecEntity   `json:"entity"`

		Expected []string `json:"expected"`

		Precondition *struct {
			Path string `json:"path"`

			Category model.RuleSpecCategory `json:"category"`
			Entity   model.RuleSpecEntity   `json:"entity"`

			Expected []string `json:"expected"`
		} `json:"precondition"`

		Info struct {
			Message string `json:"message"`
		} `json:"info"`
	}

	if err = json.Unmarshal(in.Bytes(), &view); err != nil {
		return
	}

	for i := range view {
		if err := view[i].Category.Validate(); err != nil {
			return nil, err
		}

		baseRule := model.RuleSpec{
			BaseRule: model.BaseRule{
				Path:     view[i].Path,
				Category: view[i].Category,
				Entity:   view[i].Entity,
				Expected: view[i].Expected,
			},
		}

		if view[i].Precondition != nil {
			baseRule.Precondition = &model.BaseRule{
				Path:     view[i].Precondition.Path,
				Category: view[i].Precondition.Category,
				Entity:   view[i].Precondition.Entity,
				Expected: view[i].Precondition.Expected,
			}
		}

		baseRule.Info.Message = view[i].Info.Message
		rulesSpecs = append(rulesSpecs, baseRule)
	}

	return
}
