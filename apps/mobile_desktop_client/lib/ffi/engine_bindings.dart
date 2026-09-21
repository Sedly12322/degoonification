import 'dart:ffi';
import 'dart:io';
import 'package:ffi/ffi.dart';

// Native struct matching dg_bounding_box_t
final class DgBoundingBox extends Struct {
  @Float()
  external double x;

  @Float()
  external double y;

  @Float()
  external double width;

  @Float()
  external double height;

  @Float()
  external double confidence;

  @Int32()
  external int classId;

  @Uint64()
  external int timestampMs;
}

// Opaque native types
final class DgTracker extends Opaque {}
final class DgBloom extends Opaque {}
final class DgRadix extends Opaque {}

// C ABI function signatures
typedef DgTrackerCreateC = Pointer<DgTracker> Function(
  Uint64 persistenceWindowMs,
  Float paddingRatio,
  Float mergeIouThreshold,
);
typedef DgTrackerCreateDart = Pointer<DgTracker> Function(
  int persistenceWindowMs,
  double paddingRatio,
  double mergeIouThreshold,
);

typedef DgTrackerDestroyC = Void Function(Pointer<DgTracker> tracker);
typedef DgTrackerDestroyDart = void Function(Pointer<DgTracker> tracker);

typedef DgTrackerResetC = Void Function(Pointer<DgTracker> tracker);
typedef DgTrackerResetDart = void Function(Pointer<DgTracker> tracker);

typedef DgTrackerActiveCountC = Uint64 Function(Pointer<DgTracker> tracker);
typedef DgTrackerActiveCountDart = int Function(Pointer<DgTracker> tracker);

typedef DgBloomCreateC = Pointer<DgBloom> Function(Uint64 expectedElements, Double fpRate);
typedef DgBloomCreateDart = Pointer<DgBloom> Function(int expectedElements, double fpRate);

typedef DgBloomDestroyC = Void Function(Pointer<DgBloom> bf);
typedef DgBloomDestroyDart = void Function(Pointer<DgBloom> bf);

typedef DgBloomAddC = Void Function(Pointer<DgBloom> bf, Pointer<Utf8> key);
typedef DgBloomAddDart = void Function(Pointer<DgBloom> bf, Pointer<Utf8> key);

typedef DgBloomContainsC = Bool Function(Pointer<DgBloom> bf, Pointer<Utf8> key);
typedef DgBloomContainsDart = bool Function(Pointer<DgBloom> bf, Pointer<Utf8> key);

typedef DgRadixCreateC = Pointer<DgRadix> Function();
typedef DgRadixCreateDart = Pointer<DgRadix> Function();

typedef DgRadixDestroyC = Void Function(Pointer<DgRadix> tree);
typedef DgRadixDestroyDart = void Function(Pointer<DgRadix> tree);

typedef DgRadixInsertC = Void Function(Pointer<DgRadix> tree, Pointer<Utf8> domain);
typedef DgRadixInsertDart = void Function(Pointer<DgRadix> tree, Pointer<Utf8> domain);

typedef DgRadixMatchesC = Bool Function(Pointer<DgRadix> tree, Pointer<Utf8> domain);
typedef DgRadixMatchesDart = bool Function(Pointer<DgRadix> tree, Pointer<Utf8> domain);

typedef DgRadixSizeC = Uint64 Function(Pointer<DgRadix> tree);
typedef DgRadixSizeDart = int Function(Pointer<DgRadix> tree);

class DegoonEngine {
  late final DynamicLibrary _lib;

  late final DgTrackerCreateDart _trackerCreate;
  late final DgTrackerDestroyDart _trackerDestroy;
  late final DgTrackerResetDart _trackerReset;
  late final DgTrackerActiveCountDart _trackerActiveCount;

  late final DgBloomCreateDart _bloomCreate;
  late final DgBloomDestroyDart _bloomDestroy;
  late final DgBloomAddDart _bloomAdd;
  late final DgBloomContainsDart _bloomContains;

  late final DgRadixCreateDart _radixCreate;
  late final DgRadixDestroyDart _radixDestroy;
  late final DgRadixInsertDart _radixInsert;
  late final DgRadixMatchesDart _radixMatches;
  late final DgRadixSizeDart _radixSize;

