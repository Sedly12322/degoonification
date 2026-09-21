XPI_NAME = degoonification-zen.xpi
ZEN_PROFILE = $(HOME)/.config/zen/jlgm1jgg.Default (release)

.PHONY: all clean xpi install-profile

all: xpi

xpi:
	@rm -f $(XPI_NAME)
	zip -r -FS $(XPI_NAME) manifest.json background.js content.js content.css popup.html popup.css popup.js icons/
	@echo "✓ Packaged WebExtension: $(XPI_NAME)"

clean:
	rm -f $(XPI_NAME)

install-profile: xpi
	@mkdir -p "$(ZEN_PROFILE)/extensions"
	@cp -f $(XPI_NAME) "$(ZEN_PROFILE)/extensions/degoon@degoonification.app.xpi"
	@echo "✓ Installed extension into Zen Browser profile: $(ZEN_PROFILE)/extensions/degoon@degoonification.app.xpi"
