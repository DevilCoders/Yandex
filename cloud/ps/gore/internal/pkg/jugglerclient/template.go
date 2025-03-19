package jugglerclient

import "a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"

const (
	// DescriptionTemplate - Rule.Description field for /pkg/juggler module
	DescriptionTemplate = "juggler.description"
)

func init() {
	template.RegisterTemplate(
		DescriptionTemplate,
		template.Template{
			Text:   "Logins updated by GoRe for {{.Env}} {{.Service}} at {{.Start.Format .Format}}",
			Format: "02-01-2006 15:04",
		},
	)
}
