// https://www.digitalocean.com/community/tutorials/typescript-new-project
// https://stackoverflow.com/questions/34474651/typescript-compile-to-single-file (npm install --save-dev webpack webpack-cli typescript ts-loader)

import { loadScript } from './load-script';

interface PGreed {
    safeGet(): Promise<Record<string, string>>;
}

declare global {
    interface Window {
        PGreed: PGreed;
    }
}

export async function validateFingerprint(url: string): Promise<string> {
    const fingerprint = await window.PGreed.safeGet();
    const response = await fetch(url, {
        method: 'POST',
        mode: 'cors',
        body: `rdata=${encodeURIComponent(JSON.stringify(fingerprint))}`,
    });
    await response.json();
    return '';
}

function onLoad() {
    const url = (location.host ? location.protocol + "//" + location.host : "https://yandex.ru") + "/tmgrdfrendc";
    validateFingerprint(url).then(() => 0);
}

function onError() {
    console.error("Failed to load tmgrdfrendpgrd");
}

setTimeout(() => {
    const url = location.host ? '/tmgrdfrendpgrd' : 'tmgrdfrendpgrd.js';
    loadScript(url, onLoad, onError);
}, 0);
