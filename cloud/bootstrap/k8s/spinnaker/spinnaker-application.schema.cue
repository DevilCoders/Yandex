package spinnaker

let email_address = "^[a-zA-Z0-9_!#$%&’*+/=?`{|}~^.-]+@[a-zA-Z0-9.-]+$"

#Application: {
	accounts?: string
	appConfig?: {}
	cloudProviders?: "kubernetes" | ""
	createTs?:       string
	dataSources?: {
		disabled: [...]
		enabled: [...]
	}
	description?:                    string
	email?:                          =~email_address
	enableRerunActiveExecutions?:    bool
	enableRestartRunningExecutions?: bool
	instancePort?:                   80
	name:                            string
	permissions?: {
		EXECUTE: [...string]
		READ: [...string]
		WRITE: [...string]
	}
	customBanners: [...]
	repoProjectKey?: string
	repoSlug?:       string
	repoType?:       "bitbucket"
	trafficGuards?: []
	platformHealthOnly?:             bool
	platformHealthOnlyShowOverride?: bool
	user?:                           =~email_address
	instanceLinks?: []
}
