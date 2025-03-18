import React from 'react';
import ReactDOM from 'react-dom';
import PropTypes from 'prop-types';

import {scroll} from 'app/lib/animation';
import {get, set, isEqual, has} from 'lodash';

import Bem from 'app/components/bem/bem';
import ConfigGroupEditor from './__group/config-editor__group';

import './config-editor.css';

const SCROLL_DELAY = 1000;

class ConfigEditor extends React.Component {
    constructor(props) {
        super(props);

        this._refs = {};
        this._scope = {};

        this.setWrapperRef = this.setWrapperRef.bind(this);
        this.onError = this.onError.bind(this);
        this.setRef = this.setRef.bind(this);
        this.isEnableItemConfig = this.isEnableItemConfig.bind(this);
        this.isEnableDefaultValue = this.isEnableDefaultValue.bind(this);
    }

    getChildContext() {
        return {
            scope: this._scope
        };
    }

    setWrapperRef(wrapper) {
        this._scope.dom = wrapper;
    }

    getKeyErrorGroup(errPath) {
        const {
            schema: template
        } = this.props;

        for (let i = 0; i < template.length; i++) {
            const group = template[i];
            const items = group.items;
            const hasError = items.some(item => (isEqual(item.key.name, errPath)));

            if (hasError) {
                return group.group_name;
            }
        }
    }

    searchErrElement(vPath) {
        const path = vPath.slice();

        while (path.length) {
            const el = this._refs[path.join('.')];

            if (el) {
                return el;
            }

            path.pop();
        }

        return null;
    }

    calculateScrollOffset(el, key) {
        const group = this._refs[key];
        let offset = group.offsetTop;

        if (el) {
            let prev = el;

            while (!isEqual(el.parentNode, group)) {
                prev = el;
                el = el.parentNode;
            }
            offset += prev.offsetTop;
        }

        return offset;
    }

    componentWillReceiveProps(newProps) {
        if (newProps.validation && Object.values(newProps.validation).length) {
            const firstErrKey = Object.keys(newProps.validation)[0];
            const key = this.getKeyErrorGroup(firstErrKey);

            if (key) {
                const scope = this.context.scope.dom;
                const el = this.searchErrElement(newProps.validation[firstErrKey][0].path);
                const offset = this.calculateScrollOffset(el, key);
                scroll(scope, offset, SCROLL_DELAY);
            }
        }
    }

    setRef(name) {
        return ref => {
            // TODO: подумать тут, выглядит не тривиально=)
            this._refs[name] = ReactDOM.findDOMNode(ref); // eslint-disable-line react/no-find-dom-node
        };
    }

    onError(name) {
        return value => {
            this.props.onError(name, value);
        };
    }

    getGroupData(groupItems) {
        const {
            data: dataProps,
            dataSettings: dataSettingsProps,
            dataCurrent: dataCurrentProps
        } = this.props;
        let data = {},
            dataCurrent = {},
            dataSettings = {};

        groupItems.forEach(item => {
            const name = item.key.name;

            if (dataProps[name] !== undefined) {
                set(data, name, get(dataProps, name));
            }

            if (dataCurrentProps[name] !== undefined) {
                set(dataCurrent, name, get(dataCurrentProps, name));
            }

            if (dataSettingsProps[name]) {
                set(dataSettings, [name, 'UNSET'], get(dataSettingsProps, [name, 'UNSET']));
            }
        });

        return {
            data,
            dataSettings,
            dataCurrent
        };
    }

    isEnableDefaultValue(name) {
        const path = [name, 'UNSET'];

        return Boolean(get(this.props.dataSettings, path));
    }

    isEnableItemConfig(name) {
        return has(this.props.dataCurrent, name);
    }

    render() {
        const {
            schema: template,
            readOnly,
            validation
        } = this.props;

        return (
            <Bem
                block='config-form-editor'
                tagRef={this.setWrapperRef}>
                {template.map(group => {
                    const groupData = this.getGroupData(group.items);

                    return group.items.length ?
                        <ConfigGroupEditor
                            key={`group-${group.title}`}
                            setDefaultValue={this.props.setDefaultValue}
                            isEnableItemConfig={this.isEnableItemConfig}
                            isEnableDefaultValue={this.isEnableDefaultValue}
                            title={group.title}
                            items={group.items}
                            readOnly={readOnly}
                            data={groupData.data}
                            dataCurrent={groupData.dataCurrent}
                            dataSettings={groupData.dataSettings}
                            onChange={this.props.onChange}
                            onError={this.onError}
                            setEditorRef={this.setRef}
                            ref={this.setRef(group.group_name)}
                            validation={validation || []} /> : null;
                })}
            </Bem>
        );
    }
}

ConfigEditor.childContextTypes = {
    scope: PropTypes.object
};

ConfigEditor.contextTypes = {
    scope: PropTypes.object
};

ConfigEditor.propTypes = {
    data: PropTypes.object,
    schema: PropTypes.array,
    readOnly: PropTypes.bool,
    validation: PropTypes.object,
    onChange: PropTypes.func,
    onError: PropTypes.func,
    setDefaultValue: PropTypes.func,
    dataSettings: PropTypes.object,
    dataCurrent: PropTypes.object
};

export default ConfigEditor;
