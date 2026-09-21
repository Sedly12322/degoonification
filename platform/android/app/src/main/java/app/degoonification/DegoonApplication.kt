package app.degoonification

import android.app.Application
import android.app.NotificationChannel
import android.app.NotificationManager
import android.os.Build

class DegoonApplication : Application() {

    companion object {
        const val CHANNEL_DEFENSE_SERVICE = "degoon_defense_channel"
        const val CHANNEL_ALERT = "degoon_alert_channel"
    }

    override fun onCreate() {
        super.onCreate()
        createNotificationChannels()
    }

    private fun createNotificationChannels() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            val notificationManager = getSystemService(NotificationManager::class.java)

            val serviceChannel = NotificationChannel(
                CHANNEL_DEFENSE_SERVICE,
                "Degoonification System Defense",
                NotificationManager.IMPORTANCE_LOW
            ).apply {
                description = "Foreground status of real-time AI visual shield and DNS sinkhole"
                setShowBadge(false)
            }

            val alertChannel = NotificationChannel(
                CHANNEL_ALERT,
                "Security & Intervention Alerts",
                NotificationManager.IMPORTANCE_HIGH
            ).apply {
                description = "Intervention alerts when adult content is intercepted"
            }

            notificationManager.createNotificationChannel(serviceChannel)
            notificationManager.createNotificationChannel(alertChannel)
        }
    }
}
