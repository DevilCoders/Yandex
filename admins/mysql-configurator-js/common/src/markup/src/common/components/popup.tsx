export function popupClose() {
    $(".popup").css("opacity", 0).css("visibility", "hidden");
}

export class Popup extends React.Component<{}, {}> {
    static displayName = "Popup";
    render() {
        return <div className="popup">
            { this.props.children }
        </div>;
    }
}

interface Props {
    icon: string;
}
export class PopupButton extends React.Component<Props, {}> {
    static displayName = "PopupButton";
    ref: Element;

    onButtonClick(evt: React.MouseEvent<HTMLElement>) {
        popupClose();
        $(this.ref).css("opacity", 1).css("visibility", "visible");
        evt.nativeEvent.stopImmediatePropagation();
    }

    onWindowClick(evt: React.MouseEvent<HTMLElement>) {
        evt.nativeEvent.stopImmediatePropagation();
    }

    render() {
        return <div className="popup-button" onClick={ this.onWindowClick.bind(this) }>
            <i className={`fa fa-fw fa-${ this.props.icon }`} onClick={ this.onButtonClick.bind(this) }/>
            <Popup ref={ (ref) => { this.ref = ReactDOM.findDOMNode(ref); } }>
                { this.props.children }
            </Popup>
        </div>;
    }
}
