import React from 'react';
import PropTypes from 'prop-types';

import YAML from 'js-yaml';
import {MonacoDiffEditor} from 'react-monaco-editor';

import Bem from 'app/components/bem/bem';

import './object-diff.css';
import Preloader from 'app/components/preloader/preloader';

export default class ObjectDiff extends React.Component {
    render() {
        const {
            firstObj,
            secondObj,
            loaded
        } = this.props;

        return (
            <Bem
                block='config-diff'>
                {!loaded ?
                    <Preloader /> :
                    <MonacoDiffEditor
                        language='yaml'
                        options={{
                            readOnly: true,
                            scrollBeyondLastLine: false,
                            minimap: {
                                enabled: false
                            }
                        }}
                        original={firstObj ? YAML.safeDump(firstObj, {
                            lineWidth: Infinity
                        }) : ''}
                        value={secondObj ? YAML.safeDump(secondObj, {
                            lineWidth: Infinity
                        }) : ''} />
                }
            </Bem>
        );
    }
}

ObjectDiff.propTypes = {
    firstObj: PropTypes.object,
    secondObj: PropTypes.object,
    loaded: PropTypes.bool
};
