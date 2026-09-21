package app.degoonification.overlay

import android.content.Context
import android.graphics.Canvas
import android.graphics.Color
import android.graphics.Paint
import android.graphics.RectF
import android.util.AttributeSet
import android.view.View

class BlurCanvasView @JvmOverloads constructor(
    context: Context,
    attrs: AttributeSet? = null,
    defStyleAttr: Int = 0
) : View(context, attrs, defStyleAttr) {

    private val boxPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        // Deep obsidian privacy fill
        color = Color.parseColor("#E60F172A") // 90% opacity dark slate
        style = Paint.Style.FILL
    }

    private val borderPaint = Paint(Paint.ANTI_ALIAS_FLAG).apply {
        color = Color.parseColor("#4D38BDF8") // Subtle cyber cyan border
        style = Paint.Style.STROKE
        strokeWidth = 3f
    }

    private val boxes = mutableListOf<RectF>()

    fun updateBoxes(newBoxes: List<RectF>) {
        boxes.clear()
        boxes.addAll(newBoxes)
        postInvalidate()
    }

    override fun onDraw(canvas: Canvas) {
        super.onDraw(canvas)
        for (rect in boxes) {
            // Draw smooth rounded privacy rectangle over detected trigger area
            canvas.drawRoundRect(rect, 24f, 24f, boxPaint)
            canvas.drawRoundRect(rect, 24f, 24f, borderPaint)
        }
    }
}
