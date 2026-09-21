import 'package:flutter/material.dart';
import 'models/streak_tracker.dart';

void main() {
  runApp(const DegoonApp());
}

class DegoonApp extends StatelessWidget {
  const DegoonApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Degoonification',
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0F172A),
        colorScheme: const ColorScheme.dark(
          primary: Color(0xFF38BDF8),
          secondary: Color(0xFF818CF8),
          surface: Color(0xFF1E293B),
        ),
        fontFamily: 'Inter',
        cardTheme: CardTheme(
          color: const Color(0xFF1E293B),
          elevation: 0,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(16),
            side: const BorderSide(color: Color(0xFF334155), width: 1),
          ),
        ),
      ),
      home: const DashboardScreen(),
    );
  }
}

class DashboardScreen extends StatefulWidget {
  const DashboardScreen({super.key});

  @override
  State<DashboardScreen> createState() => _DashboardScreenState();
}

class _DashboardScreenState extends State<DashboardScreen> {
  final StreakTracker _streakTracker = StreakTracker();

  bool _visualBlurActive = true;
  bool _dnsSinkholeActive = true;
  bool _antiTamperActive = true;
  double _paddingRatio = 0.15; // 15% dynamic expansion

  @override
  Widget build(BuildContext context) {
    final nextM = _streakTracker.nextMilestone;
    final currentM = _streakTracker.currentMilestone;

    return Scaffold(
      appBar: AppBar(
        backgroundColor: const Color(0xFF0F172A),
        elevation: 0,
        title: Row(
          children: [
            Container(
              padding: const EdgeInsets.all(8),
              decoration: BoxDecoration(
                color: const Color(0xFF38BDF8).withOpacity(0.15),
                borderRadius: BorderRadius.circular(10),
              ),
              child: const Icon(Icons.shield_outlined, color: Color(0xFF38BDF8), size: 22),
            ),
            const SizedBox(width: 12),
            const Text(
              'Degoonification',
              style: TextStyle(fontWeight: FontWeight.bold, fontSize: 18),
            ),
          ],
        ),
        actions: [
          IconButton(
            icon: const Icon(Icons.settings_outlined),
            onPressed: () => _showSettingsDialog(),
          ),
        ],
      ),
      body: SingleChildScrollView(
        padding: const EdgeInsets.all(20),
        child: Column(
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            // 1. Protection Status Banner
            Card(
              child: Padding(
                padding: const EdgeInsets.all(20),
                child: Column(
                  children: [
                    Row(
                      children: [
                        Container(
                          width: 12,
                          height: 12,
                          decoration: const BoxDecoration(
                            color: Color(0xFF22C55E),
                            shape: BoxShape.circle,
                          ),
                        ),
                        const SizedBox(width: 10),
                        const Text(
                          'SYSTEM DEFENSE ACTIVE',
                          style: TextStyle(
                            color: Color(0xFF22C55E),
                            fontWeight: FontWeight.bold,
                            fontSize: 12,
                            letterSpacing: 1.2,
                          ),
                        ),
                        const Spacer(),
                        const Text(
                          '100% On-Device Air-Gapped',
                          style: TextStyle(color: Color(0xFF64748B), fontSize: 12),
                        ),
                      ],
                    ),
                    const SizedBox(height: 16),
                    Row(
                      mainAxisAlignment: MainAxisAlignment.spaceAround,
                      children: [
                        _statusPill('Visual Blur', _visualBlurActive, Icons.visibility_off_outlined),
                        _statusPill('DNS Sinkhole', _dnsSinkholeActive, Icons.dns_outlined),
                        _statusPill('Anti-Tamper', _antiTamperActive, Icons.lock_outline),
                      ],
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 20),

            // 2. Streak Counter & Flame Card
            Card(
              child: Padding(
                padding: const EdgeInsets.all(24),
                child: Column(
                  children: [
                    Text(
                      currentM?.icon ?? '🌱',
                      style: const TextStyle(fontSize: 48),
                    ),
                    const SizedBox(height: 8),
                    Text(
                      '${_streakTracker.currentDays}',
                      style: const TextStyle(
                        fontSize: 64,
                        fontWeight: FontWeight.w900,
                        color: Colors.white,
                        height: 1.0,
                      ),
                    ),
                    const Text(
                      'DAYS CLEAN',
                      style: TextStyle(
                        color: Color(0xFF94A3B8),
                        fontWeight: FontWeight.bold,
                        letterSpacing: 2,
                        fontSize: 13,
                      ),
                    ),
                    const SizedBox(height: 12),
                    Text(
                      '${_streakTracker.currentHours}h ${_streakTracker.currentMinutes}m',
                      style: const TextStyle(
                        color: Color(0xFF38BDF8),
                        fontWeight: FontWeight.w600,
                        fontSize: 14,
                      ),
                    ),
                    const SizedBox(height: 20),
                    if (nextM != null) ...[
                      Row(
                        mainAxisAlignment: MainAxisAlignment.spaceBetween,
                        children: [
                          Text(
                            'Next: ${nextM.title} (${nextM.days}d)',
                            style: const TextStyle(color: Color(0xFF94A3B8), fontSize: 13),
                          ),
                          Text(
                            '${(_streakTracker.nextMilestoneProgress * 100).toInt()}%',
                            style: const TextStyle(
                              color: Color(0xFF38BDF8),
                              fontWeight: FontWeight.bold,
                              fontSize: 13,
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 8),
                      ClipRRect(
                        borderRadius: BorderRadius.circular(8),
                        child: LinearProgressIndicator(
                          value: _streakTracker.nextMilestoneProgress,
                          minHeight: 8,
                          backgroundColor: const Color(0xFF334155),
                          valueColor: const AlwaysStoppedAnimation<Color>(Color(0xFF38BDF8)),
                        ),
                      ),
                      const SizedBox(height: 8),
                      Text(
                        nextM.description,
                        textAlign: TextAlign.center,
                        style: const TextStyle(color: Color(0xFF64748B), fontSize: 12),
                      ),
                    ],
                    const SizedBox(height: 20),
                    OutlinedButton.icon(
                      onPressed: () => _showRelapseDialog(),
                      style: OutlinedButton.styleFrom(
                        foregroundColor: const Color(0xFFEF4444),
                        side: const BorderSide(color: Color(0xFFEF4444)),
                        shape: RoundedRectangleBorder(borderRadius: BorderRadius.circular(10)),
                      ),
                      icon: const Icon(Icons.refresh, size: 16),
                      label: const Text('Log Relapse / Reset Streak'),
                    ),
                  ],
                ),
              ),
            ),
            const SizedBox(height: 20),

            // 3. Quick Defense Controls
            Card(
              child: Padding(
                padding: const EdgeInsets.all(20),
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    const Text(
                      'DEFENSE SUBSYSTEMS',
                      style: TextStyle(
                        fontSize: 12,
                        fontWeight: FontWeight.bold,
                        color: Color(0xFF94A3B8),
                        letterSpacing: 1.2,
                      ),
                    ),
                    const SizedBox(height: 12),
                    SwitchListTile(
                      contentPadding: EdgeInsets.zero,
                      title: const Text('Real-Time Screen Blur'),
                      subtitle: const Text('Zero-latency AI detection with +15% dynamic padding'),
                      value: _visualBlurActive,
                      onChanged: (v) => setState(() => _visualBlurActive = v),
                    ),
                    const Divider(color: Color(0xFF334155)),
                    SwitchListTile(
                      contentPadding: EdgeInsets.zero,
                      title: const Text('DNS Sinkhole (Port 53)'),
                      subtitle: const Text('In-memory Radix tree dropping adult domains to 0.0.0.0'),
                      value: _dnsSinkholeActive,
                      onChanged: (v) => setState(() => _dnsSinkholeActive = v),
                    ),
                    const Divider(color: Color(0xFF334155)),
                    SwitchListTile(
                      contentPadding: EdgeInsets.zero,
                      title: const Text('Anti-Tamper Watchdog'),
                      subtitle: const Text('Process memory shielding and automatic respawn'),
                      value: _antiTamperActive,
                      onChanged: (v) => setState(() => _antiTamperActive = v),
                    ),
                  ],
                ),
              ),
            ),
          ],
        ),
      ),
    );
  }

  Widget _statusPill(String label, bool active, IconData icon) {
    return Container(
      padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 8),
      decoration: BoxDecoration(
        color: active ? const Color(0xFF0284C7).withOpacity(0.15) : const Color(0xFF334155).withOpacity(0.3),
        borderRadius: BorderRadius.circular(20),
        border: Border.pad(
          BorderSide(
            color: active ? const Color(0xFF38BDF8).withOpacity(0.4) : const Color(0xFF475569),
            width: 1,
          ),
        ),
      ),
      child: Row(
        mainAxisSize: MainAxisSize.min,
        children: [
          Icon(icon, size: 14, color: active ? const Color(0xFF38BDF8) : const Color(0xFF94A3B8)),
          const SizedBox(width: 6),
          Text(
            label,
            style: TextStyle(
              color: active ? Colors.white : const Color(0xFF94A3B8),
              fontSize: 12,
              fontWeight: FontWeight.w600,
            ),
          ),
        ],
      ),
    );
  }

  void _showSettingsDialog() {
    showDialog(
      context: context,
      builder: (ctx) => StatefulBuilder(
        builder: (ctx, setDialogState) => AlertDialog(
          backgroundColor: const Color(0xFF1E293B),
          title: const Text('Defense Configuration'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.start,
            children: [
              Text('Blur Padding Expansion: +${(_paddingRatio * 100).toInt()}%'),
              Slider(
                value: _paddingRatio,
                min: 0.10,
                max: 0.35,
                divisions: 5,
                label: '+${(_paddingRatio * 100).toInt()}%',
                onChanged: (v) {
                  setDialogState(() => _paddingRatio = v);
                  setState(() => _paddingRatio = v);
                },
              ),
              const SizedBox(height: 8),
              const Text(
                'Temporal Smoothing Hysteresis: 300 ms\n'
                'AI Model: YOLOv8n-NSFW (ONNX / TFLite)\n'
                'Privacy: 100% Local Inference',
                style: TextStyle(color: Color(0xFF94A3B8), fontSize: 12),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(ctx),
              child: const Text('Done'),
            ),
          ],
        ),
      ),
    );
  }

  void _showRelapseDialog() {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (ctx) => AlertDialog(
        backgroundColor: const Color(0xFF1E293B),
        title: const Text('Log Relapse'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            const Text(
              'A slip is not failure unless you stop trying. What triggered this urge?',
              style: TextStyle(color: Color(0xFF94A3B8), fontSize: 13),
            ),
            const SizedBox(height: 12),
            TextField(
              controller: controller,
              decoration: const InputDecoration(
                hintText: 'e.g. Late night browsing, stress, social media...',
                border: OutlineInputBorder(),
              ),
            ),
          ],
        ),
        actions: [
          TextButton(
            onPressed: () => Navigator.pop(ctx),
            child: const Text('Cancel'),
          ),
          ElevatedButton(
            style: ElevatedButton.styleFrom(
              backgroundColor: const Color(0xFFEF4444),
            ),
            onPressed: () {
              setState(() {
                _streakTracker.logRelapse(controller.text);
              });
              Navigator.pop(ctx);
            },
            child: const Text('Confirm Reset'),
          ),
        ],
      ),
    );
  }
}
