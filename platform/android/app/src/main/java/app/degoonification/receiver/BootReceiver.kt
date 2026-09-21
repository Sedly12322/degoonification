package app.degoonification.receiver

import android.content.BroadcastReceiver
import android.content.Context
import android.content.Intent
import app.degoonification.vpn.DnsFilterVpnService

class BootReceiver : BroadcastReceiver() {
    override fun onReceive(context: Context, intent: Intent) {
        if (intent.action == Intent.ACTION_BOOT_COMPLETED || intent.action == Intent.ACTION_MY_PACKAGE_REPLACED) {
            // Auto-start DNS Sinkhole on boot
            val vpnIntent = Intent(context, DnsFilterVpnService::class.java).apply {
                action = DnsFilterVpnService.ACTION_START_VPN
            }
            try {
                context.startService(vpnIntent)
            } catch (e: Exception) {
                e.printStackTrace()
            }
        }
    }
}
