package ydb

import (
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/core/model/license"
	"a.yandex-team.ru/cloud/marketplace/license_server/internal/db/ydb"
)

func licenseLocksFromYDB(ins []*ydb.LicenseLock) ([]*license.Lock, error) {
	if ins == nil {
		return nil, nil
	}

	outs := make([]*license.Lock, 0, len(ins))
	for i := range ins {
		out, err := license.NewLockFromYDB(ins[i])
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}

func licenseInstancesFromYDB(ins []*ydb.LicenseInstance) ([]*license.Instance, error) {
	if ins == nil {
		return nil, nil
	}

	outs := make([]*license.Instance, 0, len(ins))
	for i := range ins {
		out, err := license.NewInstanceFromYDB(ins[i])
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}

func licenseTemplatesFromYDB(ins []*ydb.LicenseTemplate) ([]*license.Template, error) {
	if ins == nil {
		return nil, nil
	}

	outs := make([]*license.Template, 0, len(ins))
	for i := range ins {
		out, err := license.NewTemplateFromYDB(ins[i])
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}

func licenseTemplateVersionsFromYDB(ins []*ydb.LicenseTemplateVersion) ([]*license.TemplateVersion, error) {
	if ins == nil {
		return nil, nil
	}

	outs := make([]*license.TemplateVersion, 0, len(ins))
	for i := range ins {
		out, err := license.NewTemplateVersionFromYDB(ins[i])
		if err != nil {
			return nil, err
		}
		outs = append(outs, out)
	}

	return outs, nil
}
