import { Button } from "./button";

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
    icon?: string;
    text?: string;
}
export class PopupButton extends React.Component<Props, {}> {
    static displayName = "PopupButton";
    ref: Element;

    onButtonClick(evt: React.MouseEvent<HTMLElement>) {
        popupClose();
        $(this.ref).css("opacity", 1).css("visibility", "visible");
        if (evt)
            evt.nativeEvent.stopImmediatePropagation();
    }

    onWindowClick(evt: React.MouseEvent<HTMLElement>) {
        if (evt)
            evt.nativeEvent.stopImmediatePropagation();
    }

    render() {
        let control: JSX.Element = null;
        if (this.props.icon)
            control = <i className={`fa fa-fw fa-${ this.props.icon }`} onClick={ this.onButtonClick.bind(this) }/>;
        if (this.props.text)
            control = <Button onClick={ this.onButtonClick.bind(this) } text={this.props.text} />;

        return <div className="popup-button" onClick={ this.onWindowClick.bind(this) }>
            { control }
            <Popup ref={ (ref) => { this.ref = ReactDOM.findDOMNode(ref); } }>
                { this.props.children }
            </Popup>
        </div>;
    }
}
