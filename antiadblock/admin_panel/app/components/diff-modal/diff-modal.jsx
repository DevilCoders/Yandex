import React from 'react';
import PropTypes from 'prop-types';

import Bem from 'app/components/bem/bem';

import Modal from 'lego-on-react/src/components/modal/modal.react';
import Button from 'lego-on-react/src/components/button/button.react';

import ModalClose from 'app/components/modal/__close/modal__close';
import ObjectDiff from 'app/components/object-diff/object-diff';
import LinkCopy from 'app/components/link-copy/link-copy';

import i18n from 'app/lib/i18n';

export default class DiffModal extends React.Component {
    render() {
        const {
            closeDiffModal: onCloseDiffModal,
            visible,
            urlDiff
        } = this.props;

        return (
            <Modal
                autoclosable
                onDocKeyDown={onCloseDiffModal}
                onOutsideClick={onCloseDiffModal}
                visible={visible}>
                <Bem
                    key='title'
                    block='modal'
                    elem='title'>
                    {i18n('service-page', 'diff-title')}
                    <LinkCopy
                        text={i18n('hints', 'link-copied-to-buffer')}
                        to='right'
                        value={urlDiff} />
                </Bem>
                <ModalClose
                    key='close'
                    onClick={onCloseDiffModal} />
                <Bem
                    key='body'
                    block='modal'
                    elem='body'>
                    <ObjectDiff
                        firstObj={this.props.firstObj}
                        secondObj={this.props.secondObj}
                        loaded={this.props.loaded} />
                </Bem>
                <Bem
                    key='actions'
                    block='modal'
                    elem='actions'>
                    <Button
                        theme='normal'
                        view='default'
                        tone='grey'
                        size='s'
                        mix={{
                            block: 'modal',
                            elem: 'action'
                        }}
                        onClick={onCloseDiffModal}>
                        {i18n('common', 'close')}
                    </Button>
                </Bem>
            </Modal>
        );
    }
}

DiffModal.propTypes = {
    firstObj: PropTypes.object,
    secondObj: PropTypes.object,
    loaded: PropTypes.bool,
    visible: PropTypes.bool,
    closeDiffModal: PropTypes.func.isRequired,
    urlDiff: PropTypes.string
};
