/**
 * Load script on src and run function on script load
 *
 * @param src Src of script to load
 * @param onload Function which will be called when script is loaded
 * @param onerror Function which will be called when script loading is failed
 */
export function loadScript(
    src: string,
    onload: GlobalEventHandlers['onload'] = null,
    onerror: GlobalEventHandlers['onerror'] = null,
) {
    const script = document.createElement('script');

    script.src = src;
    script.onload = onload;
    script.onerror = onerror;
    script.type = 'text/javascript';

    document.body.appendChild(script);
}
