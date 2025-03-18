import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {Link} from 'react-router-dom';

import Bem from 'app/components/bem/bem';
import ColorizedText from 'app/components/colorized-text/colorized-text';
import Preloader from 'app/components/preloader/preloader';
import Hint from 'app/components/hint/hint';

import {getService} from 'app/reducers/index';
import {getSbs, getSbsProfiles} from 'app/reducers/service/index';
import {getConfigById} from 'app/reducers/service';
import {openConfigPreview} from 'app/actions/config-preview';
import {openConfigApplying} from 'app/actions/config-applying';
import {openConfigDiffModal} from 'app/actions/config-diff-modal';
import {openConfigDeclining} from 'app/actions/config-declining';
import {openExperimentApplying} from 'app/actions/experiment-applying';
import {closeConfirmDialog, openConfirmDialog} from 'app/actions/confirm-dialog';
import {runServiceSbsChecks} from 'app/actions/service/sbs';
import {setModerateConfig, setArchivedConfig, removeExperiment} from 'app/actions/service';

import Button from 'lego-on-react/src/components/button/button.react';
import LegoLink from 'lego-on-react/src/components/link/link.react';
import Icon from 'lego-on-react/src/components/icon/icon.react';

import {configType, moderationType} from 'app/types';

import {ACTIONS} from 'app/enums/actions';
import {STATUS, STATUS_COLORS} from 'app/enums/config';
import {antiadbUrl} from 'app/lib/url';
import {getProfileTag} from '../../../lib/sbs/profile-tags';
import i18n from 'app/lib/i18n';
import {timeAgo} from 'app/lib/date';
import {getUser} from 'app/lib/user';
import {getPermissions} from 'app/lib/permissions';

import './service-configs__item.css';
import 'app/components/icon/_theme/icon_theme_trash.css';

class ServiceConfigsItem extends React.Component {
    constructor() {
        super();

        this.onPreviewClick = this.onPreviewClick.bind(this);
        this.onApplyClick = this.onApplyClick.bind(this);
        this.onExperimentClick = this.onExperimentClick.bind(this);
        this.onExperimentRemoveClick = this.onExperimentRemoveClick.bind(this);
        this.onApplyForPreviewClick = this.onApplyForPreviewClick.bind(this);
        this.onArchiveClick = this.onArchiveClick.bind(this);
        this.onRestoreClick = this.onRestoreClick.bind(this);
        this.onApproveClick = this.onApproveClick.bind(this);
        this.onDeclineClick = this.onDeclineClick.bind(this);
        this.onConfigDiffClick = this.onConfigDiffClick.bind(this);
        this.onSbsRunChecks = this.onSbsRunChecks.bind(this);
    }

    onConfigDiffClick() {
        this.props.openConfigDiffModal(this.props.serviceId, this.props.config.id, this.props.config.parent_id);
    }

    onPreviewClick() {
        this.props.openConfigPreview(this.props.serviceId, this.props.id, this.props.config);
    }

    onApplyClick() {
        this.props.openConfigApplying(this.props.serviceId, this.props.id, this.props.config, STATUS.ACTIVE);
    }

    onExperimentClick() {
        this.props.openExperimentApplying(this.props.serviceId, this.props.id, this.props.config);
    }

    onExperimentRemoveClick() {
        this.props.removeExperiment(this.props.serviceId, this.props.id);
    }

    onApplyForPreviewClick() {
        this.props.openConfigApplying(this.props.serviceId, this.props.id, this.props.config, STATUS.TEST);
    }

    onArchiveClick() {
        this.props.setArchivedConfig(this.props.serviceId, this.props.id, true);
    }

    onRestoreClick() {
        this.props.setArchivedConfig(this.props.serviceId, this.props.id, false);
    }

    onApproveClick() {
        const statuses = this.mapStatuses();
        const configHasDeclined = statuses.includes(STATUS.DECLINED);
        this.props.setModerateConfig(this.props.serviceId, this.props.config.label_id, this.props.id, true, 'approved', configHasDeclined);
    }

    onDeclineClick() {
        this.props.openConfigDeclining(this.props.serviceId, this.props.config.label_id, this.props.id);
    }

