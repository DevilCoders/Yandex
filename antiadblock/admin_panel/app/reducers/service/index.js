import {combineReducers} from 'redux';

import service from './service';
import audit from './audit';
import configs from './configs';
import applyingConfig from './applying-config';
import moderatingConfig from './moderating-config';
import experimentApplying from './experiment-applying';
import setup from './setup';
import labels from './labels';
import schema from './schema';
import health from './health';
import rules from './rules';
import sbs from './sbs';
import trend from './trend';

const serviceReducer = combineReducers({
    service,
    audit,
    configs,
    applyingConfig,
    moderatingConfig,
    experimentApplying,
    setup,
    schema,
    health,
    rules,
    labels,
    sbs,
    trend
});

export default serviceReducer;

const defaultConfig = {
    items: [],
    loaded: false,
    total: 0
};

export function getActiveLabelId(state) {
    return state.configs.activeLabelId;
}

export function getConfigsByLabelId(state, labelId) {
    return state.configs.byLabelId[labelId] || defaultConfig;
}

export function getConfigs(state) {
    return state.configs;
}

export function getConfigById(state, id) {
    return state.configs.byId[id];
}

export function getAudit(state) {
    return state.audit;
}

export function getAuditItemById(state, id) {
    return state.audit.byId[id];
}

export function getSchema(state) {
    return state.schema;
}

export function getHealth(state) {
    return state.health;
}

export function getSbs(state) {
    return state.sbs;
}

export function getSbsScreenshotsChecks(state) {
    return state.sbs.screenshotsChecks;
}

export function getSbsScreenshotsChecksDataByRunId(state, runId) {
    return state.sbs.screenshotsChecks.runIdData[runId];
}

export function getSbsProfiles(state) {
    return state.sbs.allSbsProfiles;
}

export function getSbsProfile(state) {
    return state.sbs.allSbsProfiles.profile;
}

export function getSbsProfileById(state, id) {
    return (state.sbs.allSbsProfiles.byId[id] ? state.sbs.allSbsProfiles.byId[id] : state.sbs.allSbsProfiles.byId[-1]);
}

export function getSbsProfilesTags(state) {
    return state.sbs.allSbsProfiles.profileTags;
}

export function getSbsResultChecks(state) {
    return state.sbs.resultChecks;
}

export function getRules(state) {
    return state.rules;
}

export function getLabels(state) {
    return state.labels;
}

export function getTrend(state) {
    return state.trend;
}

export function getParentConfigsExpId(state) {
    return state.configs.expIds;
}

export function getSbsScreenshotsChecksGroupVisibility(state) {
    return state.sbs.screenshotsChecks.logsVisible;
}

export function getSbsLogs(state) {
    return state.sbs.logs;
}
