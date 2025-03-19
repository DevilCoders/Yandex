import { Logotype } from "./components/logotype";


interface MenuHandleSelection {
    (id: number): void;
}

interface Props {
    title: string;
    items?: string[];
    selected?: number;
    handler?: MenuHandleSelection;
    onClick: () => void;
}

interface State {
}

export class Header extends React.Component<Props, State> {
    static displayName = "Header";

    onMenuClick(id: number) {
        this.props.handler(id);
    }

    render(): JSX.Element {
        let items: JSX.Element[] = [];

        if (this.props.items)
            for (let i = 0; i < this.props.items.length; i++) {
                let cls = "";
                if (this.props.selected === i) {
                    cls = "selected";
                }
                items.push(<div key={i} className={cls} onClick={ this.onMenuClick.bind(this, i) }>
                               { this.props.items[i] }
                           </div>);
            }

        let login = "dummy";
        let m = document.cookie.match(/yandex_login=([a-z0-9_-]+)/i);
        if (m) login = m[1];
        let avatar = `https://center.yandex-team.ru/api/v1/user/${login}/avatar/100.jpg`;

        return <div className="head">
            <Logotype text={ this.props.title } onClick={ this.props.onClick }/>
            <div className="menu">{ items }</div>
            <a href="https://passport.yandex-team.ru"><img className="avatar" src={avatar}/></a>
        </div>;
    }
}
