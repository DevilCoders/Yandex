import PropTypes from 'prop-types';

import {setGlobalErrors} from 'app/actions/errors';

import {getService} from 'app/reducers';
import servicesApi from 'app/api/service';

import ConfigArrayEditor from 'app/components/config-editor/__array/config-editor__array';

import {connect} from 'react-redux';

class ConfigTokensEditor extends ConfigArrayEditor {
    constructor(props) {
        super(props);

        this.state = {
            newToken: undefined,
            value: props.value || []
        };

        this.getToken = this.getToken.bind(this);
        // сразу сделаем запрос за новым токеном, чтобы при добавлении нового поля не ждать ответа от API
        if (!this.props.readOnly && this.props.service) {
            this.getToken();
        }
    }

    getToken() {
        servicesApi.generateToken(this.props.service.id).then(result => {
            this.setState({
                newToken: result.token
            });
        }, error => {
            this.setState({
                newToken: undefined
            });
            this.props.setGlobalErrors([error.message]);
        });
    }

    onAdd() {
        this.getToken();
        const {
            value: valueState,
            newToken: newTokenState
        } = this.state;
        const value = [...valueState, newTokenState];

        this.setState({
            value
        }, () => {
            this.props.onChange(value);
        });
    }
}

ConfigTokensEditor.propTypes = {
    service: PropTypes.object,
    onChange: PropTypes.func.isRequired,
    onError: PropTypes.func.isRequired,
    path: PropTypes.string.isRequired,
    validation: PropTypes.array,
    Item: PropTypes.func.isRequired,
    yamlKey: PropTypes.string,
    placeholder: PropTypes.any,
    value: PropTypes.array, // can be undefined
    title: PropTypes.string,
    hint: PropTypes.string,
    keyFunction: PropTypes.func,
    readOnly: PropTypes.bool,
    childProps: PropTypes.object,
    setGlobalErrors: PropTypes.func
};

export default connect(state => {
    return {
        service: getService(state).service
    };
}, dispatch => {
    return {
        setGlobalErrors: errors => {
            return dispatch(setGlobalErrors(errors));
        }
    };
})(ConfigTokensEditor);
