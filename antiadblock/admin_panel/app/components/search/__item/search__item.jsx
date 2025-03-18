import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';
import {timeAgo} from 'app/lib/date';
import {antiadbUrl} from 'app/lib/url';
import i18n from 'app/lib/i18n';

import Bem from 'app/components/bem/bem';
import Button from 'lego-on-react/src/components/button/button.react';
import Link from 'lego-on-react/src/components/link/link.react';
import ColorizedText from 'app/components/colorized-text/colorized-text';
import Hint from 'app/components/hint/hint';

import {configType} from 'app/types';
import {getSearchById} from 'app/reducers/index';
import {openConfigDiffModal} from 'app/actions/config-diff-modal';
import {openConfigPreview} from 'app/actions/config-preview';

import {STATUS, STATUS_COLORS} from 'app/enums/config';

import './search__item.css';

class SearchConfigsItem extends React.Component {
    constructor(props) {
        super(props);

        this.onPreviewClick = this.onPreviewClick.bind(this);
        this.openConfigDiffModal = this.openConfigDiffModal.bind(this);
    }

    onPreviewClick() {
        this.props.openConfigPreview(this.props.config.service_id, this.props.config.id, this.props.config);
    }

    openConfigDiffModal() {
        this.props.openConfigDiffModal(this.props.config.service_id, this.props.config.id, this.props.config.parent_id);
    }

    renderStatus(status) {
        const isDeclined = status.status === STATUS.DECLINED;
        const isExperiment = status.status === STATUS.EXPERIMENT;

        return (
            <ColorizedText
                key={status.status}
                color={STATUS_COLORS[status.status]}>
                {i18n('service-page', `config-status-${status.status}`)}
                {status.comment && (isDeclined || isExperiment) ?
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

    render() {
        const {config} = this.props;
        const createdDate = new Date(config.created);
        // TODO: parse statuses on separate component
        return (
            <Bem
                block='search'
                elem='item'
                mix={[
                    this.props.mix
                ]}>
                <Bem
                    block='search'
                    elem='service-id'>
                    <Link
                        theme='black'
                        url={antiadbUrl(`/service/${config.service_id}`)}>
                        {config.service_id}
                    </Link>
                </Bem>
                <Button
                    view='default'
                    tone='grey'
                    size='s'
                    theme='clear'
                    onClick={this.openConfigDiffModal}>
                    <Bem
                        block='search'
                        elem='item-id'>
                        #{config.id}
                    </Bem>
                </Button>
                <Bem
                    block='search'
                    elem='item-info'>
                    <Bem
                        block='search'
                        elem='item-info-comment'>
                        {config.comment}
                    </Bem>
                    <Bem
                        block='search'
                        elem='item-info-date'>
                        <span title={`${createdDate.toLocaleDateString()} ${createdDate.toLocaleTimeString()}`}>
                            {timeAgo(createdDate)}
                        </span>
                    </Bem>
                </Bem>
                <Bem
                    block='search'
                    elem='item-status'>
                    {config.archived ?
                        i18n('common', 'archived') :
                        this.renderStatuses(config.statuses)
                    }
                </Bem>
                <Bem
                    block='search'
                    elem='item-actions'>
                    <Button
                        view='default'
                        tone='grey'
                        size='s'
                        theme='normal'
                        onClick={this.onPreviewClick}>
                        {i18n('common', 'view')}
                    </Button>
                </Bem>
            </Bem>
        );
    }
}

SearchConfigsItem.propTypes = {
    config: configType.isRequired,
    mix: PropTypes.object.isRequired,
    openConfigPreview: PropTypes.func.isRequired,
    openConfigDiffModal: PropTypes.func.isRequired
};
export default connect((state, props) => {
    const config = getSearchById(state, props.id);

    return {
        config: config
    };
}, dispatch => {
    return {
        openConfigPreview: (serviceId, configId, configData) => {
            dispatch(openConfigPreview(serviceId, configId, configData));
        },
        openConfigDiffModal: (serviceId, id, oldId) => {
            dispatch(openConfigDiffModal(serviceId, oldId, id));
        }
    };
})(SearchConfigsItem);
