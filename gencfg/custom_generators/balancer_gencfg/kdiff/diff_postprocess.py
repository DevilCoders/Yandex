#!/usr/bin/env python

import re
import sys
import argparse


_RE_ANCHOR = re.compile(r"@@HASH_ANCHOR_BEGIN@(.+?)@@HASH_ANCHOR_END@")
_RE_LINK = re.compile(r"@@HASH_LINK_BEGIN@(.+?)@@HASH_LINK_END@")
_DIFF_CHANGED_BEGIN = '<span class="diffchanged2">'
_DIFF_CHANGED_END = '</span>'
_DIFF_DOT_X = '<span class="diffponct">&middot;</span>x<span class="diffponct">&middot;</span>'


def read_file(file_name):
    with open(file_name) as f:
        return f.read()
    return ""


def write_file(file_name, contents):
    with open(file_name, "w") as f:
        f.write(contents)


def main():
    parser = argparse.ArgumentParser(description="Balancer config diff post-processing")
    parser.add_argument(
        "-i", "--input",
        type=str,
        default=None,
        help="Input diff (will use stdin if omitted)",
    )
    parser.add_argument(
        "-o", "--output",
        type=str,
        default=None,
        help="Output diff file (will use stdout if omitted)",
    )
    options = parser.parse_args()

    diff_contents = read_file(options.input) if options.input else sys.stdin.read()

    anchor_matches = re.findall(_RE_ANCHOR, diff_contents)
    for anchor_match in anchor_matches:
        # Something like this:
        # <span class="diffchanged2">4cec32b60703520b3231bf5b9dc49ccc</span>
        # <span class="diffponct">&middot;</span>x<span class="diffponct">&middot;</span>2
        # "1649d37f52e80ed6e324490c5db93e91 \
        # <span class="diffponct">&middot;</span>x<span class="diffponct">&middot;</span> \
        # 288"
        anchor_match_filtered = anchor_match
        anchor_match_filtered = anchor_match_filtered.replace(_DIFF_DOT_X, " x ")
        anchor_match_filtered = anchor_match_filtered.replace(_DIFF_CHANGED_BEGIN, "")
        anchor_match_filtered = anchor_match_filtered.replace(_DIFF_CHANGED_END, "")
        splitted_anchor = anchor_match_filtered.split(" ")
        subtree_hash = splitted_anchor[0]
        diff_contents = diff_contents.replace(
            "@@HASH_ANCHOR_BEGIN@{anchor_match}@@HASH_ANCHOR_END@".format(
                anchor_match=anchor_match,
            ),
            "<a name=\"{subtree_hash}\" />{anchor_match}".format(
                subtree_hash=subtree_hash,
                anchor_match=anchor_match,
            ),
        )

    link_matches = re.findall(_RE_LINK, diff_contents)
    for link_match in link_matches:
        # Something like this:
        # "<span class="diffchanged2">4cec32b60703520b3231bf5b9dc49ccc</span>"
        link_match_filtered = link_match
        link_match_filtered = link_match_filtered.replace(_DIFF_CHANGED_BEGIN, "")
        link_match_filtered = link_match_filtered.replace(_DIFF_CHANGED_END, "")

        diff_contents = diff_contents.replace(
            "@@HASH_LINK_BEGIN@{link_match}@@HASH_LINK_END@".format(link_match=link_match),
            "<a class=\"subtree_hash\" href=\"#{subtree_hash_filtered}\">{subtree_hash}</a>".format(
                subtree_hash=link_match,
                subtree_hash_filtered=link_match_filtered,
            ),
        )

    diff_contents = diff_contents.replace("diffs/", "")
    diff_contents = diff_contents.replace(".cfg.json.old.simplified.json", " OLD")
    diff_contents = diff_contents.replace(".cfg.json.new.simplified.json", " NEW")
    # fix title (MINOTAUR-873)
    diff_contents = diff_contents.replace(".json.simplified.diff", "")

    if options.output:
        write_file(options.output, diff_contents)
    else:
        sys.stdout.write(diff_contents)


if __name__ == "__main__":
    main()
