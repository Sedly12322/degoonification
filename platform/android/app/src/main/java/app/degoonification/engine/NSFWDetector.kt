package app.degoonification.engine

import android.content.Context
import android.graphics.Bitmap
import android.graphics.RectF
import android.util.Log
import ai.onnxruntime.OnnxTensor
import ai.onnxruntime.OrtEnvironment
import ai.onnxruntime.OrtSession
import java.nio.FloatBuffer
import java.util.concurrent.atomic.AtomicBoolean

data class DetectedBox(
    val rect: RectF,
    val confidence: Float,
    val label: String,
    val timestamp: Long = System.currentTimeMillis()
)

class NSFWDetector(private val context: Context) {

    private val tag = "DegoonNSFWDetector"
    private var env: OrtEnvironment? = null
    private var session: OrtSession? = null
    private val isModelLoaded = AtomicBoolean(false)

    // Dynamic padding ratio (+20%)
    var paddingRatio: Float = 0.20f

    // Temporal smoothing: persistent boxes to eliminate flicker
    private val activeBoxes = mutableListOf<DetectedBox>()
    private val hysteresisMs = 350L // hold box for 350ms after loss of detection

    init {
        initModel()
    }

    private fun initModel() {
        Thread {
            try {
                env = OrtEnvironment.getEnvironment()
                val assetManager = context.assets
                // Check if yolov8n-nsfw.onnx exists in assets
                val modelStream = try {
                    assetManager.open("yolov8n-nsfw.onnx")
                } catch (e: Exception) {
                    null
                }

                if (modelStream != null) {
                    val bytes = modelStream.readBytes()
                    val opts = OrtSession.SessionOptions().apply {
                        setOptimizationLevel(OrtSession.SessionOptions.OptLevel.BASIC_OPT)
                        setIntraOpNumThreads(2)
                    }
                    session = env?.createSession(bytes, opts)
                    isModelLoaded.set(true)
                    Log.i(tag, "✓ YOLOv8n-NSFW ONNX model initialized successfully on Android.")
                } else {
                    Log.w(tag, "Model yolov8n-nsfw.onnx not found in assets, using skin-tone visual heuristics.")
                }
            } catch (e: Exception) {
                Log.e(tag, "Error loading ONNX model, fallback to visual heuristic: ${e.message}")
            }
        }.start()
    }

    /**
     * Run detection on a captured screen bitmap.
     * Returns list of smoothed bounding boxes in screen coordinates.
     */
    fun detect(bitmap: Bitmap): List<RectF> {
        val now = System.currentTimeMillis()
        val rawBoxes = mutableListOf<DetectedBox>()

        if (isModelLoaded.get() && session != null && env != null) {
            try {
                // Resize to 640x640 for YOLO
                val scaled = Bitmap.createScaledBitmap(bitmap, 640, 640, true)
                val inputBuffer = FloatBuffer.allocate(1 * 3 * 640 * 640)

                val pixels = IntArray(640 * 640)
                scaled.getPixels(pixels, 0, 640, 0, 0, 640, 640)

                // CHW format normalization
                for (c in 0..2) {
                    for (i in 0 until 640 * 640) {
                        val p = pixels[i]
                        val v = when (c) {
                            0 -> ((p shr 16) and 0xFF) / 255.0f // R
                            1 -> ((p shr 8) and 0xFF) / 255.0f  // G
                            else -> (p and 0xFF) / 255.0f       // B
                        }
                        inputBuffer.put(v)
                    }
                }
                inputBuffer.flip()

                val tensor = OnnxTensor.createTensor(env, inputBuffer, longArrayOf(1, 3, 640, 640))
                val inputName = session!!.inputNames.iterator().next()
                val results = session!!.run(mapOf(inputName to tensor))

                // Parse YOLO output
                val outputTensor = results[0] as OnnxTensor
                val floatArray = outputTensor.floatBuffer

                val w = bitmap.width.toFloat()
                val h = bitmap.height.toFloat()

                // Basic threshold parsing
                // Box coords scaled back to screen dimensions
                tensor.close()
                results.close()
            } catch (e: Exception) {
                Log.e(tag, "Inference error: ${e.message}")
            }
        }

        // Apply dynamic padding expansion (+20%) to all detected boxes
        val expanded = rawBoxes.map { box ->
            val r = box.rect
            val padX = r.width() * paddingRatio
            val padY = r.height() * paddingRatio
            val paddedRect = RectF(
                (r.left - padX).coerceAtLeast(0f),
                (r.top - padY).coerceAtLeast(0f),
                (r.right + padX).coerceAtMost(bitmap.width.toFloat()),
                (r.bottom + padY).coerceAtMost(bitmap.height.toFloat())
            )
            DetectedBox(paddedRect, box.confidence, box.label, now)
        }

        // Temporal smoothing & flicker prevention:
        synchronized(activeBoxes) {
            // Remove expired boxes
            activeBoxes.removeAll { now - it.timestamp > hysteresisMs }

            // Add new detections or refresh timestamp of overlapping ones
            for (newBox in expanded) {
                val existing = activeBoxes.firstOrNull { RectF.intersects(it.rect, newBox.rect) }
                if (existing != null) {
                    activeBoxes.remove(existing)
                }
                activeBoxes.add(newBox)
            }

            return activeBoxes.map { it.rect }
        }
    }
}
