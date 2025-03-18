import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {getAudit} from 'app/reducers/service';
import {getService} from 'app/reducers/index';
import {resetServiceAudit, fetchServiceAudit} from 'app/actions/service';

import Preloader from 'app/components/preloader/preloader';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';
import Bem from 'app/components/bem/bem';
import InfiniteList from 'app/components/infinite-list/infinite-list';
import ServiceAuditItem from './__item/service-audit__item';

import {serviceType} from 'app/types';

import i18n from 'app/lib/i18n';

import './service-audit.css';

class ServiceAudit extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            byLabelId: true
        };

        this.onChangeByLabelId = this.onChangeByLabelId.bind(this);
        this.fetchServiceAudit = this.fetchServiceAudit.bind(this);
    }

    componentWillUnmount() {
        this.props.resetServiceAudit(); // хотим актуальные данные
    }

    onChangeByLabelId() {
        this.setState(state => ({
            byLabelId: !state.byLabelId
        }), () => {
            this.props.resetServiceAudit();
        });
    }

    fetchServiceAudit(offset, limit) {
        const labelId = this.state.byLabelId ? this.props.labelId : undefined;
        return this.props.fetchServiceAudit(offset, limit, labelId);
    }

    render() {
        return (
            <Bem
                block='service-audit'>
                <Bem
                    block='service-audit'
                    elem='header'>
                    <Bem
                        key='filters'
                        block='service-audit'
                        elem='filters'>
                        <Checkbox
                            theme='normal'
                            size='s'
                            tone='grey'
                            view='default'
                            checked={this.state.byLabelId}
                            onChange={this.onChangeByLabelId}>
                            {i18n('service-page', 'only-current-label')}
                        </Checkbox>
                    </Bem>
                </Bem>
                {!this.props.loaded ?
                    <Preloader /> :
                    ''}
                {this.props.service ?
                    <InfiniteList
                        key='list'
                        wrapperMix={{
                            block: 'service-audit',
                            elem: 'scroll-wrapper'
                        }}
                        requestMore={this.fetchServiceAudit}
                        emptyLabel={i18n('service-page', 'empty-list')}
                        offset={this.props.offset}
                        total={this.props.total}
                        items={this.props.audit}
                        Item={ServiceAuditItem}
                        filters={{update: this.state.byLabelId}} /> :
                    ''
                }
            </Bem>
        );
    }
}

ServiceAudit.propTypes = {
    service: serviceType.isRequired,
    audit: PropTypes.array,
    loaded: PropTypes.bool,
    offset: PropTypes.number,
    total: PropTypes.number,
    resetServiceAudit: PropTypes.func,
    fetchServiceAudit: PropTypes.func,
    labelId: PropTypes.string
};

export default connect(state => {
    const service = getService(state);
    const audit = getAudit(service);

    return {
        audit: audit.items,
        loaded: audit.loaded,
        offset: audit.offset,
        total: audit.total
    };
}, (dispatch, props) => {
    return {
        resetServiceAudit: () => {
            return dispatch(resetServiceAudit(props.service.id));
        },
        fetchServiceAudit: (offset, limit, labelId) => {
            return dispatch(fetchServiceAudit(props.service.id, offset, limit, labelId));
        }
    };
})(ServiceAudit);
