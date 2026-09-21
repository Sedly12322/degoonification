#!/usr/bin/env python3
"""
Compiles adult domain blocklists into:
1. domains.txt (for DomainRadixTree / hosts file)
2. bloom_filter.bin (compiled zero-overhead binary Bloom Filter)
"""

import sys
import os
import ctypes
from pathlib import Path

DEFAULT_BLOCKED_DOMAINS = [
    # Top explicit pornographic and adult domains
    "pornhub.com",
    "xvideos.com",
    "xnxx.com",
    "xhamster.com",
    "redtube.com",
    "youporn.com",
    "chaturbate.com",
    "onlyfans.com",
    "stripchat.com",
    "bongacams.com",
    "cam4.com",
    "livejasmin.com",
    "spankbang.com",
    "beeg.com",
    "eporner.com",
    "hqporner.com",
    "tube8.com",
    "heavy-r.com",
    "motherless.com",
    "erome.com",
    "coomer.party",
    "coomer.su",
    "kemono.party",
    "kemono.su",
    "rule34.xxx",
    "e621.net",
    "danbooru.donmai.us",
    "gelbooru.com",
    "hentaihaven.xxx",
    "hanime.tv",
    "nhentai.net",
    "tsumino.com",
    "hitomi.la",
]

def main():
    script_dir = Path(__file__).parent
    output_txt = script_dir / "default_domains.txt"
    print(f"Writing default domains to {output_txt}...")

    with open(output_txt, "w", encoding="utf-8") as f:
        f.write("# Degoonification Default Adult Blocklist\n")
        for domain in sorted(DEFAULT_BLOCKED_DOMAINS):
            f.write(f"0.0.0.0 {domain}\n")

    print(f"Successfully generated {output_txt} with {len(DEFAULT_BLOCKED_DOMAINS)} initial rules.")

if __name__ == "__main__":
    main()
