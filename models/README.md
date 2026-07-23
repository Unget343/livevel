# MediaPipe Models

This directory stores the MediaPipe Hand Landmarker model.

Run this once from the repository root:

```powershell
.\models\download_model.ps1
```

The application intentionally does not fall back to contour-based pseudo-landmarks: proper finger tracking requires all 21 MediaPipe landmarks.
