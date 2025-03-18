import React from 'react';
import PropTypes from 'prop-types';
import {connect} from 'react-redux';

import Bem from 'app/components/bem/bem';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import Button from 'lego-on-react/src/components/button/button.react';
import ModalClose from 'app/components/modal/__close/modal__close';
import Preloader from 'app/components/preloader/preloader';
import ProfileYamlEditor from './../profile-yaml-editor/profile-yaml-editor';
import ServiceSbsProfileTags from './__tags/service-sbs-profile__tags';
import TextInput from 'lego-on-react/src/components/textinput/textinput.react';

import {serviceSbsProfileType, serviceSbsProfilesType, serviceSbsProfileTagsType} from 'app/types';
import {getSbsProfile, getSbsProfiles, getSbsProfileById, getSbsProfilesTags} from 'app/reducers/service';
import {getService} from 'app/reducers/index';
import {
    fetchServiceSbsProfile,
    deleteServiceSbsProfileByTag,
    saveServiceSbsProfile,
    closeServiceSbsProfile,
    fetchServiceSbsProfileTags
} from 'app/actions/service/sbs';

import merge from 'lodash/merge';
import i18n from 'app/lib/i18n';
import {DEFAULT_TAG} from 'app/lib/sbs/profile-tags';
import YAML from 'js-yaml';

import './service-sbs-profile.css';

const DEFAULT_TEXT_NAME = '';

class ServiceSbsProfile extends React.Component {
    constructor(props) {
        super(props);

        this.state = {
            groupIsVisible: false,
            data: props.sbsProfile.data,
            text: YAML.safeDump(props.sbsProfile.data || {}, {
                lineWidth: Infinity
            }),
            isValid: true,
            isCreateTag: false,
            newConfigTextName: DEFAULT_TEXT_NAME,
            selectedTag: props.selectedTag || DEFAULT_TAG,
            hasTagError: false,
            validation: []
        };

        this._refs = {};

        this.onChangeVisible = this.onChangeVisible.bind(this);
        this.onChangeVisibleCreateTag = this.onChangeVisibleCreateTag.bind(this);
        this.onChangeConfigTextName = this.onChangeConfigTextName.bind(this);
        this.onChangeSelectedTag = this.onChangeSelectedTag.bind(this);
        this.setRef = this.setRef.bind(this);

        this.onModalClose = this.onModalClose.bind(this);
        this.onSave = this.onSave.bind(this);
        this.onChangeData = this.onChangeData.bind(this);
        this.onError = this.onError.bind(this);
        this.deleteProfileByTag = this.deleteProfileByTag.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        let state = {};

        if (nextProps.sbsProfile.loaded && !nextProps.allSbsProfiles.validation.length && !nextProps.allSbsProfiles.saving) {
            state = {
                ...nextProps.sbsProfile,
                text: YAML.safeDump(nextProps.sbsProfile.data || {}, {
                    lineWidth: Infinity
                }),
                isValid: true
            };
        }

        // не делаем isEqual, т.к. смотрим пришел ли именно тот же массив с валидацией, что и был до этого
        // с бекенда может вернуться несколько раз одна и таже ошибка, и нужно каждый раз ее подсвечивать.
        if (nextProps.allSbsProfiles.validation.length && this.props.allSbsProfiles.validation !== nextProps.allSbsProfiles.validation) {
            state.isValid = false;
            state.validation = nextProps.allSbsProfiles.validation;
        }

        if (!this.props.allSbsProfiles.isOpen && nextProps.allSbsProfiles.isOpen) {
            state.selectedTag = nextProps.selectedTag;
        } else {
            if (((
                this.props.allSbsProfiles.saving &&
                !nextProps.allSbsProfiles.saving &&
                !nextProps.allSbsProfiles.validation.length) || (
                !nextProps.allSbsProfiles.validation.length &&
                this.props.allSbsProfiles.validation.length &&
                !nextProps.allSbsProfiles.saving)) &&
                !nextProps.allSbsProfiles.loading && !this.props.allSbsProfiles.loading
            ) {
                state.isCreateTag = false;
                state.newConfigTextName = DEFAULT_TEXT_NAME;
                state.selectedTag = DEFAULT_TAG;
            }

            if (
                !nextProps.allSbsProfiles.deleting &&
                this.props.allSbsProfiles.deleting &&
                !nextProps.allSbsProfiles.validation.length
            ) {
                state.isCreateTag = false;
                state.newConfigTextName = DEFAULT_TEXT_NAME;
                state.selectedTag = DEFAULT_TAG;
                this.props.fetchServiceSbsProfile(nextProps.serviceId, {tag: DEFAULT_TAG});
                this.props.fetchServiceSbsProfileTags(nextProps.serviceId);
            }
        }

        this.setState({
            ...state
        });
    }

