#!/usr/bin/env bash
# Redirects local outbound DNS traffic (port 53 UDP) to Degoonification Sinkhole (127.0.0.1:5353)
set -euo pipefail

ACTION="${1:-enable}"
PORT="${2:-5353}"

if [ "$(id -u)" -ne 0 ]; then
    echo "Error: This script requires root privileges to configure firewall redirection." >&2
    echo "Usage: sudo $0 [enable|disable] [port]" >&2
    exit 1
fi

if [ "$ACTION" = "enable" ]; then
    echo "Enabling DNS redirect to 127.0.0.1:$PORT..."
    if command -v nft >/dev/null 2>&1; then
        nft add table ip degoon_filter 2>/dev/null || true
        nft add chain ip degoon_filter prerouting '{ type nat hook prerouting priority dstnat; }' 2>/dev/null || true
        nft add chain ip degoon_filter output '{ type nat hook output priority -100; }' 2>/dev/null || true
        nft add rule ip degoon_filter output udp dport 53 ip daddr != 127.0.0.1 redirect to :"$PORT"
        echo "✓ nftables DNS redirect rule active."
    else
        iptables -t nat -A OUTPUT -p udp --dport 53 ! -d 127.0.0.1 -j REDIRECT --to-ports "$PORT"
        echo "✓ iptables DNS redirect rule active."
    fi
elif [ "$ACTION" = "disable" ]; then
    echo "Disabling DNS redirect..."
    if command -v nft >/dev/null 2>&1; then
        nft delete table ip degoon_filter 2>/dev/null || true
        echo "✓ nftables table degoon_filter deleted."
    else
        iptables -t nat -D OUTPUT -p udp --dport 53 ! -d 127.0.0.1 -j REDIRECT --to-ports "$PORT" 2>/dev/null || true
        echo "✓ iptables DNS redirect rule removed."
    fi
fi
