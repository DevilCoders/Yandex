package spinnaker

let email_address = "^[a-zA-Z0-9_!#$%&’*+/=?`{|}~^.-]+@[a-zA-Z0-9.-]+$"

#Project: {
	config: {
		"applications": [...string]
		"clusters": [
			...{
				account:      string
				applications: null
				detail:       string
				stack:        string
			},
		]
		"pipelineConfigs": [
			...{
				application:      string
				pipelineConfigId: string
			},
		]
	}
	createTs: int
	email:    =~email_address
	id:       string
	name:     string
}
