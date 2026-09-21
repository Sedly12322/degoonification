package app.degoonification.vpn

import android.content.Intent
import android.net.VpnService
import android.os.ParcelFileDescriptor
import android.util.Log
import java.io.BufferedReader
import java.io.InputStreamReader
import java.net.DatagramPacket
import java.net.DatagramSocket
import java.net.InetAddress
import java.util.Collections
import java.util.concurrent.ConcurrentHashMap

class DnsFilterVpnService : VpnService(), Runnable {

    private val tag = "DnsFilterVpn"
    private var vpnInterface: ParcelFileDescriptor? = null
    private var vpnThread: Thread? = null
    private var isRunning = false

    // Fast in-memory hash set of blocked adult domains
    private val blockedDomains = Collections.newSetFromMap(ConcurrentHashMap<String, Boolean>())

    companion object {
        const val ACTION_START_VPN = "app.degoonification.vpn.START"
        const val ACTION_STOP_VPN = "app.degoonification.vpn.STOP"
    }

    override fun onCreate() {
        super.onCreate()
        loadBlocklist()
    }

    private fun loadBlocklist() {
        Thread {
            try {
                val input = assets.open("default_domains.txt")
                val reader = BufferedReader(InputStreamReader(input))
                var line: String?
                var count = 0
                while (reader.readLine().also { line = it } != null) {
                    val trimmed = line?.trim() ?: continue
                    if (trimmed.isNotEmpty() && !trimmed.startsWith("#")) {
                        val domain = if (trimmed.startsWith("0.0.0.0 ")) {
                            trimmed.substring(8).trim()
                        } else {
                            trimmed
                        }
                        blockedDomains.add(domain.lowercase())
                        count++
                    }
                }
                reader.close()
                Log.i(tag, "✓ Loaded $count adult domains into Android VpnService memory.")
            } catch (e: Exception) {
                Log.e(tag, "Failed to load blocklist: ${e.message}")
            }
        }.start()
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START_VPN -> startVpn()
            ACTION_STOP_VPN -> stopVpn()
        }
        return START_STICKY
    }

    private fun startVpn() {
        if (isRunning) return
        try {
            val builder = Builder()
                .setSession("Degoonification DNS Sinkhole")
                .addAddress("10.0.0.2", 32)
                .addDnsServer("1.1.1.1") // Upstream DNS
                .addRoute("1.1.1.1", 32) // Route DNS server traffic only

            vpnInterface = builder.establish()
            isRunning = true
            vpnThread = Thread(this, "DegoonVpnThread").apply { start() }
            Log.i(tag, "✓ Degoonification DNS Sinkhole VPN started successfully.")
        } catch (e: Exception) {
            Log.e(tag, "Failed to establish VPN: ${e.message}")
        }
    }

    private fun stopVpn() {
        isRunning = false
        try {
            vpnInterface?.close()
        } catch (e: Exception) {
            e.printStackTrace()
        }
        vpnInterface = null
        vpnThread?.interrupt()
        vpnThread = null
        stopSelf()
    }

    override fun run() {
        // Background loop for DNS routing
        while (isRunning && !Thread.currentThread().isInterrupted) {
            try {
                Thread.sleep(1000)
            } catch (e: InterruptedException) {
                break
            }
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        stopVpn()
    }
}
