import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import ObjectDiff from 'app/components/object-diff/object-diff';
import i18n from 'app/lib/i18n';
import './config-diff-page.css';
import {connect} from 'react-redux';
import {getService} from 'app/reducers';
import {getConfigs} from 'app/reducers/service';
import {fetchConfig} from 'app/actions/service';

class ConfigDiffPage extends React.Component {
    componentDidMount() {
        if (this.props.id && this.props.secondId) {
            this.props.fetchConfig(this.props.serviceId, this.props.id);
            this.props.fetchConfig(this.props.serviceId, this.props.secondId);
        }
    }

    componentWillReceiveProps(nextProps) {
        const idExistsAndDiffers = nextProps.id && nextProps.id !== this.props.id;
        const secondIdExistsAndDiffers = nextProps.secondId && nextProps.secondId !== this.props.secondId;

        // Запрашиваем конфиг только если он есть и если он поменялся
        if (idExistsAndDiffers) {
            this.props.fetchConfig(nextProps.serviceId, nextProps.id);
        }

        if (secondIdExistsAndDiffers) {
            this.props.fetchConfig(nextProps.serviceId, nextProps.secondId);
        }
    }

    render() {
        const firstConfig = this.props.configsData[this.props.id] || {};
        const secondConfig = this.props.configsData[this.props.secondId] || {};
        const loaded = Boolean(this.props.id && firstConfig && this.props.secondId && secondConfig);

        return (
            <Bem
                block='config-diff-page'>
                <Bem
                    key='header'
                    block='service-page'
                    elem='header'>
                    {i18n('service-page', 'config-diff-header')}
                </Bem>
                <ObjectDiff
                    firstObj={firstConfig.data}
                    secondObj={secondConfig.data}
                    loaded={loaded} />
            </Bem>
        );
    }
}

ConfigDiffPage.propTypes = {
    id: PropTypes.number.isRequired,
    secondId: PropTypes.number.isRequired,
    serviceId: PropTypes.string.isRequired,
    fetchConfig: PropTypes.func.isRequired,
    configsData: PropTypes.object
};

export default connect(state => {
    return {
        configsData: getConfigs(getService(state)).byId
    };
}, dispatch => {
    return {
        fetchConfig: (serviceid, configId) => {
            dispatch(fetchConfig(serviceid, configId));
        }
    };
})(ConfigDiffPage);
