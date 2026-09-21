package app.degoonification.engine

import android.content.Context
import android.content.SharedPreferences

data class Milestone(
    val days: Int,
    val title: String,
    val icon: String,
    val description: String
)

class StreakRepository(context: Context) {

    private val prefs: SharedPreferences =
        context.getSharedPreferences("degoon_streak_prefs", Context.MODE_PRIVATE)

    companion object {
        private const val KEY_START_TIME = "streak_start_time"
        private const val KEY_RELAPSE_COUNT = "streak_relapse_count"
        private const val KEY_LAST_RELAPSE_REASON = "streak_last_relapse_reason"

        val MILESTONES = listOf(
            Milestone(0, "Initiation", "🌱", "Beginning neural detox. Urges will peak in the first 72 hours."),
            Milestone(3, "The Wall", "🔥", "Dopamine receptors craving baseline stimulation. Stay vigilant."),
            Milestone(7, "Clarity Horizon", "⚡", "Energy recovery starting. Brain fog starts diminishing."),
            Milestone(14, "Neuroplastic Shift", "🧠", "Frontal lobe control strengthening. Cravings decrease."),
            Milestone(30, "Metabolic Baseline", "🛡️", "Significant neuroplastic repair. Dopamine baseline stabilizing."),
            Milestone(60, "Rewired Identity", "💎", "New habits established. Old triggers lose visceral power."),
            Milestone(90, "Reboot Complete", "👑", "Full dopamine receptor density recovery.")
        )
    }

    init {
        if (!prefs.contains(KEY_START_TIME)) {
            prefs.edit().putLong(KEY_START_TIME, System.currentTimeMillis()).apply()
        }
    }

    fun getStreakDays(): Int {
        val start = prefs.getLong(KEY_START_TIME, System.currentTimeMillis())
        val diffMs = System.currentTimeMillis() - start
        return (diffMs / (1000 * 60 * 60 * 24)).toInt().coerceAtLeast(0)
    }

    fun getStreakHours(): Int {
        val start = prefs.getLong(KEY_START_TIME, System.currentTimeMillis())
        val diffMs = System.currentTimeMillis() - start
        return ((diffMs / (1000 * 60 * 60)) % 24).toInt().coerceAtLeast(0)
    }

    fun getCurrentMilestone(): Milestone {
        val days = getStreakDays()
        return MILESTONES.lastOrNull { it.days <= days } ?: MILESTONES.first()
    }

    fun logRelapse(reason: String) {
        val relapses = prefs.getInt(KEY_RELAPSE_COUNT, 0)
        prefs.edit()
            .putLong(KEY_START_TIME, System.currentTimeMillis())
            .putInt(KEY_RELAPSE_COUNT, relapses + 1)
            .putString(KEY_LAST_RELAPSE_REASON, reason)
            .apply()
    }
}
