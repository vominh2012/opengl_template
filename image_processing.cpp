#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

typedef struct {
    unsigned char r, g, b, a;
} RGBA;

function bool blt_capture_screen(u8* image_data, u32 size, u32* width, u32* height)
{
    u32 w = GetSystemMetrics(SM_CXVIRTUALSCREEN);  
    u32 h = GetSystemMetrics(SM_CYVIRTUALSCREEN);  
    
    Assert(size >= w * h * 4);
    
    HDC hdc = GetDC(NULL);
    
    HDC hDest = CreateCompatibleDC(hdc); // create a dc to use for capture 
    
    HBITMAP hbCapture = CreateCompatibleBitmap(hdc, w, h);  
    SelectObject(hDest, hbCapture); 
    
    // the following line effectively copies the screen into the capture bitamp 
    BitBlt(hDest, 0,0, w, h, hdc, 0, 0, SRCCOPY);  
    
    BITMAPINFOHEADER bmpInfoHeader = { sizeof(BITMAPINFOHEADER), (long)w, (long)h, 1, 32 };
    GetDIBits(hdc, hbCapture, 0, h, image_data, (BITMAPINFO*)&bmpInfoHeader, DIB_RGB_COLORS);
    
    // clean up - release unused resources! 
    ReleaseDC(NULL, hdc);  
    DeleteDC(hDest);
    DeleteObject(hbCapture);
    
    *width = w;
    *height = h;
    return true;
}

RGBA interpolate_rgba(RGBA color1, RGBA color2, float t) {
    // Clamp interpolation factor between 0 and 1
    t = fmaxf(0.0f, fminf(1.0f, t));
    
    // Calculate interpolated values for each channel
    RGBA result;
    result.r = (unsigned char)(color1.r + t * (color2.r - color1.r));
    result.g = (unsigned char)(color1.g + t * (color2.g - color1.g));
    result.b = (unsigned char)(color1.b + t * (color2.b - color1.b));
    result.a = (unsigned char)(color1.a + t * (color2.a - color1.a));
    
    return result;
}

void generate_interpolated_image(int width, int height, RGBA start_color, RGBA end_color, RGBA* image_data) {
    // Calculate step size for interpolation factor
    float step_t = 1.0f / (width - 1);
    
    // Iterate through each pixel and perform interpolation
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Calculate interpolation factor based on position
            float t = x * step_t;
            image_data[y * width + x] = interpolate_rgba(start_color, end_color, t);
        }
    }
}
