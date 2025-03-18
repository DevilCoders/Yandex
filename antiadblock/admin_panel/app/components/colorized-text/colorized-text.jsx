import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import './colorized-text.css';

class ColorizedText extends React.Component {
    render() {
        const color = this.props.color || 'default';

        return (
            <Bem
                block='colorized-text'
                mods={{
                    color
                }}>
                {this.props.children}
            </Bem>
        );
    }
}

ColorizedText.propTypes = {
    color: PropTypes.string,
    children: PropTypes.oneOfType([
        PropTypes.array,
        PropTypes.object,
        PropTypes.string
    ])
};

export default ColorizedText;
