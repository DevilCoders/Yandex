from kernel.alice.music_scenario.web_url_canonizer.pybinding import alice_web_url_canonizer


def main():
    print alice_web_url_canonizer.canonize_music_url('https://music.yandex.ru/album/8928045/track/58666693?from=serp')


if __name__ == "__main__":
    main()
