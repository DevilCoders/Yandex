package spinnaker

UpdateSaltFormulaVersionPipelineName: "[child] update salt version" | "[child] update salt version HW" | "[child] update salt version SVM"

//Is it possible to remove a branch from Spinnaker? (to repleace check exists step)

UpdateSaltFormulaVersionPipeline: GenericPipeline & {
	name: UpdateSaltFormulaVersionPipelineName
	parameterConfig: [ParameterReleaseTicket, ParameterSelectStand, ParameterSaltFormulaVersion]
	stages: [
		//k8s-deploy
		UpdateSaltFormulaVersionConfigureContext,
		UpdateSaltFormulaVersionCheckBranchExistsK8SDeploy,
		UpdateSaltFormulaVersionCreateBranchK8SDeploy,
		UpdateSaltFormulaVersionUpdateFilesK8SDeploy,
		UpdateSaltFormulaVersionGenerateK8SManifests,
		UpdateSaltFormulaVersionWaitForPRMergedK8SDeploy,
		//cluster-configs
		UpdateSaltFormulaVersionCheckBranchExistsClusterConfigs,
		UpdateSaltFormulaVersionCreateBranchClusterConfigs,
		UpdateSaltFormulaVersionUpdateFileClusterConfigs,
		UpdateSaltFormulaVersionWaitPRClusterConfigs,
	] |
		[
			UpdateSaltFormulaVersionConfigureContext,
			UpdateSaltFormulaVersionCheckBranchExistsK8SDeploy,
			UpdateSaltFormulaVersionCheckBranchExistsClusterConfigs,
			UpdateSaltFormulaVersionCreateBranchK8SDeploy,
			UpdateSaltFormulaVersionUpdateFilesK8SDeploy,
			UpdateSaltFormulaVersionGenerateK8SManifests,
			UpdateSaltFormulaVersionWaitForPRMergedK8SDeploy,
			UpdateSaltFormulaVersionCreateBranchClusterConfigs,
			UpdateSaltFormulaVersionUpdateFileClusterConfigs,
			UpdateSaltFormulaVersionWaitPRClusterConfigs,
		]
}

UpdateSaltFormulaVersionConfigureContext: {
	failOnFailedExpressions: true
	name:                    "Configure context"
	refId:                   "10"
	requisiteStageRefIds: []
	type: "evaluateVariables"
	//Input parameters for pipepilne generation
	varialble: [...ConfigureContextVar]
	variables: [
		//Application specific variables
		ConfigureContextVarInSvmCluster,
		ConfigureContextVarNamespace,
		ConfigureContextVarBaseRole,
		ConfigureContextVarSaltRole,

		//Validate pipeline parameters
		ConfigureContextVarReleaseTicket,
		ConfigureContextVarStand,
		ConfigureContextVarSaltFormulaVersion,

		//Evaluate other vars
		ConfigureContextVarExecutionURL,
		ConfigureContextVarExecutionMD,
		ConfigureContextVarTriggerUserLogin,
		{
			key:   "branchName"
			value: "release-${parameters.releaseTicket}-${parameters.stand}"
		},
		ConfigureContextVarOptionalClusterSuffix,
		{//remove
			"key":   "k8sRelease"
			"value": "${baseRole}-base-role" | "node"
		},
		{
			"key":   "emptyPRWorkaround"
			"value": "# ${new java.text.SimpleDateFormat(\"yyMMddHHmmss\").format(new java.util.Date())} empty PR workaround"
		},

	]
}

UpdateSaltFormulaVersionCheckBranchExistsK8SDeploy: {
	actions: [close({
		//actionId:         "cdf79792-0f69-436b-9c65-283ce5406de0"
		actionName:       "check"
		actionType:       "getBranch"
		bitbucketProfile: "bb.yandex-team.ru"
		branchName:       "${branchName}"
		outputVariable:   "branchInfo_k8s_deploy"
		project:          "cloud"
		repo:             "k8s-deploy"
	})]
	completeOtherBranchesThenFail: false
	continuePipeline:              true
	failPipeline:                  false
	name:                          "[k8s-deploy] Check if branch exists"
	refId:                         "110"
	requisiteStageRefIds: [
		"10",
	]
	type: "bitbucket"
}

