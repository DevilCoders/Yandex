import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import './config-editor__error.css';

const ConfigGroupEditor = props => {
    const {
        visible,
        text,
        hasSpaceTop,
        hasRoundBottom,
        tagRef
    } = props;

    if (!visible) {
        return '';
    }

    return (
        <Bem
            block='config-error-editor'
            tagRef={tagRef}
            mix={{
                block: 'config-error-editor',
                elem: 'body',
                mods: {
                    'space-top': hasSpaceTop,
                    'round-bottom': hasRoundBottom
                }
            }}>
            {text}
        </Bem>
    );
};

ConfigGroupEditor.propTypes = {
    tagRef: PropTypes.func,
    visible: PropTypes.bool,
    hasSpaceTop: PropTypes.bool,
    hasRoundBottom: PropTypes.bool,
    text: PropTypes.string

};

export default ConfigGroupEditor;