    onSbsRunChecks(testing) {
        return () => {
            this.props.runServiceSbsChecks(
                this.props.serviceId,
                testing,
                this.props.config.exp_id,
                getProfileTag(this.props.serviceId)
            );
        };
    }

    mapStatuses() {
        const {config} = this.props;

        return config.statuses.map(function(status) {
            return status.status;
        });
    }

    renderArchivedActions() {
        const user = getUser();
        const permissions = getPermissions();

        return (
            <Bem
                block='service-configs'
                elem='item-actions'>
                <Button
                    view='default'
                    tone='grey'
                    size='s'
                    theme='normal'
                    disabled={!(this.props.config && this.props.config.parent_id)}
                    onClick={this.onConfigDiffClick}>
                    {i18n('service-page', 'diff-title')}
                </Button>
                {user.hasPermission(permissions.CONFIG_CREATE) && (
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        onClick={this.onRestoreClick}>
                        {i18n('service-page', 'config-restore')}
                    </Button>
                )}
            </Bem>
        );
    }

    renderActions() {
        const user = getUser();
        const permissions = getPermissions();
        // TODO: parse statuses on separate component
        const statuses = this.mapStatuses();
        const configHasActive = statuses.includes(STATUS.ACTIVE);
        const configHasTest = statuses.includes(STATUS.TEST);
        const configHasApproved = statuses.includes(STATUS.APPROVED);
        const configHasDeclined = statuses.includes(STATUS.DECLINED);
        const configHasExperiment = this.props.config.exp_id;
        const configApplyingProgress = this.props.applyingConfig.progress && this.props.applyingConfig.id === this.props.id;
        const configApplying = configApplyingProgress && this.props.applyingConfig.target === STATUS.ACTIVE;
        const configApplyingForPreview = configApplyingProgress && this.props.applyingConfig.target === STATUS.ACTIVE;
        const configApplyUserPermission = user.hasPermission(permissions.CONFIG_MARK_ACTIVE) && !configHasActive && configHasApproved;
        const configApplyAdminPermission = user.hasPermission(permissions.CONFIG_MODERATE) && !configHasActive;
        const configModerating = this.props.moderatingConfig.progress && this.props.moderatingConfig.id === this.props.id;
        const configApproving = configModerating && this.props.moderatingConfig.approve === true;
        const configDeclining = configModerating && this.props.moderatingConfig.approve === false;
        const experimentInProgress = this.props.experimentConfig.progress;

        return (
            <Bem
                block='service-configs'
                elem='item-actions'>
                {user.hasPermission(permissions.CONFIG_CREATE) && (configHasActive || configHasTest || configHasExperiment) &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        disable={!this.props.profileExist}
                        progress={this.props.loadingRunChecks}
                        onClick={this.onSbsRunChecks(configHasTest)}
                        prvntKeys={['ENTER']}>
                        {i18n('service-page', 'config-sbs-run-checks')}
                    </Button>}
                {user.hasPermission(permissions.CONFIG_MARK_TEST) && !configHasTest && !configHasDeclined &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        progress={configApplyingForPreview}
                        disabled={(this.props.applyingConfig.progress && !configApplyingForPreview) || configModerating}
                        onClick={this.onApplyForPreviewClick}
                        prvntKeys={['ENTER']}>
                        {i18n('service-page', 'config-apply-for-test')}
                    </Button>}
                {!configHasDeclined && (configApplyUserPermission || configApplyAdminPermission) &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        progress={configApplying}
                        disabled={(this.props.applyingConfig.progress && !configApplying) || configModerating}
                        onClick={this.onApplyClick}
                        prvntKeys={['ENTER']}>
                        {i18n('service-page', 'config-apply')}
                    </Button>}
                {user.hasPermission(permissions.CONFIG_MODERATE) && !configHasActive && !configHasApproved &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        progress={configApproving}
                        disabled={(configModerating && !configApproving) || configApplyingProgress}
                        onClick={this.onApproveClick}
                        prvntKeys={['ENTER']}>
                        {i18n('service-page', 'config-approved')}
                    </Button>}
                {user.hasPermission(permissions.CONFIG_MODERATE) && !configHasTest && !configHasActive && !configHasDeclined &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        progress={configDeclining}
                        disabled={(configModerating && !configDeclining) || configApplyingProgress}
                        onClick={this.onDeclineClick}
                        prvntKeys={['ENTER']}>
                        {i18n('service-page', 'config-declined')}
                    </Button>}
                {!configHasExperiment && (user.hasPermission(permissions.CONFIG_MARK_ACTIVE) || user.hasPermission(permissions.CONFIG_MODERATE)) &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='action'
                        disabled={experimentInProgress}
                        onClick={this.onExperimentClick}>
                        {i18n('service-page', 'config-experiment')}
                    </Button>}
                {configHasExperiment && (user.hasPermission(permissions.CONFIG_MARK_ACTIVE) || user.hasPermission(permissions.CONFIG_MODERATE)) &&
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='normal'
                        disabled={experimentInProgress}
                        onClick={this.onExperimentRemoveClick}>
                        {i18n('service-page', 'config-experiment-cancel')}
                    </Button>}
                <Button
                    view='default'
                    tone='grey'
                    size='s'
                    theme='normal'
                    disabled={!(this.props.config && this.props.config.parent_id)}
                    onClick={this.onConfigDiffClick}>
                    {i18n('service-page', 'diff-title')}
                </Button>
                {user.hasPermission(permissions.CONFIG_CREATE) && (
                    <Link
                        to={antiadbUrl(`/service/${this.props.serviceId}/label/${this.props.config.label_id}/${ACTIONS.CONFIGS}/${this.props.id}/status/${this.getStatus(configHasActive, configHasTest, configHasExperiment)}`)}>
                        <Button
                            view='default'
                            tone='grey'
                            size='s'
                            theme='link'>
                            {i18n('common', 'duplicate')}
                        </Button>
                    </Link>
                )}
                <Bem
                    block='service-configs'
                    elem='item-trash-icon'>
                    {user.hasPermission(permissions.CONFIG_CREATE) && !configHasActive && !configHasTest && ( // конфиг не применен
                        <Icon
                            key='icon'
                            attrs={{
                                onClick: this.onArchiveClick
                            }}
                            mix={{
                                block: 'icon',
                                mods: {
                                    theme: 'trash'
                                }
                            }}
                            size='s' />
                    )}
                </Bem>
            </Bem>
        );
    }