UpdateSaltFormulaVersionCreateBranchK8SDeploy: {
	actions: [{
		actionName:       "create"
		actionType:       "createBranch"
		bitbucketProfile: "bb.yandex-team.ru"
		branchName:       "${branchName}"
		outputVariable:   "branchInfo_k8s_deploy"
		project:          "cloud"
		repo:             "k8s-deploy"
		startPoint:       "master"
	}]
	name:  "[k8s-deploy] Create branch"
	refId: "120"
	requisiteStageRefIds: [
		"110",
	]
	stageEnabled: {
		expression: "${branchInfo_k8s_deploy == null}"
		type:       "expression"
	}
	type: "bitbucket"
}

UpdateSaltFormulaVersionK8SDeployEditFileAction: {
	_az:              string
	actionName:       "Update values.yaml for \"\(_az)\""
	actionType:       "editFile"
	bitbucketProfile: "bb.yandex-team.ru"
	branch:           "${branchName}"
	changes: [{
		changeType:  "regexReplace"
		expression:  "(${saltRole}:[\\s\\S]+?version:\\s*).+"
		replacement: "$1${saltFormulaVersion} ${emptyPRWorkaround}"
	}]
	commitMessage:    "${releaseTicket} Update salt formula version to ${saltFormulaVersion} for ${stand}-\(_az)"
	filePath:         "${stand}/ru-central1-\(_az)${optionalClusterSuffix}/helmfiles/${namespace}/${k8sRelease}/values.yaml"
	ignoreUnmodified: false //We use ${emptyPRWorkaround} that guarantees that a file is always modified
	project:          "cloud"
	repo:             "k8s-deploy"
}

UpdateSaltFormulaVersionUpdateFilesK8SDeploy: {
	actions: [
		UpdateSaltFormulaVersionK8SDeployEditFileAction & {_az: "a"},
		UpdateSaltFormulaVersionK8SDeployEditFileAction & {_az: "b"},
		UpdateSaltFormulaVersionK8SDeployEditFileAction & {_az: "c"},
		{
			actionId:         "e13cf689-406c-4fc2-95ba-0572504b3f2b"
			actionName:       "Create PR k8s-deploy"
			actionType:       "createPullRequest"
			bitbucketProfile: "bb.yandex-team.ru"
			description:      "Created by Spinnaker execution: ${spinnakerExecutionMD}"
			findExisting:     true
			fromRef:          "${branchName}"
			ignoreEmpty:      false
			outputVariable:   "bitbucketPR_k8s_deploy"
			project:          "cloud"
			repo:             "k8s-deploy"
			reviewers: ["${triggerUserLogin}"]
			title: "${releaseTicket} Update ${saltRole}=${saltFormulaVersion} for base role ${baseRole} in ${stand}"
			toRef: "master"
		}]
	failOnFailedExpressions: true
	name:                    "[k8s-deploy] Update salt formula version"
	refId:                   "130"
	requisiteStageRefIds: ["110", "120"]
	type: "bitbucket"
}

UpdateSaltFormulaVersionGenerateK8SManifests: {
	attempts:             3
	buildConfigurationId: "Selfhost_GenerateK8sManifests"
	buildParameters: [{
		key:   "env.PREFIX"
		value: "${namespace}/${k8sRelease}"
	}]
	completeOtherBranchesThenFail: false
	continuePipeline:              true
	failOnFailedExpressions:       false
	failPipeline:                  false
	logicalBranchName:             "${branchName}"
	name:                          "[k8s-deploy] Generate manifests"
	refId:                         "140"
	requisiteStageRefIds: [
		"130",
	]
	tcProfile: "aw"
	type:      "teamcityBuild"
}

UpdateSaltFormulaVersionWaitForPRMergedK8SDeploy: {
	actions: [{
		actionId:       "405835ea-eeaa-4305-b6c1-2afb69981e11"
		actionName:     "Wait for PR merged"
		actionType:     "waitPullRequest"
		source:         "priorAction"
		sourceActionId: "e13cf689-406c-4fc2-95ba-0572504b3f2b"
	}]
	completeOtherBranchesThenFail: false
	continuePipeline:              true
	failPipeline:                  false
	name:                          "[k8s-deploy] Wait for PR merged"
	refId:                         "150"
	requisiteStageRefIds: [
		"140",
	]
	type: "bitbucket"
}

