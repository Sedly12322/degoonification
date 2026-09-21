# 🧠 Degoonification AI Models

This directory stores quantized models for real-time adult content detection:

- **`yolov8n-nsfw.onnx`** (FP16/INT8): Used for Desktop inference (Windows DirectML, Linux ONNX Runtime Vulkan/TensorRT).
- **`yolov8n-nsfw.tflite`** (INT8): Used for Android NNAPI/GPU delegate inference.

## Classes Detected
1. `FEMALE_GENITALIA_COVERED`
2. `FACE_FEMALE`
3. `BUTTOCKS_EXPOSED`
4. `FEMALE_BREAST_EXPOSED`
5. `FEMALE_GENITALIA_EXPOSED`
6. `MALE_BREAST_EXPOSED`
7. `ANUS_EXPOSED`
8. `FEET_EXPOSED`
9. `BELLY_COVERED`
10. `FEET_COVERED`
11. `ARMPITS_COVERED`
12. `ARMPITS_EXPOSED`
13. `FACE_MALE`
14. `BELLY_EXPOSED`
15. `MALE_GENITALIA_EXPOSED`
16. `ANUS_COVERED`
17. `FEMALE_BREAST_COVERED`
18. `BUTTOCKS_COVERED`

*(Classes with EXPOSED genitals, breasts, buttocks, and explicit interactions trigger active blur boxes).*