    renderStatus(status) {
        const isDeclined = status.status === STATUS.DECLINED;
        const isExperiment = status.status === STATUS.EXPERIMENT;

        return (
            <ColorizedText
                key={status.status}
                color={STATUS_COLORS[status.status]}>
                {i18n('service-page', `config-status-${status.status}`)}
                {isDeclined || isExperiment ?
                    <Hint
                        text={status.comment}
                        to='right'
                        theme={isDeclined ? 'attention-red' : 'hint'}
                        scope={this.context.scope} /> :
                        null}
            </ColorizedText>
        );
    }

    renderStatuses(statuses) {
        if (this.props.config.exp_id) {
            statuses = [
                ...statuses,
                {
                    status: STATUS.EXPERIMENT,
                    comment: this.props.config.exp_id
                }
            ];
        }

        if (statuses.length === 0) {
            return this.renderStatus({
                status: STATUS.INACTIVE,
                comment: ''
            });
        }

        const declined = statuses.find(status => status.status === STATUS.DECLINED);

        if (declined) {
            return this.renderStatus(declined);
        }

        const isActive = statuses.find(status => status.status === STATUS.ACTIVE);

        return statuses.map(status => {
            if (isActive && status.status === STATUS.APPROVED) {
                return '';
            }
            return this.renderStatus(status);
        });
    }

    getStatus(hasActive, hasTest, hasExp) {
        return (
            hasActive ? STATUS.ACTIVE :
            hasTest ? STATUS.TEST :
            hasExp ? STATUS.EXPERIMENT : ''
        );
    }