UpdateSaltFormulaVersionCheckBranchExistsClusterConfigs: {
	actions: [{
		//actionId:         "cdf79792-0f69-436b-9c65-283ce5406de0"
		actionName:       "check"
		actionType:       "getBranch"
		bitbucketProfile: "bb.yandex-team.ru"
		branchName:       "${branchName}"
		outputVariable:   "branchInfo_cluster_configs" //check we use it
		project:          "cloud"
		repo:             "cluster-configs"
	}]
	completeOtherBranchesThenFail: false
	continuePipeline:              true
	failPipeline:                  false
	name:                          "[cluster-configs] Check if branch exists"
	refId:                         "210"
	requisiteStageRefIds: [
		"10",
	]
	type: "bitbucket"
}

UpdateSaltFormulaVersionCreateBranchClusterConfigs: {
	actions: [{
		actionName:       "create"
		actionType:       "createBranch"
		bitbucketProfile: "bb.yandex-team.ru"
		branchName:       "${branchName}"
		outputVariable:   "branchInfo_cluster_configs"
		project:          "cloud"
		repo:             "cluster-configs"
		startPoint:       "master"
	}]
	name:  "[cluster-configs] Create branch"
	refId: "220"
	requisiteStageRefIds: [
		"210",
	]
	stageEnabled: {
		expression: "${branchInfo_cluster_configs == null}"
		type:       "expression"
	}
	type: "bitbucket"
}

UpdateSaltFormulaVersionUpdateFileClusterConfigs: {
	_editSaltRoleVersion: {//generic schema for both salt-role-releases and base-rol-releases
		actionName:       "Update"
		actionType:       "editFile"
		bitbucketProfile: "bb.yandex-team.ru"
		branch:           "${branchName}"
		changes: [{
			changeType:  "regexReplace"
			expression:  string
			replacement: string
		}]
		commitMessage:    string
		filePath:         string
		ignoreUnmodified: false
		project:          "cloud"
		repo:             "cluster-configs"
		_prTitle:         string
		_prDescription:   string
	}
	_editSaltRoleVersionInSaltRoleReleases: _editSaltRoleVersion & {
		changes: [{
			expression:  "yc-salt-formula: .+"
			replacement: "yc-salt-formula: ${saltFormulaVersion}  ${emptyPRWorkaround}"
		},
		]
		_prTitle:       "${releaseTicket} Update ${saltRole}=${saltFormulaVersion} in ${stand}"
		_prDescription: "Created by Spinnaker execution: ${spinnakerExecutionMD}"
	}
	_editSaltRoleVersionInBaseRoleReleases: _editSaltRoleVersion & {
		changes: [{
			expression:  "(bootstrap:\\n      version: ).+"
			replacement: "$1${saltFormulaVersion} ${emptyPRWorkaround}"
		},
		]
		_prTitle:       "${releaseTicket} Update ${saltRole}=${saltFormulaVersion} for base role ${baseRole} in ${stand}"
		_prDescription: "Created by Spinnaker execution: ${spinnakerExecutionMD}"
	}
	actions: [
		_editSaltRoleVersionInSaltRoleReleases | _editSaltRoleVersionInBaseRoleReleases,
		{
			actionId:         "e13cf689-406c-4fc2-95ba-0572504b3f2b"
			actionName:       "Create PR cluster-configs"
			actionType:       "createPullRequest"
			bitbucketProfile: "bb.yandex-team.ru"
			description:      actions[0]._prDescription
			findExisting:     true
			fromRef:          "${branchName}"
			ignoreEmpty:      false
			outputVariable:   "bitbucketPR"
			project:          "cloud"
			repo:             "cluster-configs"
			reviewers: ["${triggerUserLogin}"]
			title: actions[0]._prTitle
			toRef: "master"
		}]
	failOnFailedExpressions: true
	name:                    "[cluster-configs] Update salt formula version"
	refId:                   "230"
	requisiteStageRefIds: ["210", "220"]
	type: "bitbucket"
}

UpdateSaltFormulaVersionWaitPRClusterConfigs: {
	actions: [{
		actionName:     "Wait for PR merged"
		actionType:     "waitPullRequest"
		source:         "priorAction"
		sourceActionId: "e13cf689-406c-4fc2-95ba-0572504b3f2b"
	}]
	name:  "[cluster-configs] Wait for PR merged"
	refId: "240"
	requisiteStageRefIds: [
		"230",
	]
	type: "bitbucket"
}
