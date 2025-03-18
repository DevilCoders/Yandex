import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import TagList from 'app/components/tag-list/tag-list';
import ConfigEditorTitle from 'app/components/config-editor/__title/config-editor__title';
import ConfigEditorError from 'app/components/config-editor/__error/config-editor__error';

import i18n from 'app/lib/i18n';

import './config-editor__tags.css';

class ConfigTagsEditor extends React.PureComponent {
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

    render() {
        const validation = this.validate();
        const mods = {
            'has-errors': Boolean(validation),
            'from-parent': this.props.isFromParentConfig
        };

        return (
            <Bem block='config-tags-editor'
                mods={mods}
                tagRef={this.props.setEditorRef && this.props.setEditorRef(this.props.path)}>
                {this.props.title &&
                    <Bem
                        key='title'
                        block='config-tags-editor'
                        elem='title'>
                        <ConfigEditorTitle
                            title={this.props.title}
                            yamlKey={this.props.yamlKey}
                            hint={this.props.hint}
                            ref={this.setAnchorRef}
                            readOnly={this.props.readOnly}
                            setDefaultValue={this.props.setDefaultValue}
                            isDefaultValue={this.props.isDefaultValue} />
                    </Bem>}
                <TagList
                    value={this.props.value}
                    onChange={this.props.onChange}
                    placeholder={i18n('placeholders', this.props.placeholder) || this.props.placeholder}
                    readOnly={this.props.readOnly} />
                <ConfigEditorError
                    text={validation}
                    hasRoundBottom
                    visible={Boolean(validation)} />
            </Bem>
        );
    }
}

ConfigTagsEditor.propTypes = {
    onChange: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    yamlKey: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.array,
    title: PropTypes.string,
    hint: PropTypes.string,
    readOnly: PropTypes.bool,
    isFromParentConfig: PropTypes.bool,
    isDefaultValue: PropTypes.bool,
    setDefaultValue: PropTypes.func,
    setEditorRef: PropTypes.func
};

export default ConfigTagsEditor;
