package stclient

import (
	"a.yandex-team.ru/cloud/ps/gore/internal/pkg/template"
	"a.yandex-team.ru/cloud/ps/gore/pkg/startrek"
	"github.com/imdario/mergo"
)

const (
	// DescriptionTemplate has descriptionRenderType = "WF"
	DescriptionTemplate = "startrek.description"
	SummaryTemplate     = "startrek.summary"
	ChecklistTemplate   = "startrek.checklist"
)

func init() {
	template.RegisterTemplate(
		DescriptionTemplate,
		template.Template{
			Text:   "",
			Format: "02-01-2006 15:04",
			Unique: true,
		},
	)
	template.RegisterTemplate(
		SummaryTemplate,
		template.Template{
			Text:   "{{.Service}} from {{.Start.Format .Format}} to {{.End.Format .Format}}",
			Format: "02-01-2006 15:04",
			Unique: true,
		},
	)
	template.RegisterTemplate(
		ChecklistTemplate,
		template.Template{},
	)
}

func fillTemplatedData(sid string, s Storage, v *template.Variables, t *startrek.Ticket) (err error) {
	dt, _ := template.DefaultTemplate(SummaryTemplate)
	if summary, err := s.GetActiveTemplate(sid, SummaryTemplate); err == nil {
		_ = mergo.Merge(&dt, summary, mergo.WithOverride)
	}

	if t.Summary, err = dt.Render(v); err != nil {
		return
	}

	dt, _ = template.DefaultTemplate(DescriptionTemplate)
	if desc, err := s.GetActiveTemplate(sid, DescriptionTemplate); err == nil {
		_ = mergo.Merge(&dt, desc, mergo.WithOverride)
		t.DescRender = "WF"
	}

	if t.Description, err = dt.Render(v); err != nil {
		t.Description = ""
	}

	checklist, _ := s.ListTemplates(sid, "", ChecklistTemplate)
	for _, item := range checklist {
		if !item.Active {
			continue
		}

		if cl, err := item.Render(v); err == nil {
			t.ChecklistItems = append(t.ChecklistItems, startrek.ChecklistItem{
				Text:    cl,
				Checked: false,
			})
		}
	}

	return nil
}
