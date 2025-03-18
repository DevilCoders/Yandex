import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import DiffModal from 'app/components/diff-modal/diff-modal';

import {closeProfileDiffModal} from 'app/actions/profile-diff-modal';
import {fetchServiceSbsProfile} from 'app/actions/service/sbs';
import {getProfileDiffModal, getService} from 'app/reducers';
import {serviceSbsProfilesType} from 'app/types';
import {getSbsProfiles} from 'app/reducers/service';

class ConfigDiffModal extends React.Component {
    constructor() {
        super();

        this.onModalClose = this.onModalClose.bind(this);
    }

    onModalClose() {
        this.props.closeProfileDiffModal();
    }

    componentDidMount() {
        const {
            id,
            secondId,
            fetchServiceSbsProfile,
            serviceId
        } = this.props;

        if (id && secondId) {
            fetchServiceSbsProfile(serviceId, {profileId: id});
            fetchServiceSbsProfile(serviceId, {profileId: secondId});
        }
    }

    componentWillReceiveProps(nextProps) {
        const idExistsAndDiffers = Number.isFinite(nextProps.id) && nextProps.id !== this.props.id;
        const secondIdExistsAndDiffers = Number.isFinite(nextProps.secondId) && nextProps.secondId !== this.props.secondId;

        // Запрашиваем профиль только если он есть и если он поменялся
        if (idExistsAndDiffers && !this.props.allSbsProfiles.byId[nextProps.id]) {
            this.props.fetchServiceSbsProfile(nextProps.serviceId, {profileId: nextProps.id});
        }

        if (secondIdExistsAndDiffers && !this.props.allSbsProfiles.byId[nextProps.secondId]) {
            this.props.fetchServiceSbsProfile(nextProps.serviceId, {profileId: nextProps.secondId});
        }
    }

    getLinkDiffConfigToCopyUrl() {
        const {serviceId, id, secondId} = this.props;

        return `https://${location.host}/service/${serviceId}/sbs-profiles/diff/${id}/${secondId}`;
    }

    render() {
        const {
            id,
            secondId,
            visible,
            allSbsProfiles
        } = this.props;
        const firstProfile = allSbsProfiles.byId[id];
        const secondProfile = allSbsProfiles.byId[secondId];
        const loaded = Boolean(Number.isFinite(id) && firstProfile && Number.isFinite(secondId) && secondProfile);

        return (
            <DiffModal
                loaded={loaded}
                firstObj={firstProfile ? firstProfile.data : {}}
                secondObj={secondProfile ? secondProfile.data : {}}
                closeDiffModal={this.onModalClose}
                urlDiff={this.getLinkDiffConfigToCopyUrl()}
                visible={visible} />
        );
    }
}

ConfigDiffModal.propTypes = {
    id: PropTypes.number,
    secondId: PropTypes.number,
    visible: PropTypes.bool,
    serviceId: PropTypes.string,
    fetchServiceSbsProfile: PropTypes.func.isRequired,
    closeProfileDiffModal: PropTypes.func.isRequired,
    allSbsProfiles: serviceSbsProfilesType.isRequired
};

export default connect(state => {
    const service = getService(state);
    const profileDiffModal = getProfileDiffModal(state);

    return {
        id: profileDiffModal.id,
        secondId: profileDiffModal.secondId,
        visible: profileDiffModal.modalVisible,
        serviceId: profileDiffModal.serviceId,
        allSbsProfiles: getSbsProfiles(service)
    };
}, dispatch => {
    return {
        closeProfileDiffModal: () => {
            dispatch(closeProfileDiffModal());
        },
        fetchServiceSbsProfile: (serviceId, params) => {
            return dispatch(fetchServiceSbsProfile(serviceId, params));
        }
    };
})(ConfigDiffModal);
