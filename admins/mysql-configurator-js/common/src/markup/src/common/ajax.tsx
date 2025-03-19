/// <reference types="es6-promise" />

class NetProgress {
    static width = 0;
    static timer: number = 0;

    static hide() {
        $("#netProgress")
        .css("opacity", 0)
        .css("width", "0%")
        .css("height", "3px")
        .css("padding", "0")
        .text("");
    }

    static success() {
        $("#netProgress")
        .css("opacity", 1)
        .css("width", "100%")
        .css("background-color", "#00C000");

        clearInterval(NetProgress.timer);
        setTimeout(function(){ NetProgress.hide(); }, 1500);
    }

    static error(err: any) {
        $("#netProgress")
        .css("opacity", 1)
        .css("width", "100%")
        .css("height", "1.4em")
        .css("padding", "0.5em 1em")
        .css("background-color", "#C00000")
        .text(`Ошибка ${JSON.stringify(err)}`);

        clearInterval(NetProgress.timer);
        setTimeout(function(){ NetProgress.hide(); }, 3500);
    }

    static progressAdvance() {
        $("#netProgress")
        .css("height", "3px")
        .css("padding", "0")
        .css("opacity", 1)
        .css("width", `${NetProgress.width}%`)
        .css("background-color", "#FFC000");
        NetProgress.width += 3 * (1 - NetProgress.width / 100);
    }

    static progress() {
        NetProgress.width = 0;
        NetProgress.progressAdvance();
        clearInterval(NetProgress.timer);
        NetProgress.timer = window.setInterval(NetProgress.progressAdvance, 500);
    }
}

export interface ScalarObject {
    [name: string]: string|number;
}

export function getUrl(url: string, method: string = "GET", data: string = undefined) {
    NetProgress.progress();

    return new Promise((resolve, reject): void => {
        let params: any = {
            method: method,
            dataType: "json",
            error: (err: any): void => {
                NetProgress.error(err);
                reject(err);
            },
            success: (data: Object): void => {
                NetProgress.success();
                resolve(data);
            }
        };
        if (data) {
            params.data = data;
            params.contentType = "application/json";
        }
        $.ajax(url, params);
    });
}

export function getUrlWithParams(url: string, params: ScalarObject, method: string = "GET") {
    let filter: string[] = [];
    for (let prm in params) {
        if (params[prm]) {
            filter.push(`${prm}=${params[prm]}`);
        }
    }

    let metaurl = url;
    if (filter.length > 0) {
        metaurl += "?" + filter.join("&");
    }

    return getUrl(metaurl, method);
}

export function stdCatch(err: any) {
    console.log(err);
}
