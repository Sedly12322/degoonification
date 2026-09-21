export JAVA_HOME ?= $(HOME)/.local/share/jvm/jdk-21
export ANDROID_HOME ?= $(HOME)/.local/share/android-sdk
export PATH := $(JAVA_HOME)/bin:$(ANDROID_HOME)/cmdline-tools/latest/bin:$(ANDROID_HOME/platform-tools):$(HOME)/.local/bin:$(PATH)

GRADLE = $(HOME)/.local/bin/gradle
APK_PATH = app/build/outputs/apk/release/app-release.apk

.PHONY: all build clean install-device launch-device

all: build

build:
	$(GRADLE) assembleRelease

clean:
	$(GRADLE) clean

install-device: build
	adb install -r $(APK_PATH)

launch-device: install-device
	adb shell am start -n app.degoonification/.MainActivity

logs:
	adb logcat -s DegoonNSFWDetector ScreenCaptureService BlurOverlayService DnsFilterVpn
