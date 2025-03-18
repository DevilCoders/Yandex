import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import {closeConfigDiffModal} from 'app/actions/config-diff-modal';
import {fetchConfig} from 'app/actions/service';
import {getConfigDiffModal, getService} from 'app/reducers';

import {getConfigs} from 'app/reducers/service';
import DiffModal from 'app/components/diff-modal/diff-modal';

class ConfigDiffModal extends React.Component {
    constructor() {
        super();

        this.onModalClose = this.onModalClose.bind(this);
        this.getLinkDiffConfigToCopyUrl = this.getLinkDiffConfigToCopyUrl.bind(this);
    }

    onModalClose() {
        this.props.closeConfigDiffModal();
    }

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

    getLinkDiffConfigToCopyUrl() {
        const {serviceId, id, secondId} = this.props;

        return `https://${location.host}/service/${serviceId}/configs/diff/${id}/${secondId}`;
    }

    render() {
        const firstConfig = this.props.configsData[this.props.id];
        const secondConfig = this.props.configsData[this.props.secondId];
        const loaded = Boolean(this.props.id && firstConfig && this.props.secondId && secondConfig);

        return (
            <DiffModal
                loaded={loaded}
                firstObj={firstConfig ? firstConfig.data : {}}
                secondObj={secondConfig ? secondConfig.data : {}}
                closeDiffModal={this.onModalClose}
                urlDiff={this.getLinkDiffConfigToCopyUrl()}
                visible={this.props.visible} />
        );
    }
}

ConfigDiffModal.propTypes = {
    id: PropTypes.number,
    secondId: PropTypes.number,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    closeConfigDiffModal: PropTypes.func.isRequired,
    fetchConfig: PropTypes.func.isRequired,
    configsData: PropTypes.object
};

export default connect(state => {
    return {
        id: getConfigDiffModal(state).id,
        secondId: getConfigDiffModal(state).secondId,
        visible: getConfigDiffModal(state).modalVisible,
        serviceId: getConfigDiffModal(state).serviceId,
        configsData: getConfigs(getService(state)).byId
    };
}, dispatch => {
    return {
        closeConfigDiffModal: () => {
            dispatch(closeConfigDiffModal());
        },
        fetchConfig: (serviceId, configId) => {
            dispatch(fetchConfig(serviceId, configId));
        }
    };
})(ConfigDiffModal);