    setRef(name) {
        return value => {
            this._refs[name] = value;
        };
    }

    onChangeVisibleCreateTag() {
        this.setState(state => ({
            isCreateTag: !state.isCreateTag,
            hasTagError: false,
            newConfigTextName: ''
        }), () => {
            if (this.state.isCreateTag) {
                setTimeout(() => {
                    this._refs['services-sbs-profile-input-new-tag'].focus();
                }, 0);
            }
        });
    }

    onChangeVisible() {
        this.setState(state => ({
            groupIsVisible: !state.groupIsVisible
        }));
    }

    onChangeSelectedTag(val) {
        this.setState({
            selectedTag: val
        }, () => {
            this.props.fetchServiceSbsProfile(this.props.serviceId, {tag: val});
        });
    }

    componentWillUpdate(nextProps) {
        const {
            allSbsProfiles,
            fetchServiceSbsProfile
        } = this.props;
        const {
            isOpen
        } = allSbsProfiles;
        const {
            openedProfileId: nextOpenedProfileId,
            isOpen: nextIsOpen
        } = nextProps.allSbsProfiles;
        const preview = nextOpenedProfileId ? nextProps.allSbsProfiles.byId[nextOpenedProfileId] : false;

        if (!preview && !isOpen && nextIsOpen) {
            fetchServiceSbsProfile(nextProps.serviceId, {profileId: nextOpenedProfileId, tag: this.state.selectedTag});
        }
    }

    onModalClose() {
        const {
            closeServiceSbsProfile
        } = this.props;

        this.setState({
            newConfigTextName: DEFAULT_TEXT_NAME,
            selectedTag: DEFAULT_TAG,
            hasTagError: false,
            isCreateTag: false
        }, () => {
            closeServiceSbsProfile();
        });
    }

    onChangeData(data, text, isValid) {
        if (isValid) {
            this.setState({
                data,
                text,
                isValid
            });
        }
    }

    onChangeConfigTextName(value) {
        this.setState({
            newConfigTextName: value,
            isValid: true,
            validation: []
        });
    }

    onSave() {
        const {
            saveServiceSbsProfile,
            serviceId
        } = this.props;

        if (this.state.isCreateTag && !this.state.newConfigTextName) {
            this.setState({
                hasTagError: true
            });
        } else {
            saveServiceSbsProfile(serviceId, this.state.data, this.state.newConfigTextName || this.state.selectedTag);
        }
    }

    onError(value) {
        if (this.state.isValid !== value) {
            this.setState({
                isValid: value
            });
        }
    }

    getYamlKeywords() {
        const {
            sbsProfile
        } = this.props;

        if (sbsProfile) {
            const keysTopLvl = Object.keys(sbsProfile.data);
            const keysGeneralSettings = Object.keys(sbsProfile.data.general_settings);
            const keysUrlSettings = (sbsProfile.data.url_settings && sbsProfile.data.url_settings[0] && Object.keys(sbsProfile.data.url_settings[0])) || [];

            return merge(keysTopLvl, keysGeneralSettings, keysUrlSettings);
        }

        return {};
    }

    deleteProfileByTag() {
        if (this.state.selectedTag !== DEFAULT_TAG) {
            this.props.deleteServiceSbsProfileByTag(this.props.serviceId, this.state.selectedTag);
        } else {
            this.setState({
                validation: [{
                    message: i18n('sbs', 'delete-default-tag'),
                    path: []
                }]
            });
        }
    }

