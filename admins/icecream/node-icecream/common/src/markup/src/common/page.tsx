/// <reference types="react-addons-css-transition-group"/>

import { Header } from "./header";
import { Page404 } from "./pages/404";
import { popupClose } from "./components/popup";


import * as __React from "react";
import * as __ReactDOM from "react-dom";
import * as __ReactCSSTransitionGroup from "react-addons-css-transition-group";
declare global {
    const React: typeof __React;
    const ReactDOM: typeof __ReactDOM;
    const ReactCSSTransitionGroup: typeof __ReactCSSTransitionGroup;
}
(window as any).ReactCSSTransitionGroup = (React as any).addons.CSSTransitionGroup;


interface MenuItem {
    url: string;
    name: string;
    page: JSX.Element;
}

export class Page extends React.Component<{}, {}> {
    static displayName = "Page";
    pageName: string;
    menu: Array<MenuItem>;

    constructor() {
        super();
        this.menu = [];
        this.pageName = "Привет, мир!";
        window.onpopstate = this.onPopState.bind(this);
    }

    pushState(url: string) {
        window.history.pushState(url, url, url);
        this.setState({});
    }

    onPopState(url: string) {
        this.setState({});
    }

    onMenuSelect(id: number) {
        let mi = this.menu[id];
        window.history.pushState({}, mi.name, mi.url);
        this.setState({});
    }

    getPage(path: string): JSX.Element {
        return null;
    }

    onLogoClick() {
        this.pushState("/");
    }

    render(): JSX.Element {
        let page: JSX.Element = null;

        let names: string[] = [];
        let selected = -1;
        for (let x in this.menu) {
            names.push(this.menu[x].name);
            if (document.location.pathname === this.menu[x].url) {
                page = this.menu[x].page;
                selected = Number(x);
            }
        }

        if (!page) page = this.getPage(document.location.pathname);
        if (!page) page = <Page404 key="404"/>;

        return <div className="page">
            <div className="netProgress"><div id="netProgress"/></div>
            <Header
                handler={ this.onMenuSelect.bind(this) }
                items={names}
                selected={ selected }
                title={ this.pageName }
                onClick={ this.onLogoClick.bind(this) }
                />
            <div className="content">
                <ReactCSSTransitionGroup transitionName="page" transitionEnterTimeout={200} transitionLeaveTimeout={200}>
                { page }
                </ReactCSSTransitionGroup>
            </div>
        </div>;
    }
}

$(function(){
    document.onclick = popupClose;
});
