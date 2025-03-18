import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Preloader from 'app/components/preloader/preloader';
import SearchSelect from 'app/components/search-select/search-select';

import 'app/components/icon/_theme/icon_theme_search.css';
import './service-sbs-profile__tags.css';

export default class ServiceSbsProfileTags extends React.Component {
    render() {
        const {loading, loaded, data} = this.props.profileTags;

        return (
            <Bem block='services-sbs-profile-tags'>
                {loading && !loaded && <Preloader />}
                <SearchSelect
                    selectItems={data}
                    selectedItemText={this.props.selectedTag}
                    onChangeSelect={this.props.onChangeTag} />
            </Bem>
        );
    }
}

ServiceSbsProfileTags.propTypes = {
    profileTags: PropTypes.any,
    selectedTag: PropTypes.string,
    onChangeTag: PropTypes.func
};