  DegoonEngine({String? customPath}) {
    _lib = _loadLibrary(customPath);

    _trackerCreate = _lib.lookupFunction<DgTrackerCreateC, DgTrackerCreateDart>('dg_tracker_create');
    _trackerDestroy = _lib.lookupFunction<DgTrackerDestroyC, DgTrackerDestroyDart>('dg_tracker_destroy');
    _trackerReset = _lib.lookupFunction<DgTrackerResetC, DgTrackerResetDart>('dg_tracker_reset');
    _trackerActiveCount = _lib.lookupFunction<DgTrackerActiveCountC, DgTrackerActiveCountDart>('dg_tracker_active_count');

    _bloomCreate = _lib.lookupFunction<DgBloomCreateC, DgBloomCreateDart>('dg_bloom_create');
    _bloomDestroy = _lib.lookupFunction<DgBloomDestroyC, DgBloomDestroyDart>('dg_bloom_destroy');
    _bloomAdd = _lib.lookupFunction<DgBloomAddC, DgBloomAddDart>('dg_bloom_add');
    _bloomContains = _lib.lookupFunction<DgBloomContainsC, DgBloomContainsDart>('dg_bloom_contains');

    _radixCreate = _lib.lookupFunction<DgRadixCreateC, DgRadixCreateDart>('dg_radix_create');
    _radixDestroy = _lib.lookupFunction<DgRadixDestroyC, DgRadixDestroyDart>('dg_radix_destroy');
    _radixInsert = _lib.lookupFunction<DgRadixInsertC, DgRadixInsertDart>('dg_radix_insert');
    _radixMatches = _lib.lookupFunction<DgRadixMatchesC, DgRadixMatchesDart>('dg_radix_matches');
    _radixSize = _lib.lookupFunction<DgRadixSizeC, DgRadixSizeDart>('dg_radix_size');
  }

  static DynamicLibrary _loadLibrary(String? customPath) {
    if (customPath != null && File(customPath).existsSync()) {
      return DynamicLibrary.open(customPath);
    }
    if (Platform.isLinux) {
      return DynamicLibrary.open('libengine_shared.so');
    } else if (Platform.isWindows) {
      return DynamicLibrary.open('engine_shared.dll');
    } else if (Platform.isMacOS) {
      return DynamicLibrary.open('libengine_shared.dylib');
    } else if (Platform.isAndroid) {
      return DynamicLibrary.open('libengine_shared.so');
    }
    throw UnsupportedError('Unsupported platform: ${Platform.operatingSystem}');
  }

  Pointer<DgTracker> createTracker({int windowMs = 300, double padding = 0.15, double mergeIou = 0.25}) {
    return _trackerCreate(windowMs, padding, mergeIou);
  }

  void destroyTracker(Pointer<DgTracker> tracker) => _trackerDestroy(tracker);
  void resetTracker(Pointer<DgTracker> tracker) => _trackerReset(tracker);
  int activeTrackerCount(Pointer<DgTracker> tracker) => _trackerActiveCount(tracker);

  Pointer<DgBloom> createBloomFilter({int capacity = 50000, double fpRate = 0.001}) {
    return _bloomCreate(capacity, fpRate);
  }

  void destroyBloomFilter(Pointer<DgBloom> bf) => _bloomDestroy(bf);

  void bloomAdd(Pointer<DgBloom> bf, String key) {
    final nativeKey = key.toNativeUtf8();
    try {
      _bloomAdd(bf, nativeKey);
    } finally {
      calloc.free(nativeKey);
    }
  }

  bool bloomContains(Pointer<DgBloom> bf, String key) {
    final nativeKey = key.toNativeUtf8();
    try {
      return _bloomContains(bf, nativeKey);
    } finally {
      calloc.free(nativeKey);
    }
  }

  Pointer<DgRadix> createRadixTree() => _radixCreate();
  void destroyRadixTree(Pointer<DgRadix> tree) => _radixDestroy(tree);

  void radixInsert(Pointer<DgRadix> tree, String domain) {
    final nativeDomain = domain.toNativeUtf8();
    try {
      _radixInsert(tree, nativeDomain);
    } finally {
      calloc.free(nativeDomain);
    }
  }

  bool radixMatches(Pointer<DgRadix> tree, String domain) {
    final nativeDomain = domain.toNativeUtf8();
    try {
      return _radixMatches(tree, nativeDomain);
    } finally {
      calloc.free(nativeDomain);
    }
  }

  int radixSize(Pointer<DgRadix> tree) => _radixSize(tree);
}
