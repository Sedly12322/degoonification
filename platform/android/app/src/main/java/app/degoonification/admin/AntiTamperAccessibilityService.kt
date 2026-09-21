package app.degoonification.admin

import android.accessibilityservice.AccessibilityService
import android.content.Intent
import android.util.Log
import android.view.accessibility.AccessibilityEvent

class AntiTamperAccessibilityService : AccessibilityService() {

    private val tag = "AntiTamperService"

    override fun onAccessibilityEvent(event: AccessibilityEvent?) {
        if (event == null) return

        val pkgName = event.packageName?.toString() ?: return

        // Intercept attempt to force-stop or uninstall via settings
        if (pkgName.contains("com.android.settings", ignoreCase = true) ||
            pkgName.contains("com.google.android.packageinstaller", ignoreCase = true)) {

            val text = event.text.toString()
            if (text.contains("degoonification", ignoreCase = true) &&
                (text.contains("uninstall", ignoreCase = true) || text.contains("force stop", ignoreCase = true))) {
                Log.w(tag, "Tamper attempt detected! Navigating back to home screen.")
                performGlobalAction(GLOBAL_ACTION_HOME)
            }
        }
    }

    override fun onInterrupt() {
        Log.w(tag, "AntiTamperAccessibilityService interrupted.")
    }
}
