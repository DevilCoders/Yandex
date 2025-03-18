import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Bem from 'app/components/bem/bem';
import ColorizedText from 'app/components/colorized-text/colorized-text';
import Button from 'lego-on-react/src/components/button/button.react';

import {openConfigDiffModal} from 'app/actions/config-diff-modal';
import {openProfileDiffModal} from 'app/actions/profile-diff-modal';
import {getService} from 'app/reducers/index';
import {getAuditItemById} from 'app/reducers/service';

import {auditItemType} from 'app/types';

import {timeAgo} from 'app/lib/date';

import * as auditEnums from 'app/enums/audit';
import i18n from 'app/lib/i18n';

import './service-audit__item.css';

class ServiceAuditItem extends React.Component {
    constructor() {
        super();

        this.openConfigDiffModal = this.openConfigDiffModal.bind(this);
        this.openProfileDiffModal = this.openProfileDiffModal.bind(this);
    }

    openConfigDiffModal() {
        this.props.openConfigDiffModal(this.props.serviceId, this.props.auditItem.params.config_id, this.props.auditItem.params.old_config_id);
    }

    openProfileDiffModal() {
        this.props.openProfileDiffModal(this.props.serviceId, this.props.auditItem.params.new_profile_id, this.props.auditItem.params.old_profile_id);
    }

    getUser(auditItem) {
        return auditItem.user_name || auditItem.user_id;
    }

    getStatus(auditItem) {
        const action = auditItem.action;
        let i18nKey = `audit-action-${action}`;

        switch (action) {
            case auditEnums.ACTIONS.CONFIG_ARCHIVE:
                i18nKey += `-${auditItem.params.archived}`;
                break;
            case auditEnums.ACTIONS.CONFIG_MODERATE:
                i18nKey += `-${auditItem.params.moderate}`;
                break;
            case auditEnums.ACTIONS.SERVICE_STATUS_SWITCH:
                i18nKey += `-${auditItem.params.status === 'inactive' ? 'false' : 'true'}`;
                break;
            case auditEnums.ACTIONS.SERVICE_MONITORINGS_SWITCH:
                i18nKey += `-${auditItem.params.monitorings_enabled ? 'true' : 'false'}`;
                break;
            default:
                break;
        }

        return i18n('service-page', i18nKey);
    }

    getNormalizeDate(auditDate) {
        const date = new Date(auditDate);
        const options = {
            year: 'numeric',
            month: 'numeric',
            day: 'numeric',
            timezone: 'UTC',
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric'
        };
        const dateArr = (date).toLocaleString('ru', options).split(', ');

        return dateArr[0].split('.').reverse().join('.') + ', ' + dateArr[1];
    }

    getSubMessage(auditItem) {
        const action = auditItem.action;

        let result = '';

        switch (action) {
            // eslint-disable-next-line no-fallthrough
            case auditEnums.ACTIONS.TICKET_CREATE:
            case auditEnums.ACTIONS.SERVICE_STATUS_SWITCH:
            case auditEnums.ACTIONS.SERVICE_MONITORINGS_SWITCH:
            case auditEnums.ACTIONS.SERVICE_CREATE:
                // do nothing
                break;
            case auditEnums.ACTIONS.SERVICE_UPDATE_SBS_PROFILE:
                result = ` #${auditItem.params.new_profile_id}`;
                break;
            case auditEnums.ACTIONS.SUPPORT_PRIORITY_SWITCH:
                result = ` #${auditItem.params.support_priority}`;
                break;
            default:
                result = ` #${auditItem.params.config_id}`;
        }

        return result;
    }

