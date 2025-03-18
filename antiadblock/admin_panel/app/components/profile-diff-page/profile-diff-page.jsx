import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import ObjectDiff from 'app/components/object-diff/object-diff';
import i18n from 'app/lib/i18n';
import './profile-diff-page.css';
import {connect} from 'react-redux';
import {getService} from 'app/reducers';
import {getSbsProfiles} from 'app/reducers/service';
import {serviceSbsProfilesType} from 'app/types';
import {fetchServiceSbsProfile} from 'app/actions/service/sbs';

class ProfileDiffPage extends React.Component {
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

    render() {
        const {
            id,
            secondId,
            allSbsProfiles
        } = this.props;
        const firstProfile = allSbsProfiles.byId[id];
        const secondProfile = allSbsProfiles.byId[secondId];
        const loaded = Boolean(this.props.id && firstProfile && this.props.secondId && secondProfile);

        return (
            <Bem
                block='profile-diff-page'>
                <Bem
                    key='header'
                    block='service-page'
                    elem='header'>
                    {i18n('service-page', 'profile-diff-header')}
                </Bem>
                <ObjectDiff
                    firstObj={loaded && firstProfile.data}
                    secondObj={loaded && secondProfile.data}
                    loaded={loaded} />
            </Bem>
        );
    }
}

ProfileDiffPage.propTypes = {
    id: PropTypes.number.isRequired,
    secondId: PropTypes.number.isRequired,
    serviceId: PropTypes.string.isRequired,
    fetchServiceSbsProfile: PropTypes.func.isRequired,
    allSbsProfiles: serviceSbsProfilesType.isRequired
};

export default connect(state => {
    return {
        allSbsProfiles: getSbsProfiles(getService(state))
    };
}, dispatch => {
    return {
        fetchServiceSbsProfile: (serviceId, params) => {
            return dispatch(fetchServiceSbsProfile(serviceId, params));
        }
    };
})(ProfileDiffPage);