    renderActions() {
        const {
            allSbsProfiles
        } = this.props;
        const {
            readOnly,
            saving: savingProfile
        } = allSbsProfiles;

        return (
            <Bem
                key='actions'
                block='modal'
                elem='actions'
                mix={{
                    block: 'service-sbs-profile',
                    elem: 'actions'
                }}>
                <Button
                    theme='link'
                    view='default'
                    tone='grey'
                    size='s'
                    mix={{
                        block: 'modal',
                        elem: 'action'
                    }}
                    progress={savingProfile}
                    onClick={this.onSave}
                    disabled={readOnly || !this.state.isValid}>
                    {i18n('common', 'save')}
                </Button>
                <Button
                    theme='normal'
                    view='default'
                    tone='grey'
                    size='s'
                    mix={{
                        block: 'modal',
                        elem: 'action'
                    }}
                    onClick={this.onModalClose}>
                    {i18n('common', 'close')}
                </Button>
                {!this.state.isCreateTag &&
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onChangeVisibleCreateTag}>
                        {i18n('sbs', 'create-tag')}
                    </Button>}
                {this.state.isCreateTag &&
                    <Bem
                        block='service-sbs-profile'
                        elem='input-new-tag'
                        mods={{
                            error: this.state.hasTagError
                        }}>
                        <TextInput
                            theme='normal'
                            tone='grey'
                            view='default'
                            size='m'
                            ref={this.setRef('services-sbs-profile-input-new-tag')}
                            text={this.state.newConfigTextName}
                            onChange={this.onChangeConfigTextName} />
                    </Bem>}
                {this.state.isCreateTag &&
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={this.onChangeVisibleCreateTag}>
                        {i18n('common', 'cancel')}
                    </Button>}
                <Button
                    view='default'
                    tone='red'
                    size='s'
                    theme='normal'
                    onClick={this.deleteProfileByTag}
                    disable={this.props.allSbsProfiles.deleting}
                    mix={[{
                        block: 'modal',
                        elem: 'action'
                    }, {
                        block: 'service-sbs-profile',
                        elem: 'action-delete'
                    }]}>
                    {i18n('sbs', 'delete-tag')}
                </Button>
            </Bem>
        );
    }

    render() {
        const {
            sbsProfile,
            allSbsProfiles
        } = this.props;
        const {
            isOpen,
            readOnly
        } = allSbsProfiles;

        if (!isOpen) {
            return null;
        }

        return (
            (sbsProfile && sbsProfile.loaded) ? (
                <Modal
                    autoclosable
                    onDocKeyDown={this.onModalClose}
                    onOutsideClick={this.onModalClose}
                    visible>
                    <Bem
                        key='title'
                        block='modal'
                        elem='title'
                        mix={{
                           block: 'service-sbs-profile',
                           elem: 'title'
                        }}>
                        <Bem
                            block='service-sbs-profile'
                            elem='title-text'>
                            {i18n('sbs', 'profile-title')}
                        </Bem>
                        <ServiceSbsProfileTags
                            profileTags={this.props.profileTags}
                            selectedTag={this.state.selectedTag}
                            onChangeTag={this.onChangeSelectedTag} />
                    </Bem>
                    <ModalClose
                        key='close'
                        onClick={this.onModalClose} />
                    <Bem
                        key='body'
                        block='modal'
                        elem='body'>
                        <ProfileYamlEditor
                            validation={this.state.validation}
                            data={this.state.text}
                            readOnly={readOnly}
                            onChange={this.onChangeData}
                            onError={this.onError}
                            keywords={this.getYamlKeywords()} />
                    </Bem>
                    {this.renderActions()}
                </Modal>) :
                <Preloader />
        );
    }
}

ServiceSbsProfile.propTypes = {
    serviceId: PropTypes.string.isRequired,
    sbsProfile: serviceSbsProfileType.isRequired,
    allSbsProfiles: serviceSbsProfilesType.isRequired,
    closeServiceSbsProfile: PropTypes.func.isRequired,
    fetchServiceSbsProfile: PropTypes.func.isRequired,
    saveServiceSbsProfile: PropTypes.func.isRequired,
    fetchServiceSbsProfileTags: PropTypes.func.isRequired,
    deleteServiceSbsProfileByTag: PropTypes.func.isRequired,
    profileTags: serviceSbsProfileTagsType.isRequired,
    selectedTag: PropTypes.string
};

export default connect(state => {
    const service = getService(state);
    const sbsProfiles = getSbsProfiles(service);

    return {
        allSbsProfiles: sbsProfiles,
        sbsProfile: sbsProfiles.readOnly ?
            getSbsProfileById(service, sbsProfiles.openedProfileId) :
            getSbsProfile(service),
        profileTags: getSbsProfilesTags(service)
    };
}, dispatch => {
    return {
        closeServiceSbsProfile: () => {
            dispatch(closeServiceSbsProfile());
        },
        fetchServiceSbsProfile: (serviceId, params) => {
            return dispatch(fetchServiceSbsProfile(serviceId, params));
        },
        fetchServiceSbsProfileTags: serviceId => (
            dispatch(fetchServiceSbsProfileTags(serviceId))
        ),
        saveServiceSbsProfile: (serviceId, data, tag) => {
            return dispatch(saveServiceSbsProfile(serviceId, data, tag));
        },
        deleteServiceSbsProfileByTag: (serviceId, tag) => {
            return dispatch(deleteServiceSbsProfileByTag(serviceId, tag));
        }
    };
})(ServiceSbsProfile);
