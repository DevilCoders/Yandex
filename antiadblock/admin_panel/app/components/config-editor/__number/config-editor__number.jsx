import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ConfigTextEditor from '../__text/config-editor__text';

class ConfigNumberEditor extends React.PureComponent {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    onChange(value) {
        this.props.onChange(parseInt(value, 10), this.props.path);
    }

    render() {
        return (
            <Bem
                block='config-editor-number'>
                <ConfigTextEditor
                    {...this.props}
                    value={Number.isFinite(this.props.value) ? String(this.props.value) : ''}
                    type='number'
                    onChange={this.onChange} />
            </Bem>
        );
    }
}

ConfigNumberEditor.propTypes = {
    title: PropTypes.string,
    hint: PropTypes.string,
    yamlKey: PropTypes.string,
    path: PropTypes.string,
    type: PropTypes.string,
    value: PropTypes.any,
    validation: PropTypes.array,
    placeholder: PropTypes.string,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigNumberEditor;
