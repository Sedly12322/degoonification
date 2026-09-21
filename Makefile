.PHONY: all run test clean install

all:
	$(MAKE) -C core/engine_shared
	$(MAKE) -C platform/linux

run: all
	@echo "=== Starting Degoonification Daemon ==="
	./platform/linux/bin/degoonification-daemon

test:
	$(MAKE) -C core/engine_shared test
	$(MAKE) -C platform/linux/network test
	$(MAKE) -C platform/linux/watchdog test

clean:
	$(MAKE) -C core/engine_shared clean
	$(MAKE) -C platform/linux clean
	$(MAKE) -C platform/linux/network clean
	$(MAKE) -C platform/linux/watchdog clean

install: all
	install -d /usr/local/bin
	install -m 755 platform/linux/bin/degoonification-daemon /usr/local/bin/
	install -d /etc/systemd/system
	install -m 644 platform/linux/watchdog/degoonification.service /etc/systemd/system/
	@echo "✓ Installed degoonification-daemon to /usr/local/bin"
