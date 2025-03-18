import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Bem from 'app/components/bem/bem';
import ServiceRulesTable from './__table/service-rules__table';
import ServiceRulesAction from './__action/service-rules__action';
import Preloader from 'app/components/preloader/preloader';
import Splitter from 'app/components/splitter/splitter';

import i18n from 'app/lib/i18n';
import {formatDateTo} from 'app/lib/date';

import {getService} from 'app/reducers';
import {getRules} from 'app/reducers/service';
import {fetchServiceRules} from 'app/actions/service/rules';
import {serviceType, serviceRulesType} from 'app/types';

import './service-rules.css';

const INTERVAL_TIME = 60 * 1000;

class ServiceRules extends React.Component {
    constructor() {
        super();

        this.state = JSON.parse(localStorage.getItem('action-rule-tags') || JSON.stringify({
            main: {
                onlyServiceRules: false
            },
            adblocks: {
                adblock: false,
                adguard: false,
                ublock: false,
                opera: false
            },
            devices: {
                desktop: false,
                mobile: false
            }
        }));

        this._timer = null;
        this.renderRightComponent = this.renderRightComponent.bind(this);
        this.renderLeftComponent = this.renderLeftComponent.bind(this);
        this.onConfirmTags = this.onConfirmTags.bind(this);
    }

    componentDidMount() {
        const {
            service,
            fetchServiceRules
        } = this.props;
        const tags = this.getTags(this.state);

        fetchServiceRules(service.id, tags);
        this._timer = setInterval(() => fetchServiceRules(service.id, tags), INTERVAL_TIME);
    }

    componentWillUnmount() {
        clearInterval(this._timer);
    }

    prepareData(data, onlyServiceRules) {
        // Если не копировать объект, то после выставления флажка is_partner_rule
        // либа не может отформатировать уже отформатированную строку и получем мусор вместо даты
        let copyData = data.map(item => (JSON.parse(JSON.stringify(item))));

        copyData = onlyServiceRules ? copyData.filter(item => (item.is_partner_rule)) : copyData;
        this.formatData(copyData);
        return copyData;
    }

    clearUrls(urls) {
        return urls.map(url => (url.replace(/^(https?:)?\/\//, '')));
    }

    formatData(items) {
        if (!items.length) {
            return;
        }

        const format = 'DD.MM.YYYY\nHH:mm';
        items[0].added = formatDateTo(items[0].added, format);
        items[0].list_url = this.clearUrls(items[0].list_url);

        for (let i = 1, lastEqI = 0; i < items.length; i++) {
            const date = formatDateTo(items[i].added, format);
            items[i].list_url = this.clearUrls(items[i].list_url);

            if (items[lastEqI].added === date) {
                items[i].added = '';
            } else {
                items[i].added = date;
                lastEqI = i;
            }
        }
    }

    getTags(obj) {
        return Object.keys(obj).reduce((acc, i) => {
            switch (i) {
                case 'main':
                    return acc;
                case 'adblocks':
                    return acc.concat(Object.keys(obj[i]).reduce((acc, j) => {
                        if (obj[i][j]) {
                            acc.push(j === 'adblock' ? 'AdblockPlus' : j[0].toUpperCase() + j.slice(1));
                        }

                        return acc;
                    }, []));
                default:
                    return acc.concat(Object.keys(obj[i]).filter(j => obj[i][j]));
            }
        }, []);
    }

    onConfirmTags(state) {
        localStorage.setItem('action-rule-tags', JSON.stringify(state));

        if (JSON.stringify({...state, main: true}) === JSON.stringify({...this.state, main: true})) {
            if (state.main.onlyServiceRules !== this.state.main.onlyServiceRules) {
                this.setState(state => ({
                    main: {
                        ...state.main,
                        onlyServiceRules: !state.main.onlyServiceRules
                    }
                }));
            }

            return;
        }

        this.setState(state, () => {
            const {
                service,
                fetchServiceRules
            } = this.props;
            const tags = this.getTags(state);

            clearInterval(this._timer);

            fetchServiceRules(service.id, tags);
            this._timer = setInterval(() => fetchServiceRules(service.id, tags), INTERVAL_TIME);
        });
    }

    renderWarning(key) {
        return (
            <Bem
                key='empty-table'
                block='service-rules'
                elem='empty'>
                {i18n('service-rules', key)}
            </Bem>
        );
    }

    renderRightComponent() {
        const {
            rules
        } = this.props;
        const rulesData = this.prepareData(rules.data.items, this.state.main.onlyServiceRules);

        return (
            <Bem
                block='service-rules'
                elem='body'>
                {!rules.loaded &&
                    <Preloader />}
                {rules.loaded && !rulesData.length ?
                    this.renderWarning('empty') :
                    <ServiceRulesTable
                        data={rulesData}
                        schema={rules.schema} />}
            </Bem>
        );
    }

    renderLeftComponent() {
        return (
            <ServiceRulesAction
                onConfirmAction={this.onConfirmTags}
                data={this.state}
                customTitle={{
                    onlyServiceRules: 'only-partner-rules'
                }} />
        );
    }

    render() {
        return (
            <Bem
                block='service-rules'>
                <Splitter
                    componentLocalStorageName='service-rules'
                    leftComponent={this.renderLeftComponent()}
                    rightComponent={this.renderRightComponent()}
                    minLeftWidth={-1}
                    leftWidth={180}
                    maxLeftWidth={270} />
            </Bem>
        );
    }
}

ServiceRules.propTypes = {
    service: serviceType,
    fetchServiceRules: PropTypes.func.isRequired,
    rules: serviceRulesType
};

export default connect(state => {
    return {
        rules: getRules(getService(state))
    };
}, dispatch => {
    return {
        fetchServiceRules: (serviceId, tags) => {
            return dispatch(fetchServiceRules(serviceId, tags));
        }
    };
})(ServiceRules);