    getUnderMessage(auditItem) {
        const action = auditItem.action;

        let result = '';

        switch (action) {
            // eslint-disable-next-line no-fallthrough
            case auditEnums.ACTIONS.SUPPORT_PRIORITY_SWITCH:
            case auditEnums.ACTIONS.TICKET_CREATE:
            case auditEnums.ACTIONS.SERVICE_STATUS_SWITCH:
            case auditEnums.ACTIONS.SERVICE_MONITORINGS_SWITCH:
            case auditEnums.ACTIONS.SERVICE_CREATE:
            case auditEnums.ACTIONS.SERVICE_UPDATE_SBS_PROFILE:
                // do nothing
                break;
            default:
                result = auditItem.label_id;
        }

        return result;
    }

    render() {
        const auditItem = this.props.auditItem;
        const date = new Date(auditItem.date);

        let action = auditItem.action;

        const isActivateAction = [
            auditEnums.ACTIONS.CONFIG_ACTIVE,
            auditEnums.ACTIONS.CONFIG_TEST
        ].includes(action);

        if (action === auditEnums.ACTIONS.CONFIG_MODERATE) {
            if (auditItem.params.moderate === 'approved') {
                action = auditEnums.ACTIONS.CONFIG_APPROVED;
            } else {
                action = auditEnums.ACTIONS.CONFIG_DECLINED;
            }
        }

        return (
            <Bem
                block='service-audit'
                elem='item'
                mix={this.props.mix}>
                <Bem
                    key='date'
                    block='service-audit'
                    elem='item-date'>
                    <span title={timeAgo(date)}>
                        {this.getNormalizeDate(auditItem.date)}
                    </span>
                </Bem>
                <Bem
                    key='author'
                    block='service-audit'
                    elem='item-author'>
                    {this.getUser(auditItem)}
                </Bem>
                <Bem
                    key='action'
                    block='service-audit'
                    elem='item-action'>
                    <ColorizedText color={auditEnums.ACTIONS_COLORS[action]}>
                        {this.getStatus(auditItem)}
                        <Bem
                            block='service-audit'
                            elem='item-id'>
                            {this.getSubMessage(auditItem)}
                        </Bem>
                    </ColorizedText>
                    <Bem
                        block='service-audit'
                        elem='label-id'>
                        <ColorizedText color='gray'>
                            {this.getUnderMessage(auditItem)}
                        </ColorizedText>
                    </Bem>
                </Bem>
                <Bem
                    key='additional-info'
                    block='service-audit'
                    elem='item-additional-info'>
                    {isActivateAction && auditItem.params.config_comment}
                    {action === auditEnums.ACTIONS.CONFIG_DECLINED && auditItem.params.comment}
                </Bem>
                <Bem
                    key='actions'
                    block='service-audit'
                    elem='item-actions'>
                    {isActivateAction &&
                        <Button
                            view='default'
                            tone='grey'
                            size='s'
                            theme='normal'
                            onClick={this.openConfigDiffModal}>
                            {i18n('service-page', 'diff-title')}
                        </Button>}
                    {auditEnums.ACTIONS.SERVICE_UPDATE_SBS_PROFILE === action &&
                        <Button
                            view='default'
                            tone='grey'
                            size='s'
                            theme='normal'
                            onClick={this.openProfileDiffModal}>
                            {i18n('service-page', 'diff-title')}
                        </Button>}
                </Bem>
            </Bem>
        );
    }
}

ServiceAuditItem.propTypes = {
    serviceId: PropTypes.string.isRequired,
    auditItem: auditItemType,
    mix: PropTypes.object,
    openConfigDiffModal: PropTypes.func.isRequired,
    openProfileDiffModal: PropTypes.func.isRequired
};

export default connect((state, props) => {
    return {
        serviceId: getService(state).service.id,
        auditItem: getAuditItemById(getService(state), props.id)
    };
}, dispatch => {
    return {
        openConfigDiffModal: (serviceId, id, oldId) => {
            dispatch(openConfigDiffModal(serviceId, oldId, id));
        },
        openProfileDiffModal: (serviceId, id, oldId) => {
            dispatch(openProfileDiffModal(serviceId, oldId, id));
        }
    };
})(ServiceAuditItem);
