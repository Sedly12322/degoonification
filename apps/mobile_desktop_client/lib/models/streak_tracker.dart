class Milestone {
  final int days;
  final String title;
  final String description;
  final String icon;

  const Milestone({
    required this.days,
    required this.title,
    required this.description,
    required this.icon,
  });
}

class RelapseEntry {
  final DateTime timestamp;
  final String reason;
  final int streakDaysLost;

  RelapseEntry({
    required this.timestamp,
    required this.reason,
    required this.streakDaysLost,
  });

  Map<String, dynamic> toJson() => {
    'timestamp': timestamp.toIso8601String(),
    'reason': reason,
    'streakDaysLost': streakDaysLost,
  };

  factory RelapseEntry.fromJson(Map<String, dynamic> json) => RelapseEntry(
    timestamp: DateTime.parse(json['timestamp'] as String),
    reason: json['reason'] as String? ?? '',
    streakDaysLost: json['streakDaysLost'] as int? ?? 0,
  );
}

class StreakTracker {
  static const List<Milestone> milestones = [
    Milestone(days: 1, title: 'First Step', description: '24 hours clean. The journey begins.', icon: '🌱'),
    Milestone(days: 3, title: 'Dopamine Stabilization', description: 'Receptor sensitivity starting to recover.', icon: '⚡'),
    Milestone(days: 7, title: '1 Week Milestone', description: 'Urges subside, concentration improves.', icon: '🔥'),
    Milestone(days: 14, title: 'Mental Clarity', description: 'Brain fog clearing, higher energy levels.', icon: '🧠'),
    Milestone(days: 30, title: 'Habit Overhaul', description: '1 Month clean! Compulsive pathways fading.', icon: '🛡️'),
    Milestone(days: 60, title: 'New Baseline', description: 'Natural emotional regulation restored.', icon: '💎'),
    Milestone(days: 90, title: 'Complete Neuro-Reboot', description: 'Full neural pathway rewiring achieved.', icon: '🏆'),
  ];

  DateTime streakStartDate;
  int longestStreakDays;
  final List<RelapseEntry> relapseHistory;

  StreakTracker({
    DateTime? streakStartDate,
    this.longestStreakDays = 0,
    List<RelapseEntry>? relapseHistory,
  })  : streakStartDate = streakStartDate ?? DateTime.now(),
        relapseHistory = relapseHistory ?? [];

  Duration get currentDuration => DateTime.now().difference(streakStartDate);
  int get currentDays => currentDuration.inDays;
  int get currentHours => currentDuration.inHours % 24;
  int get currentMinutes => currentDuration.inMinutes % 60;

  Milestone? get currentMilestone {
    Milestone? achieved;
    for (final m in milestones) {
      if (currentDays >= m.days) {
        achieved = m;
      }
    }
    return achieved;
  }

  Milestone? get nextMilestone {
    for (final m in milestones) {
      if (currentDays < m.days) {
        return m;
      }
    }
    return null;
  }

  double get nextMilestoneProgress {
    final next = nextMilestone;
    if (next == null) return 1.0;
    final prevDays = currentMilestone?.days ?? 0;
    final span = next.days - prevDays;
    final progress = currentDays - prevDays;
    return (progress / span).clamp(0.0, 1.0);
  }

  void logRelapse(String reason) {
    final daysLost = currentDays;
    relapseHistory.add(RelapseEntry(
      timestamp: DateTime.now(),
      reason: reason,
      streakDaysLost: daysLost,
    ));
    if (daysLost > longestStreakDays) {
      longestStreakDays = daysLost;
    }
    streakStartDate = DateTime.now();
  }
}
