import React from 'react';
import PropTypes from 'prop-types';
import Bem from 'app/components/bem/bem';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import Tooltip from 'lego-on-react/src/components/tooltip/tooltip.react';

import './profile-yaml-editor.css';
import YAML from 'js-yaml';

class ProfileYamlEditor extends React.Component {
    constructor(props) {
        super();

        this._anchor = {};
        this.state = {
            validation: [],
            data: props.data
        };

        this.setAnchorRef = this.setAnchorRef.bind(this);
        this.onChange = this.onChange.bind(this);
    }

    componentWillReceiveProps(nextProps) {
        if (this.props.validation !== nextProps.validation) {
            this.handleErrors(nextProps.validation || []);
        }

        if (!this.state.validation.length) {
            this.setState({
                data: nextProps.data
            });
        }
    }

    setAnchorRef(ref) {
        this._anchor = ref;
    }

    getValidationMessage(validation) {
        const map = {};

        return validation.reduce((errMsg, err) => {
            if (!map[err.message]) {
                map[err.message] = true;
                errMsg.push(err.message);
            }

            return errMsg;
        }, []).join('\n');
    }

    getErrors(text, maxIter) {
        let i = 0;
        const validation = [];
        const splittedText = text.split('\n');

        while (i < maxIter) {
            try {
                YAML.safeLoad(splittedText.join('\n'));
                return validation;
            } catch (e) {
                const line = splittedText[e.mark.line] ? e.mark.line : e.mark.line - 1;
                validation.push({
                    location: {
                        start: {
                            line: line + 1,
                            column: 0
                        },
                        end: {
                            line: line + 1,
                            column: splittedText[line].length + 1
                        }
                    },
                    message: 'error parsing yaml'
                });
                splittedText.splice(line, 1);
            }
            i++;
        }

        return validation;
    }

    onChange(text) {
        const state = {};

        try {
            const data = YAML.safeLoad(text);
            this.props.onChange(data, text, true);

            if (this.state.validation.length) {
                state.validation = [];
                state.data = text;
            }
        } catch (e) {
            state.validation = this.getErrors(text, 4);
            state.data = text;
            this.props.onError(false);
        }

        this.setState({
            ...state
        });
    }

    handleErrors(errors) {
        const lines = this.state.data.split('\n');
        const yamlErrors = errors
            .map(property => {
                let lineNumber = 0,
                    location = null;

                for (let i = 0; i < property.path.length; i++) {
                    let currentLine = lineNumber,
                        currentPathItem = property.path[i];

                    if (typeof currentPathItem === 'number') {
                        currentLine += (currentPathItem * 2) + 1;
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

        this.setState({
            validation: yamlErrors
        });
    }

    render() {
        return (
            <Bem
                block='profile-yaml-editor'>
                {this.state.validation && this.state.validation.length ?
                    <Tooltip
                        anchor={this._anchor}
                        view='classic'
                        tone='default'
                        theme='error'
                        to='top'
                        mix={{
                            block: 'profile-yaml-editor',
                            elem: 'tooltip'
                        }}
                        visible>
                        {this.getValidationMessage(this.state.validation)}
                    </Tooltip> :
                    null}
                <Bem
                    key='body'
                    block='profile-yaml-editor'
                    elem='textarea'
                    ref={this.setAnchorRef}>
                    <MonacoEditor
                        editorWillMount={this.editorWillMount}
                        errors={this.state.validation.filter(item => item.location)}
                        language='yaml'
                        options={{
                            quickSuggestions: {
                                other: false,
                                comments: false,
                                strings: true
                            },
                            readOnly: this.props.readOnly,
                            automaticLayout: true,
                            scrollBeyondLastLine: false
                        }}
                        keywords={this.props.keywords}
                        value={this.state.data}
                        onChange={this.onChange} />
                </Bem>
            </Bem>
        );
    }
}

ProfileYamlEditor.propTypes = {
    data: PropTypes.string,
    validation: PropTypes.array,
    onChange: PropTypes.func,
    onError: PropTypes.func,
    keywords: PropTypes.array,
    readOnly: PropTypes.bool
};

ProfileYamlEditor.defaultProps = {
    data: ''
};

export default ProfileYamlEditor;
