import React, { ChangeEventHandler, FC, useCallback, useState } from 'react';

import { configureRootTheme } from '@yandex-lego/components/Theme';
import { theme } from '@yandex-lego/components/Theme/presets/default';
import { Button } from '@yandex-lego/components/Button/desktop/bundle';
import { Text } from '@yandex-lego/components/Text/bundle';
import { Textinput } from '@yandex-lego/components/Textinput/desktop/bundle';
import { Select } from '@yandex-lego/components/Select/desktop/bundle';
import { Checkbox } from '@yandex-lego/components/Checkbox/desktop/bundle';
import { Captcha } from '@yandex-int/captcha/Captcha/desktop';

import { Header } from '@yandex-lego/components/Header/desktop';
// @ts-expect-error
import * as favoritesIcon from '@yandex-lego/serp-header/dist/base/favorites-icon.desktop';
// @ts-expect-error
import * as yaplusIcon from '@yandex-lego/serp-header/dist/base/yaplus.desktop.base';
// @ts-expect-error
import * as signupLink from '@yandex-lego/serp-header/dist/base/signup-link.desktop';
// @ts-expect-error
import * as loginButton from '@yandex-lego/serp-header/dist/base/login-button.desktop';
// @ts-expect-error
import * as notifierIcon from '@yandex-lego/serp-header/dist/base/notifier.desktop';

import './App.css';

configureRootTheme({ theme });

// @ts-expect-error
const FORM_DATA = window.FORM_DATA as any;

const FontView: FC<{ onChangeHandler: ChangeEventHandler }> = ({
  onChangeHandler,
}) => {
  const options = FORM_DATA.font.choices.map((item: any) => ({
    value: item[0],
    content: item[1],
  }));

  const [value, setValue] = useState(FORM_DATA.font.value);

  return (
    <div>
      <input
        type="hidden"
        value={value}
        name="font"
        onChange={onChangeHandler}
      />
      <Select
        view="default"
        size="m"
        value={value}
        onChange={(event) => setValue(event.target.value)}
        options={options}
      />
    </div>
  );
};

const BackgroundImageView: FC<{ onChangeHandler: ChangeEventHandler }> = ({
  onChangeHandler,
}) => {
  const options = FORM_DATA.background.choices.map((item: any) => ({
    value: item[0],
    content: item[1],
  }));

  const [value, setValue] = useState(FORM_DATA.background.value);

  return (
    <div>
      <input
        type="hidden"
        value={value}
        name="background"
        onChange={onChangeHandler}
      />
      <Select
        view="default"
        size="m"
        value={value}
        onChange={(event) => setValue(event.target.value)}
        options={options}
      />
    </div>
  );
};

const ModeView: FC<{ onChangeHandler: ChangeEventHandler }> = ({
  onChangeHandler,
}) => {
  const options = FORM_DATA.mode.choices.map((item: any) => ({
    value: item[0],
    content: item[1],
  }));

  const [value, setValue] = useState(FORM_DATA.mode.value);

  return (
    <div>
      <input
        type="hidden"
        value={value}
        name="mode"
        onChange={onChangeHandler}
      />
      <Select
        view="default"
        size="m"
        value={value}
        onChange={(event) => setValue(event.target.value)}
        options={options}
      />
    </div>
  );
};
const ReverseArcView = () => {
  const [checked, setChecked] = useState(FORM_DATA.reversed_arc.value);

  return (
    <Checkbox
      name="reversed_arc"
      size="m"
      view="default"
      label="Reverse arc"
      onChange={() => setChecked(!checked)}
      checked={checked}
    />
  );
};

const CaptchaForm = () => {
  const [token, setToken] = useState('');

  const onValidate = useCallback(
    (token?: string) => {
      setToken(token || '');
    },
    [setToken]
  );

  return (
    <div>
      <div>
        <input type="hidden" name="spravka" value={token} />
        <Captcha onValidate={onValidate} />
      </div>

      <Button
        view="action"
        size="s"
        type="submit"
        disabled={!token}
        id="submit"
      >
        Отправить
      </Button>

      <input
        name="csrf_token"
        type="hidden"
        value={FORM_DATA.csrf_token.value}
      />
    </div>
  );
};

const ControlledFormComponent: FC<any> = () => {
  const [state, setState] = useState({});

  const handleSubmit = (e: React.FormEvent) => {};
  const onChangeHandler = (e: any) => {
    setState({
      [e.target.name]: e.target.value,
    });
  };

  return (
    <div>
      <form onSubmit={handleSubmit} method="POST">
        <div className="wrapper">
          <div className="one">
            <Text typography="body-short-l" weight="light">
              Text
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.name.value}
              name="name"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Curvature
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.curvature.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="curvature"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Min color shift
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.color_shift.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="color_shift"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Noise
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.noise.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="noise"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Arc (* π)
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.arc.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="arc"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Squeeze
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.squeeze.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="squeeze"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Shade
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.shade_val.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="shade_val"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Aspect Ratio
            </Text>
          </div>
          <div className="two">
            <Textinput
              size="m"
              view="default"
              className="PinInput form-input"
              key="round-round"
              pin="round-round"
              value={FORM_DATA.aspect_ratio.value}
              type="number"
              // @ts-expect-error
              step="0.01"
              name="aspect_ratio"
              onChange={onChangeHandler}
            />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Reverse arc
            </Text>
          </div>
          <div className="two">
            <ReverseArcView />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Mode
            </Text>
          </div>
          <div className="two">
            <ModeView onChangeHandler={onChangeHandler} />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Background
            </Text>
          </div>
          <div className="two">
            <BackgroundImageView onChangeHandler={onChangeHandler} />
          </div>

          <div className="one">
            <Text typography="body-short-l" weight="light">
              Font
            </Text>
          </div>
          <div className="two">
            <FontView onChangeHandler={onChangeHandler} />
          </div>

          <CaptchaForm />
        </div>
      </form>

      <div className="wrapper">
        <div className="one"></div>
        <div className="two">
          <div className="image-wrapper">
            <img src={FORM_DATA.image} />
          </div>
        </div>
      </div>

      <div className="state-block">
        <br />
        <hr />
        <pre>{JSON.stringify(state, null, 2)}</pre>
      </div>
    </div>
  );
};

function App() {
  const baseProps = {
    key: 'base',
    /* домен */
    tld: 'ru',
    /* язык */
    lang: 'ru',
  };

  return (
    <div className="App">
      <div id="main">
        <Header
          className="SearchHeader"
          actions={
            <>
              <div
                style={{ height: 44 }}
                dangerouslySetInnerHTML={{
                  __html: favoritesIcon.getContent(baseProps),
                }}
              />
              <div
                dangerouslySetInnerHTML={{
                  __html: notifierIcon.getContent(baseProps),
                }}
              />
              <div
                dangerouslySetInnerHTML={{
                  __html: yaplusIcon.getContent(baseProps),
                }}
              />
              <div
                dangerouslySetInnerHTML={{
                  __html: signupLink.getContent(baseProps),
                }}
              />
              <div
                dangerouslySetInnerHTML={{
                  __html: loginButton.getContent(baseProps),
                }}
              />
            </>
          }
        />

        <Text typography="display-s" weight="light">
          Wave капча.
        </Text>

        <ControlledFormComponent />
      </div>
    </div>
  );
}

export default App;
