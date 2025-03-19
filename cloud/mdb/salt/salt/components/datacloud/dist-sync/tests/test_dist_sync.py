from dist_sync.dist_sync import parse_publish_list_out, PublishedSnapshot


def test_publish_list_out():
    ret = parse_publish_list_out(
        """
Published repositories:
  * s3:bucket:./.- (origin: mdb) [all] publishes {main: [mdb-bionic-stable-all-2021-04-16T04:32:37.239729]: Snapshot from mirror [mdb-bionic-stable-all]: http://dist.yandex.ru/mdb-bionic-secure/stable/all/ ./}
  * s3:bucket:./.- (origin: mdb) [amd64] publishes {main: [mdb-bionic-stable-amd64-2021-04-15T21:15:50.380286]: Snapshot from mirror [mdb-bionic-stable-amd64]: http://dist.yandex.ru/mdb-bionic-secure/stable/amd64/ ./}
"""
    )
    assert list(ret) == [
        PublishedSnapshot(
            snapshot="mdb-bionic-stable-all-2021-04-16T04:32:37.239729",
            mirror="mdb-bionic-stable-all",
        ),
        PublishedSnapshot(
            snapshot="mdb-bionic-stable-amd64-2021-04-15T21:15:50.380286",
            mirror="mdb-bionic-stable-amd64",
        ),
    ]
