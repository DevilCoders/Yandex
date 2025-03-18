import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Spin from 'lego-on-react/src/components/spin/spin.react';
import './preloader.css';

export default class Preloader extends React.Component {
    render() {
        return (
            <Bem
                block='preloader'
                mods={{
                    fit: this.props.fit
                }}>
                <Spin
                    mix={{
                        block: 'preloader',
                        elem: 'spin'
                    }}
                    size='m'
                    progress />
            </Bem>
        );
    }
}

Preloader.propTypes = {
    fit: PropTypes.oneOf([
        'viewport'
    ])
};
