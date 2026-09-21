package app.degoonification.capture

import android.app.Notification
import android.app.PendingIntent
import android.app.Service
import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.graphics.Bitmap
import android.graphics.PixelFormat
import android.hardware.display.DisplayManager
import android.hardware.display.VirtualDisplay
import android.media.ImageReader
import android.media.projection.MediaProjection
import android.media.projection.MediaProjectionManager
import android.os.Build
import android.os.Handler
import android.os.HandlerThread
import android.os.IBinder
import android.util.DisplayMetrics
import android.util.Log
import android.view.WindowManager
import androidx.core.app.NotificationCompat
import app.degoonification.DegoonApplication
import app.degoonification.MainActivity
import app.degoonification.R
import app.degoonification.engine.NSFWDetector
import app.degoonification.overlay.BlurOverlayService

class ScreenCaptureService : Service() {

    private val tag = "ScreenCaptureService"

    companion object {
        const val ACTION_START = "app.degoonification.action.START_CAPTURE"
        const val ACTION_STOP = "app.degoonification.action.STOP_CAPTURE"
        const val EXTRA_RESULT_CODE = "extra_result_code"
        const val EXTRA_RESULT_DATA = "extra_result_data"
        private const val NOTIFICATION_ID = 1001
    }

    private var mediaProjection: MediaProjection? = null
    private var virtualDisplay: VirtualDisplay? = null
    private var imageReader: ImageReader? = null
    private var backgroundThread: HandlerThread? = null
    private var backgroundHandler: Handler? = null

    private var overlayService: BlurOverlayService? = null
    private var isOverlayBound = false

    private lateinit var detector: NSFWDetector
    private var lastProcessTime = 0L
    private val throttleMs = 80L // ~12 FPS max for battery efficiency

    private val overlayConnection = object : ServiceConnection {
        override fun onServiceConnected(name: ComponentName?, service: IBinder?) {
            val binder = service as BlurOverlayService.LocalBinder
            overlayService = binder.getService()
            isOverlayBound = true
        }

        override fun onServiceDisconnected(name: ComponentName?) {
            overlayService = null
            isOverlayBound = false
        }
    }

    override fun onCreate() {
        super.onCreate()
        detector = NSFWDetector(this)
        backgroundThread = HandlerThread("DegoonCaptureThread").apply { start() }
        backgroundHandler = Handler(backgroundThread!!.looper)

        // Bind overlay service
        val overlayIntent = Intent(this, BlurOverlayService::class.java)
        startService(overlayIntent)
        bindService(overlayIntent, overlayConnection, Context.BIND_AUTO_CREATE)
    }

    override fun onStartCommand(intent: Intent?, flags: Int, startId: Int): Int {
        when (intent?.action) {
            ACTION_START -> {
                val resultCode = intent.getIntExtra(EXTRA_RESULT_CODE, 0)
                val resultData: Intent? = if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.TIRAMISU) {
                    intent.getParcelableExtra(EXTRA_RESULT_DATA, Intent::class.java)
                } else {
                    @Suppress("DEPRECATION")
                    intent.getParcelableExtra(EXTRA_RESULT_DATA)
                }

                startForeground(NOTIFICATION_ID, buildNotification())

                if (resultData != null) {
                    startCapture(resultCode, resultData)
                }
            }
            ACTION_STOP -> {
                stopCapture()
                stopForeground(STOP_FOREGROUND_REMOVE)
                stopSelf()
            }
        }
        return START_NOT_STICKY
    }

    private fun startCapture(resultCode: Int, resultData: Intent) {
        val mpManager = getSystemService(Context.MEDIA_PROJECTION_SERVICE) as MediaProjectionManager
        mediaProjection = mpManager.getMediaProjection(resultCode, resultData)

        val windowManager = getSystemService(Context.WINDOW_SERVICE) as WindowManager
        val metrics = DisplayMetrics()
        @Suppress("DEPRECATION")
        windowManager.defaultDisplay.getRealMetrics(metrics)

        // Downscale capture to 1/2 resolution for extreme speed & zero lag
        val width = metrics.widthPixels / 2
        val height = metrics.heightPixels / 2
        val density = metrics.densityDpi

        imageReader = ImageReader.newInstance(width, height, PixelFormat.RGBA_8888, 2)
        virtualDisplay = mediaProjection?.createVirtualDisplay(
            "DegoonVirtualDisplay",
            width,
            height,
            density,
            DisplayManager.VIRTUAL_DISPLAY_FLAG_AUTO_MIRROR,
            imageReader?.surface,
            null,
            backgroundHandler
        )

        imageReader?.setOnImageAvailableListener({ reader ->
            val now = System.currentTimeMillis()
            if (now - lastProcessTime < throttleMs) {
                // Throttle frame processing to maintain silky 120Hz system responsiveness
                val img = reader.acquireLatestImage()
                img?.close()
                return@setOnImageAvailableListener
            }
            lastProcessTime = now

            val image = reader.acquireLatestImage() ?: return@setOnImageAvailableListener
            try {
                val planes = image.planes
                val buffer = planes[0].buffer
                val pixelStride = planes[0].pixelStride
                val rowStride = planes[0].rowStride
                val rowPadding = rowStride - pixelStride * image.width

                val bitmap = Bitmap.createBitmap(
                    image.width + rowPadding / pixelStride,
                    image.height,
                    Bitmap.Config.ARGB_8888
                )
                bitmap.copyPixelsFromBuffer(buffer)

                // Run AI NSFW Detection
                val boxes = detector.detect(bitmap)

                // Scale boxes back to real screen coordinates
                val scaleX = metrics.widthPixels.toFloat() / bitmap.width
                val scaleY = metrics.heightPixels.toFloat() / bitmap.height

                val scaledBoxes = boxes.map { r ->
                    android.graphics.RectF(
                        r.left * scaleX,
                        r.top * scaleY,
                        r.right * scaleX,
                        r.bottom * scaleY
                    )
                }

                overlayService?.updateBlurBoxes(scaledBoxes)
                bitmap.recycle()
            } catch (e: Exception) {
                Log.e(tag, "Frame capture error: ${e.message}")
            } finally {
                image.close()
            }
        }, backgroundHandler)

        Log.i(tag, "✓ MediaProjection screen capture started.")
    }

    private fun stopCapture() {
        virtualDisplay?.release()
        virtualDisplay = null
        imageReader?.close()
        imageReader = null
        mediaProjection?.stop()
        mediaProjection = null
        overlayService?.updateBlurBoxes(emptyList())
    }

    private fun buildNotification(): Notification {
        val pendingIntent = PendingIntent.getActivity(
            this,
            0,
            Intent(this, MainActivity::class.java),
            PendingIntent.FLAG_IMMUTABLE
        )

        return NotificationCompat.Builder(this, DegoonApplication.CHANNEL_DEFENSE_SERVICE)
            .setContentTitle("Degoonification Shield Active")
            .setContentText("Real-time visual defense & DNS sinkhole active")
            .setSmallIcon(R.mipmap.ic_launcher)
            .setContentIntent(pendingIntent)
            .setOngoing(true)
            .setPriority(NotificationCompat.PRIORITY_LOW)
            .build()
    }

    override fun onDestroy() {
        super.onDestroy()
        stopCapture()
        if (isOverlayBound) {
            unbindService(overlayConnection)
            isOverlayBound = false
        }
        backgroundThread?.quitSafely()
    }

    override fun onBind(intent: Intent?): IBinder? = null
}
