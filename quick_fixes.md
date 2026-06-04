# Quick Fixes & Troubleshooting Log

## UI Text Rendering Glitches (Garbled or "Japanese-looking" characters)
- **Symptom:** Text rendered in the UI bounding boxes appears as tiny blips, garbled pixels, or small characters that look foreign.
- **Cause:** There is a mismatch between the texture atlas resolution used when baking the font (`stbtt_BakeFontBitmap`) and the resolution queried when generating the text quads (`stbtt_GetBakedQuad`). If the font was baked at 2048x2048, but queried at 1024x1024, the UV coordinates will be sampled incorrectly.
- **Fix:** Locate the `stbtt_GetBakedQuad` calls (e.g., in `VulkanRenderer.cpp`) and ensure the width and height parameters match the exact dimensions of the font image buffer (e.g., `2048, 2048`).
