package app.degoonification

import android.app.Activity
import android.app.admin.DevicePolicyManager
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.media.projection.MediaProjectionManager
import android.net.Uri
import android.net.VpnService
import android.os.Build
import android.os.Bundle
import android.provider.Settings
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import app.degoonification.admin.DegoonDeviceAdminReceiver
import app.degoonification.capture.ScreenCaptureService
import app.degoonification.databinding.ActivityMainBinding
import app.degoonification.engine.StreakRepository
import app.degoonification.vpn.DnsFilterVpnService

class MainActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMainBinding
    private lateinit var streakRepo: StreakRepository

    private var isDefenseRunning = false

    // MediaProjection Launcher
    private val captureLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK && result.data != null) {
            val serviceIntent = Intent(this, ScreenCaptureService::class.java).apply {
                action = ScreenCaptureService.ACTION_START
                putExtra(ScreenCaptureService.EXTRA_RESULT_CODE, result.resultCode)
                putExtra(ScreenCaptureService.EXTRA_RESULT_DATA, result.data)
            }
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
                startForegroundService(serviceIntent)
            } else {
                startService(serviceIntent)
            }
            isDefenseRunning = true
            updateDefenseUi()
            Toast.makeText(this, "🛡️ Visual Screen Defense Started", Toast.LENGTH_SHORT).show()
        } else {
            Toast.makeText(this, "Screen capture permission is required for visual blur", Toast.LENGTH_LONG).show()
        }
    }

    // VPN Preparation Launcher
    private val vpnLauncher = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { result ->
        if (result.resultCode == Activity.RESULT_OK) {
            startDnsVpn()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityMainBinding.inflate(layoutInflater)
        setContentView(binding.root)

        streakRepo = StreakRepository(this)
        updateStreakUi()

        binding.btnMasterAction.setOnClickListener {
            if (isDefenseRunning) {
                stopFullDefense()
            } else {
                startFullDefense()
            }
        }

        binding.btnRelapse.setOnClickListener {
            showRelapseDialog()
        }

        binding.btnSetupPermissions.setOnClickListener {
            checkAndRequestSpecialPermissions()
        }
    }

    override fun onResume() {
        super.onResume()
        updateStreakUi()
    }

    private fun updateStreakUi() {
        val days = streakRepo.getStreakDays()
        val milestone = streakRepo.getCurrentMilestone()
        binding.streakDays.text = days.toString()
        binding.streakIcon.text = milestone.icon
    }

    private fun updateDefenseUi() {
        if (isDefenseRunning) {
            binding.btnMasterAction.text = "STOP DEFENSE"
            binding.statusText.text = "SYSTEM DEFENSE ACTIVE"
            binding.statusText.setTextColor(getColor(R.color.status_green))
            binding.statusDot.setBackgroundColor(getColor(R.color.status_green))
        } else {
            binding.btnMasterAction.text = "START FULL DEFENSE"
            binding.statusText.text = "SYSTEM DEFENSE PAUSED"
            binding.statusText.setTextColor(getColor(R.color.danger_red))
            binding.statusDot.setBackgroundColor(getColor(R.color.danger_red))
        }
    }

    private fun startFullDefense() {
        // 1. Check Overlay Permission
        if (!Settings.canDrawOverlays(this)) {
            val intent = Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:$packageName")
            )
            startActivity(intent)
            Toast.makeText(this, "Please grant 'Appear on top' permission for blur overlay", Toast.LENGTH_LONG).show()
            return
        }

        // 2. Start DNS Sinkhole
        if (binding.switchDnsSinkhole.isChecked) {
            val vpnIntent = VpnService.prepare(this)
            if (vpnIntent != null) {
                vpnLauncher.launch(vpnIntent)
            } else {
                startDnsVpn()
            }
        }

        // 3. Start Visual Screen Capture
        if (binding.switchVisualBlur.isChecked) {
            val mpManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
            captureLauncher.launch(mpManager.createScreenCaptureIntent())
        }
    }

    private fun startDnsVpn() {
        val intent = Intent(this, DnsFilterVpnService::class.java).apply {
            action = DnsFilterVpnService.ACTION_START_VPN
        }
        startService(intent)
    }

    private fun stopFullDefense() {
        val captureIntent = Intent(this, ScreenCaptureService::class.java).apply {
            action = ScreenCaptureService.ACTION_STOP
        }
        startService(captureIntent)

        val vpnIntent = Intent(this, DnsFilterVpnService::class.java).apply {
            action = DnsFilterVpnService.ACTION_STOP_VPN
        }
        startService(vpnIntent)

        isDefenseRunning = false
        updateDefenseUi()
        Toast.makeText(this, "Defense paused", Toast.LENGTH_SHORT).show()
    }

    private fun checkAndRequestSpecialPermissions() {
        // 1. Overlay
        if (!Settings.canDrawOverlays(this)) {
            val intent = Intent(
                Settings.ACTION_MANAGE_OVERLAY_PERMISSION,
                Uri.parse("package:$packageName")
            )
            startActivity(intent)
            return
        }

        // 2. Device Admin
        val dpm = getSystemService(Context.DEVICE_POLICY_SERVICE) as DevicePolicyManager
        val adminComponent = ComponentName(this, DegoonDeviceAdminReceiver::class.java)
        if (!dpm.isAdminActive(adminComponent)) {
            val intent = Intent(DevicePolicyManager.ACTION_ADD_DEVICE_ADMIN).apply {
                putExtra(DevicePolicyManager.EXTRA_DEVICE_ADMIN, adminComponent)
                putExtra(
                    DevicePolicyManager.EXTRA_ADD_EXPLANATION,
                    "Activate Device Admin to protect against accidental uninstallation during vulnerable impulses."
                )
            }
            startActivity(intent)
            return
        }

        // 3. Accessibility
        val intent = Intent(Settings.ACTION_ACCESSIBILITY_SETTINGS)
        startActivity(intent)
        Toast.makeText(this, "Find 'Degoonification' under Installed apps and enable it", Toast.LENGTH_LONG).show()
    }

    private fun showRelapseDialog() {
        val input = android.widget.EditText(this).apply {
            hint = "Trigger notes (e.g. stress, fatigue...)"
        }
        AlertDialog.Builder(this)
            .setTitle("Log Relapse")
            .setMessage("Reset streak? Progress is rebuilt one day at a time.")
            .setView(input)
            .setPositiveButton("Reset") { _, _ ->
                streakRepo.logRelapse(input.text.toString())
                updateStreakUi()
                Toast.makeText(this, "Streak reset. Tomorrow starts day one.", Toast.LENGTH_SHORT).show()
            }
            .setNegativeButton("Cancel", null)
            .show()
    }
}
