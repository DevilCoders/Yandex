import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import Tumbler from 'app/components/tumbler/tumbler';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import i18n from 'app/lib/i18n';

import './config-editor__bool.css';

class ConfigBoolEditor extends React.Component {
    constructor() {
        super();

        this.onChange = this.onChange.bind(this);
    }

    onChange() {
        this.props.onChange(!this.props.value, this.props.path);
    }

    render() {
        const value = this.props.value || false;
        const mods = {
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-bool-editor'
                mods={mods}>
                {this.props.title &&
                    <Bem
                        block='config-bool-editor'
                        elem='title'
                        key='title'>
                        <ConfigEditorTitle
                            title={this.props.title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint} />
                    </Bem>}
                <Bem
                    block='config-bool-editor'
                    elem='tumbler'
                    key='tumbler'>
                    <Tumbler
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        onVal='onVal'
                        offVal='offVal'
                        disabled={this.props.readOnly}
                        checked={value}
                        onChange={this.onChange} />
                </Bem>
                {this.props.setDefaultValue &&
                    <Bem
                        block='config-bool-editor'
                        elem='action'>
                        <Bem
                            block='config-bool-editor'
                            elem='description'>
                            {`${i18n('config-editor', 'unset')}:`}
                        </Bem>
                        <Checkbox
                            theme='normal'
                            size='s'
                            tone='grey'
                            view='default'
                            disabled={this.props.readOnly}
                            checked={this.props.isDefaultValue}
                            onChange={this.props.setDefaultValue} />
                    </Bem>}
            </Bem>
        );
    }
}

ConfigBoolEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    yamlKey: PropTypes.string,
    title: PropTypes.string,
    hint: PropTypes.string,
    value: PropTypes.bool,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func
};

export default ConfigBoolEditor;
