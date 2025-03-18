import React from 'react';
import PropTypes from 'prop-types';
import Bem from 'app/components/bem/bem';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';

import './config-yaml-editor.css';

class ConfigYamlEditor extends React.Component {
    constructor() {
        super();

        this._anchor = {};

        this.setAnchorRef = this.setAnchorRef.bind(this);
    }

    setAnchorRef(ref) {
        this._anchor = ref;
    }

    handleErrors() {
        const lines = this.props.data.split('\n');
        const validation = Object.values(this.props.validation || {}).reduce((validation, errors) => {
            return [...validation, ...errors];
        }, []);

        return validation
            .map(property => {
                let lineNumber = 0,
                    location = null;

                for (let i = 1; i < property.path.length; i++) {
                    let currentLine = lineNumber,
                        currentPathItem = property.path[i];

                    if (typeof currentPathItem === 'number') {
                        currentLine += currentPathItem + 1;
                    } else {
                        const escapedItem = currentPathItem.replace(/[-/\\^$*+?.()|[\]{}]/g, '\\$&');
                        const regexp = new RegExp(`^'?${escapedItem}'?:`);
                        while (currentLine < lines.length) {
                            if (regexp.test(lines[currentLine].trim())) {
                                break;
                            }
                            currentLine++;
                        }
                    }

                    if (currentLine < lines.length) {
                        lineNumber = currentLine;
                    }
                }

                if (lineNumber < lines.length) {
                    let startPos = lines[lineNumber].length - lines[lineNumber].trimLeft().length + 1,
                        endPos = startPos + lines[lineNumber].trim().length;

                    location = {
                        start: {
                            line: lineNumber + 1,
                            column: startPos
                        },
                        end: {
                            line: lineNumber + 1,
                            column: endPos
                        }
                    };
                }

                return {
                    location: location,
                    message: property.message
                };
            });
    }

    hasError(errMsg) {
        return errMsg !== '';
    }

    getErrorMessage() {
        return Object.values(this.props.validation || {})
            .reduce((errMsg, errors) => (
                errMsg.concat(errors
                    .map(error => error.message)
                    .join('\n')
                )
            ), []).join('\n');
    }

    render() {
        const errMsg = this.getErrorMessage();
        const errors = this.handleErrors();

        return (
            <Bem
                block='config-yaml-editor'>
                {this.hasError(errMsg) ?
                    <Tooltip
                        anchor={this._anchor}
                        view='classic'
                        tone='default'
                        theme='error'
                        to='top'
                        mix={{
                            block: 'config-yaml-editor',
                            elem: 'tooltip'
                        }}
                        visible>
                        {errMsg}
                    </Tooltip> :
                    null}
                <Bem
                    block='config-yaml-editor'
                    elem='body'
                    key='body'>
                    <Bem

                        block='config-yaml-editor'
                        elem='textarea'
                        ref={this.setAnchorRef}>
                        <MonacoEditor
                            errors={errors}
                            language='yaml'
                            options={{
                                readOnly: this.props.readOnly,
                                quickSuggestions: {
                                    other: false,
                                    comments: false,
                                    strings: true
                                },
                                automaticLayout: true,
                                scrollBeyondLastLine: false
                            }}
                            keywords={this.props.keywords}
                            value={this.props.data}
                            onChange={this.props.onChange} />
                    </Bem>
                    <Bem
                        block='config-yaml-editor'
                        elem='textarea'>
                        <MonacoEditor
                            language='yaml'
                            options={{
                                readOnly: this.props.readOnly,
                                quickSuggestions: {
                                    other: false,
                                    comments: false,
                                    strings: true
                                },
                                automaticLayout: true,
                                scrollBeyondLastLine: false
                            }}
                            keywords={this.props.keywords}
                            value={this.props.dataSettings}
                            onChange={this.props.onChangeDataSettings} />
                    </Bem>
                </Bem>
            </Bem>
        );
    }
}

ConfigYamlEditor.propTypes = {
    data: PropTypes.string,
    validation: PropTypes.array,
    onChange: PropTypes.func,
    keywords: PropTypes.array,
    dataSettings: PropTypes.string,
    onChangeDataSettings: PropTypes.func,
    readOnly: PropTypes.bool
};

export default ConfigYamlEditor;
