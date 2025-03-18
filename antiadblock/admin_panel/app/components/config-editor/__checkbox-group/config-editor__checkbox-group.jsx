import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Hint from 'app/components/hint/hint';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';
import Checkbox from 'lego-on-react/src/components/checkbox/checkbox.react';

import i18n from 'app/lib/i18n';

import './config-editor__checkbox-group.css';

class ConfigCheckboxGroupEditor extends React.PureComponent {
    validate() {
        let answer = [];

        if (this.props.validation) {
            for (let i = 0; i < this.props.validation.length; i++) {
                const error = this.props.validation[i];
                const path = error.path.join('.');

                if (this.props.path === path) {
                    answer.push(error.message);

                    break;
                }
            }
        }

        return answer.join('\n');
    }

    getOnChangeHandler(value) {
        return () => {
            const list = (this.props.value || []).slice();
            const index = list.indexOf(value);

            if (index === -1) {
                list.push(value);
            } else {
                list.splice(index, 1);
            }

            // make sure we have right sorting
            const newValue = this.props.enum
                .filter(item => list.indexOf(item.value) !== -1)
                .map(item => item.value);

            this.props.onChange(newValue, this.props.path);
        };
    }

    render() {
        const validation = this.validate();
        const value = this.props.value || [];
        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem
                block='config-checkbox-group-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        block='config-checkbox-group-editor'
                        elem='title'>
                        <ConfigEditorTitle
                            title={this.props.title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint}
                            readOnly={this.props.readOnly}
                            setDefaultValue={this.props.setDefaultValue}
                            isDefaultValue={this.props.isDefaultValue} />
                    </Bem>}
                <Bem
                    block='config-checkbox-group-editor'
                    elem='body'>
                    {this.props.enum.map(params => {
                        const checked = value.includes(params.value);
                        return (
                            <Bem
                                key={params.value}
                                block='config-checkbox-group-editor'
                                elem='item'>
                                <Checkbox
                                    theme='normal'
                                    size='s'
                                    tone='grey'
                                    view='default'
                                    checked={checked}
                                    onChange={this.getOnChangeHandler(params.value)}
                                    disabled={this.props.readOnly}>
                                    {i18n('titles', params.title) || params.title}
                                </Checkbox>
                                <Hint
                                    text={i18n('hints', params.hint)}
                                    to='right'
                                    scope={this.context.scope} />
                            </Bem>
                        );
                    })}
                </Bem>
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigCheckboxGroupEditor.contextTypes = {
    scope: PropTypes.object
};

ConfigCheckboxGroupEditor.propTypes = {
    title: PropTypes.string,
    hint: PropTypes.string,
    yamlKey: PropTypes.string,
    path: PropTypes.string,
    enum: PropTypes.array,
    value: PropTypes.any,
    validation: PropTypes.array,
    // placeholder: PropTypes.string,
    readOnly: PropTypes.bool,
    onChange: PropTypes.func,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigCheckboxGroupEditor;
