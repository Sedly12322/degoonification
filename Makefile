.PHONY: all run test clean install

all:
	$(MAKE) -C core/engine_shared
	$(MAKE) -C platform/linux
	$(MAKE) -C apps/browser_extension

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
	$(MAKE) -C apps/browser_extension clean

install: all
	install -d $(HOME)/.local/bin
	install -d $(HOME)/.local/share/degoonification/models
	install -d $(HOME)/.local/share/degoonification/blocklists
	install -d $(HOME)/.config/systemd/user
	install -m 755 platform/linux/bin/degoon $(HOME)/.local/bin/degoon
	install -m 755 platform/linux/bin/degoonification-daemon $(HOME)/.local/bin/degoonification-daemon
	install -m 644 core/models/yolov8n-nsfw.onnx $(HOME)/.local/share/degoonification/models/
	install -m 644 core/blocklists/default_domains.txt $(HOME)/.local/share/degoonification/blocklists/
	install -m 644 platform/linux/watchdog/degoonification-user.service $(HOME)/.config/systemd/user/degoonification.service
	systemctl --user daemon-reload || true
	$(MAKE) -C apps/browser_extension install-profile
	@echo "✓ Installed 'degoon' and 'degoonification-daemon' to $(HOME)/.local/bin"
	@echo "✓ Installed model and blocklists to $(HOME)/.local/share/degoonification"
	@echo "✓ Installed systemd user service to $(HOME)/.config/systemd/user/degoonification.service"

install-system: all
	install -d /usr/local/bin
	install -d /usr/local/share/degoonification/models
	install -d /usr/local/share/degoonification/blocklists
	install -m 755 platform/linux/bin/degoon /usr/local/bin/degoon
	install -m 755 platform/linux/bin/degoonification-daemon /usr/local/bin/degoonification-daemon
	install -m 644 core/models/yolov8n-nsfw.onnx /usr/local/share/degoonification/models/
	install -m 644 core/blocklists/default_domains.txt /usr/local/share/degoonification/blocklists/
	install -d /etc/systemd/system
	install -m 644 platform/linux/watchdog/degoonification.service /etc/systemd/system/
	@echo "✓ System installation complete."
