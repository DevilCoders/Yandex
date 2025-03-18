import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';
import Modal from 'lego-on-react/src/components/modal/modal.react';
import MonacoEditor from 'app/components/monaco-editor/monaco-editor';
import ModalClose from 'app/components/modal/__close/modal__close';

import i18n from 'app/lib/i18n';

import './service-sbs-screenshots-checks__modal.css';

export default class ServiceSbsScreenshotsChecksModal extends React.Component {
    renderDirectionElement(direction, data) {
        return (
            <Bem
                block='service-sbs-screenshots-checks-modal'
                elem={`body-${direction}`}>
                <MonacoEditor
                    language='json'
                    options={{
                        readOnly: true,
                        scrollBeyondLastLine: false
                    }}
                    value={JSON.stringify((data || {}), null, 4)} />
            </Bem>
        );
    }

    render() {
        return (this.props.visible &&
            <Modal
                autoclosable
                onDocKeyDown={this.props.onChangeVisible}
                onOutsideClick={this.props.onChangeVisible}
                visible={this.props.visible}>
                <Bem
                    key='title'
                    block='service-sbs-screenshots-checks-modal'
                    elem='title'>
                    {i18n('sbs', 'case-info')}
                </Bem>
                <Bem
                    block='service-sbs-screenshots-checks-modal'
                    elem='body'>
                    {['left', 'right'].map(direction => (
                        this.renderDirectionElement(direction, this.props.data[direction])
                    ))}
                </Bem>
                <ModalClose
                    key='close'
                    onClick={this.onChangeModalVisible} />
            </Modal>);
    }
}

ServiceSbsScreenshotsChecksModal.propTypes = {
    visible: PropTypes.bool,
    onChangeVisible: PropTypes.func,
    data: PropTypes.any
};
