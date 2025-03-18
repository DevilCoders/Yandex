import React from 'react';
import Popup from 'reactjs-popup';
import SyntaxHighlighter from 'react-syntax-highlighter';
import { docco } from 'react-syntax-highlighter/dist/esm/styles/hljs';
import 'reactjs-popup/dist/index.css';
import '../css/PopUpTraceback.css';
import {ArgsKwargsToList} from "../utils/ValueList";


const PopUpTraceback = props => {
    let title = props.title ? props.title : 'EMPTY_TITLE';
    let args = props.args ? props.args : 'EMPTY_ARGS';
    let kwargs = props.kwargs ? props.kwargs : 'EMPTY_KWARGS';
    let content = props.content ? props.content : 'EMPTY_CONTENT';
    let caption = props.caption ? props.caption : "E";

    return <Popup
        trigger={
            <button className="button"> { caption } </button>
        }
        modal
        nested
    >
        {close => {
            return <div className="modal">
                <button className="close" onClick={close}>
                    &times;
                </button>
                <div className="header">
                    { title }
                </div>
                <div className="content">
                    {' '}
                    { ArgsKwargsToList(args, kwargs) }
                    <SyntaxHighlighter language="python" style={docco}>
                        { content }
                    </SyntaxHighlighter>
                </div>
            </div>
        }}
    </Popup>
};

export default PopUpTraceback;
