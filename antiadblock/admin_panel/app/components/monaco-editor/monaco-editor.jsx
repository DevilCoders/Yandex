import React from 'react';
import PropTypes from 'prop-types';

import MonacoEditor from 'react-monaco-editor';

import Bem from 'app/components/bem/bem';

import './monaco-editor.css';

class Editor extends React.Component {
    constructor() {
        super();

        this._monaco = null;

        this._disposable = null;

        this.editorDidMount = this.editorDidMount.bind(this);
    }

    componentWillReceiveProps(props) {
        this.setErrors(this._monaco, props.errors || []);
    }

    componentWillUnmount() {
        if (this._disposable) {
            this._disposable.dispose();
        }
    }

    editorDidMount(editor, monaco) {
        this._monaco = monaco;

        // Register completion only once
        this._disposable = monaco.languages.registerCompletionItemProvider(this.props.language, {
            provideCompletionItems: () => {
                return (this.props.keywords || []).map(item => ({
                    label: item,
                    kind: monaco.languages.CompletionItemKind.Text
                }));
            }
        });
    }

    setErrors(monaco, errors) {
        // Монако может очень медленно инициализироваться
        // Вызов ReceiveProps приходит раньше DidMount
        // Если ошибок нет, то выходим, иначе пустой массив затрет стандартные ошибки
        if (!monaco || !errors) {
            return;
        }

        const markers = errors.map(error => {
            let location = error.location;

            return {
                severity: monaco.Severity.Error,
                code: null,
                source: null,
                startLineNumber: location.start.line,
                startColumn: location.start.column,
                endLineNumber: location.end.line,
                endColumn: location.end.column,
                message: error.message
            };
        });
        const models = monaco.editor.getModels();
        models.forEach(model => monaco.editor.setModelMarkers(model, this.props.language, markers));
    }

    render() {
        const monacoProps = {};
        if (this.props.editorWillMount) {
            monacoProps.editorWillMount = this.props.editorWillMount;
        }

        return (
            <Bem
                block='monaco-editor'
                elem='main'>
                <MonacoEditor
                    {...monacoProps}
                    width={this.props.width}
                    height={this.props.height}
                    language={this.props.language}
                    options={this.props.options}
                    editorDidMount={this.editorDidMount}
                    onChange={this.props.onChange}
                    value={this.props.value || ''} />
            </Bem>
        );
    }
}

Editor.propTypes = {
    editorWillMount: PropTypes.func,
    errors: PropTypes.array,
    width: PropTypes.string,
    height: PropTypes.string,
    language: PropTypes.string,
    value: PropTypes.string,
    onChange: PropTypes.func,
    keywords: PropTypes.arrayOf(PropTypes.string),
    options: PropTypes.shape({
        readOnly: PropTypes.bool,
        scrollBeyondLastLine: PropTypes.bool,
        minimap: PropTypes.shape({
            enabled: PropTypes.bool
        })
    })
};

Editor.defaultProps = {
    width: '100%'
};

export default Editor;