    render() {
        const {config} = this.props;
        const createdDate = new Date(config.created);

        return (
            <Bem
                block='service-configs'
                elem='item'
                mix={[
                    this.props.mix,
                    config.archived ? {
                        block: 'service-configs',
                        elem: 'item_archived'
                    } :
                    null
                ]}>
                {config.processing && <Preloader />}
                <Bem
                    block='service-configs'
                    elem='item-id'>
                    <LegoLink
                        theme='black'
                        elem='item-id-link'
                        onClick={this.onPreviewClick}>
                        #{config.id}
                    </LegoLink>
                </Bem>
                <Bem
                    block='service-configs'
                    elem='item-info'>
                    <Bem
                        block='service-configs'
                        elem='item-info-comment'>
                        <LegoLink
                            theme='black'
                            elem='item-info-comment-link'
                            onClick={this.onPreviewClick}>
                            {config.comment}
                        </LegoLink>
                    </Bem>
                    <Bem
                        block='service-configs'
                        elem='item-info-date'>
                        <span title={`${createdDate.toLocaleDateString()} ${createdDate.toLocaleTimeString()}`}>
                            {timeAgo(createdDate)}
                        </span>
                    </Bem>
                </Bem>
                <Bem
                    block='service-configs'
                    elem='item-status'>
                    {config.archived ?
                        i18n('common', 'archived') :
                        this.renderStatuses(this.props.config.statuses)
                    }
                </Bem>
                {config.archived ?
                    this.renderArchivedActions() :
                    this.renderActions()}
            </Bem>
        );
    }
}

ServiceConfigsItem.propTypes = {
    id: PropTypes.number.isRequired,
    serviceId: PropTypes.string.isRequired,
    applyingConfig: PropTypes.shape({
        id: PropTypes.number,
        progress: PropTypes.bool,
        target: PropTypes.string
    }).isRequired,
    moderatingConfig: moderationType.isRequired,
    experimentConfig: PropTypes.object.isRequired,
    config: configType,
    mix: PropTypes.object,
    openConfigPreview: PropTypes.func.isRequired,
    openConfigApplying: PropTypes.func.isRequired,
    setArchivedConfig: PropTypes.func.isRequired,
    setModerateConfig: PropTypes.func.isRequired,
    openConfigDeclining: PropTypes.func.isRequired,
    openConfigDiffModal: PropTypes.func.isRequired,
    openExperimentApplying: PropTypes.func.isRequired,
    runServiceSbsChecks: PropTypes.func.isRequired,
    loadingRunChecks: PropTypes.bool.isRequired,
    removeExperiment: PropTypes.func.isRequired,
    profileExist: PropTypes.bool.isRequired
};

export default connect((state, props) => {
    const service = getService(state);

    return {
        serviceId: service.service.id,
        applyingConfig: service.applyingConfig,
        moderatingConfig: service.moderatingConfig,
        experimentConfig: service.experimentApplying,
        config: getConfigById(service, props.id),
        loadingRunChecks: getSbs(service).loadingRunChecks,
        profileExist: getSbsProfiles(getService(state)).exist
    };
}, dispatch => {
    return {
        openConfigDiffModal: (serviceId, id, oldId) => {
            dispatch(openConfigDiffModal(serviceId, oldId, id));
        },
        openConfigPreview: (serviceId, configId, configData) => {
            dispatch(openConfigPreview(serviceId, configId, configData));
        },
        openConfigApplying: (serviceId, configId, configData, target) => {
            dispatch(openConfigApplying(serviceId, configId, configData, target));
        },
        openExperimentApplying: (serviceId, configId) => {
            dispatch(openExperimentApplying(serviceId, configId));
        },
        removeExperiment: (serviceId, configId) => {
            dispatch(removeExperiment(serviceId, configId));
        },
        setArchivedConfig: (serviceId, configId, archived) => {
            dispatch(setArchivedConfig(serviceId, configId, archived));
        },
        setModerateConfig: (serviceId, labelId, configId, approved, comment, declined) => {
            if (declined) {
                dispatch(openConfirmDialog(i18n('service-page', 'approved-declined-config'), result => {
                    dispatch(closeConfirmDialog());

                    if (result) {
                        dispatch(setModerateConfig(serviceId, labelId, configId, approved, comment));
                    }
                }));
            } else {
                dispatch(setModerateConfig(serviceId, labelId, configId, approved, comment));
            }
        },
        openConfigDeclining: (serviceId, labelId, configId) => {
            dispatch(openConfigDeclining(serviceId, labelId, configId));
        },
        runServiceSbsChecks: (serviceId, testing, expId, tag) => {
            dispatch(runServiceSbsChecks(serviceId, testing, expId, tag));
        }

    };
})(ServiceConfigsItem);
