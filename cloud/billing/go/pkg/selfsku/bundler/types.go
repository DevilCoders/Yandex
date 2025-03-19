package bundler

import (
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/origin"
	"a.yandex-team.ru/cloud/billing/go/pkg/selfsku/tools"
)

func sourcePath(p string) tools.SourcePath {
	return tools.SourcePath(p)
}

type UnitConversion struct {
	tools.SourcePath
	origin.UnitConversion
}

type Service struct {
	tools.SourcePath
	origin.Service
}

type TagChecks struct {
	tools.SourcePath
	origin.TagChecks
}

type ServiceSkus struct {
	tools.SourcePath
	origin.ServiceSkus
}

type BundleSkus struct {
	tools.SourcePath
	origin.BundleSkus
}
