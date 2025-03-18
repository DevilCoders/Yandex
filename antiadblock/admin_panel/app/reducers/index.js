import {combineReducers} from 'redux';

import alerts from './alerts';
import service from './service';
import services from './services';
import configPreview from './config-preview';
import configApplying from './config-applying';
import configDiffModal from './config-diff-modal';
import ticketCreation from './ticket-creation';
import confirmDialog from './confirm-dialog';
import configDeclining from './config-declining';
import createLabel from './create-label';
import experimentApplying from './experiment-applying';
import errors from './errors';
import metric from './metric';
import search from './search';
import heatmap from './heatmap';
import user from './user';
import profileDiffModal from './profile-diff-modal';

import config from './config/config';

const appReducer = combineReducers({
    service,
    services,
    configPreview,
    configApplying,
    configDiffModal,
    confirmDialog,
    configDeclining,
    createLabel,
    experimentApplying,
    metric,
    errors,
    search,
    heatmap,
    alerts,
    user,
    config,
    profileDiffModal,
    ticketCreation
});

export default appReducer;

export function getService(state) {
    return state.service;
}

export function getServices(state) {
    return state.services;
}

export function getConfigPreview(state) {
    return state.configPreview;
}

export function getConfigApplying(state) {
    return state.configApplying;
}

export function getConfigDiffModal(state) {
    return state.configDiffModal;
}

export function getConfigDeclining(state) {
    return state.configDeclining;
}

export function getExperimentApplying(state) {
    return state.experimentApplying;
}

export function getTicketCreation(state) {
    return state.ticketCreation;
}

export function getMetric(state, serviceId) {
    return state.metric[serviceId];
}

export function getErrors(state) {
    return state.errors;
}

export function getSearch(state) {
    return state.search;
}

export function getHeatmap(state) {
    return state.heatmap;
}

export function getSearchById(state, id) {
    return state.search.byId[id];
}

export function getAlerts(state) {
    return state.alerts;
}

export function getUser(state) {
    return state.user;
}

export function getConfig(state) {
    return state.config;
}

export function getProfileDiffModal(state) {
    return state.profileDiffModal;
}

export function getCreateLabel(state) {
    return state.createLabel;
}
