import yt.wrapper.client
import yt.wrapper.config
import yt.wrapper


def GetYtClient(proxy, token, loggerName=None):
    res = yt.wrapper.client.Yt(config=yt.wrapper.config.config)
    res.config['token'] = token
    res.config['proxy']['url'] = proxy
    res.config['read_retries']['allow_multiple_ranges'] = True
    res.config['read_retries']['count'] = 5
    res.config['read_retries']['enable'] = False
    res.config["tabular_data_format"] = yt.wrapper.YsonFormat()

    return res


def CloneInstance(ytInstance):
    return yt.wrapper.client.Yt(config=ytInstance.config)


def MkDirSafe(ytInst, name):
    if not ytInst.exists(name):
        ytInst.mkdir(name, recursive=True)
