export default function (value) {
    const control = document.createElement('textarea');

    control.style = 'position: absolute; left: -1000px; top: -1000px';
    control.value = value || '';
    document.body.appendChild(control);
    control.select();
    document.execCommand('copy');
    document.body.removeChild(control);
}
