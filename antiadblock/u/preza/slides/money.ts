import pptxgen from "pptxgenjs";
import moment from "moment";
import { makeCall } from "../utils/utils";
import { TITLE_FONT_SIZE } from '../const';

// TODO Тут можно оптимизировать делать запросы в сеть до yield
export async function* createSlideMoneyDesktop(){
    console.log('/--------------------------------------------/');
    console.log('START DESKTOP TOTAL MONEY SLIDE');
    const slide: pptxgen.Slide = yield;
    await createSlide(slide, "desktop");
    console.log('/--------------------------------------------/');
    return slide;
}

export async function* createSlideMoneyMobile(){
    console.log('/--------------------------------------------/');
    console.log('START MOBILE TOTAL MONEY SLIDE');
    const slide: pptxgen.Slide = yield;
    await createSlide(slide, "mobile");
    console.log('/--------------------------------------------/');
    return slide;
}

export async function* createSlideMoneySdkAndroid(){
    console.log('/--------------------------------------------/');
    console.log('START ANDROID SDK MONEY SLIDE');
    const slide: pptxgen.Slide = yield;
    await createSlide(slide, "android");
    console.log('/--------------------------------------------/');
    return slide;
}

async function createSlide(slide: pptxgen.Slide, device: string) {
    const DATE_TIME_FORMAT = 'YYYY-MM-DD';
    let dateMax = new Date();
    let dateMin = new Date();
    dateMax.setDate(dateMax.getDate() - 1);
    dateMin.setDate(dateMin.getDate() - 60);
    let dateMaxFormatted = (moment(dateMax)).format(DATE_TIME_FORMAT);
    let dateMinFormatted = (moment(dateMin)).format(DATE_TIME_FORMAT);
    let textboxOpts = { x: 0.5, y: 0.5, fontSize: TITLE_FONT_SIZE, color: '363636', fill: { color:'F1F1F1' } };
    slide.addText(`Общие деньги (${device})`, textboxOpts);
    let download_link = `https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/editor/x656vzfeqaout?domain=_total&scale=d&device=${device}&date_max=${dateMaxFormatted}&date_min=${dateMinFormatted}&_no_controls=true&__scr_width=1600&__scr_height=720&&_embedded=1`;
    let image:string = await makeCall(download_link);
    slide.addImage({x:0.1, y:0.8, w:8.0, h:4.0, data: `data:image/png;base64,${image}`});
    download_link = `https://charts.yandex-team.ru/api/scr/v1/screenshots/preview/editor/mn5ab1pehlpkg?service_id=%09_total%09device%09${device}%09&_no_controls=true&_embedded=1&__scr_width=420&__scr_height=1650`;
    image = await makeCall(download_link);
    slide.addImage({x:8.5, y:0.0, w:1.2, h:5.5, data: `data:image/png;base64,${image}`});
    return slide;
}
