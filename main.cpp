#define WIN32_LEAN_AND_MEAN 
#include "windows.h"
#include <gl/gl.h>
#include <stdio.h>

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int
#define u64 unsigned long long

#define function static
#define Assert(e) {if(!(e)) {*((void**)(0)) = 0;}}
#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

#include "image_processing.cpp"
#include "dx_capture_screen.cpp"

#ifdef UNICODE
#define _T(str) L##str
#else
#define _T(str) str
#endif


#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8                          0x8C41
#define GL_SRGB8_ALPHA8                   0x8C43

struct PerfCounter {
    LARGE_INTEGER frequency, start_time, end_time;
    
    void begin()
    {
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start_time);
    }
    
    // return elapsed time in seconds
    double end()
    {
        QueryPerformanceCounter(&end_time);
        
        return (double)(end_time.QuadPart - start_time.QuadPart) / frequency.QuadPart;
    }
};

static u32 opengl_internal_image_format = GL_RGBA8;

struct opengl_extension_opt {
    char *extension_strings;
    bool GL_ARB_framebuffer_sRGB;
    bool WGL_EXT_swap_control;
};

function void get_opengl_extension_options(opengl_extension_opt *options)
{
    char *gl_extensions = (char*)glGetString(GL_EXTENSIONS);
    
    
    FILE *f = fopen("opengl_extensions.txt", "wb");
    if (f) {
        fwrite(gl_extensions, 1, strlen(gl_extensions), f);
        fclose(f);
    }
    
    char *start = gl_extensions;
    char *end = start;
    while (true)
    {
        if (*end == ' ' || *end == 0) {
            if (end > start)
            {
                u64 length = end - start - 1;
                if (strncmp(start, "GL_ARB_framebuffer_sRGB", length) == 0)
                {
                    options->GL_ARB_framebuffer_sRGB = true;
                } 
                else if (strncmp(start, "WGL_EXT_swap_control", length) == 0)
                {
                    options->WGL_EXT_swap_control = true;
                }
            }
            
            if (*end == 0)
                break;
            
            start = end + 1;
            end = start;
        }
        
        end++;
    }
    
    options->extension_strings = gl_extensions;
    
}

function void
win32_opengl_init(HWND Window, HDC WindowDC)
{
    PIXELFORMATDESCRIPTOR DesiredPixelFormat = {};
    DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
    DesiredPixelFormat.nVersion = 1;
    DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
    
    DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
    DesiredPixelFormat.cColorBits = 32;
    DesiredPixelFormat.cAlphaBits = 8;
    DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
    
    int SuggestedPixelFormatIndex = ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                        sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
    
    HGLRC OpenGLRC = wglCreateContext(WindowDC);
    if(wglMakeCurrent(WindowDC, OpenGLRC))        
    {
        
        opengl_extension_opt gl_options = {};
        get_opengl_extension_options(&gl_options);
        
        //glGenTextures(1, &TextureHandle);
#if 1
        // vsync
        if (gl_options.WGL_EXT_swap_control)
        {
            typedef BOOL WINAPI wglSwapIntervalEXT_func(int interval);
            wglSwapIntervalEXT_func* wglSwapIntervalEXT = (wglSwapIntervalEXT_func*)wglGetProcAddress("wglSwapIntervalEXT");
            if (wglSwapIntervalEXT)
                wglSwapIntervalEXT(1); 
        }
        
        // srgb
        if (gl_options.GL_ARB_framebuffer_sRGB)
        {
            opengl_internal_image_format = GL_SRGB8_ALPHA8;
            //opengl_internal_image_format = GL_SRGB8;
            glEnable(GL_FRAMEBUFFER_SRGB);
        }
        
#endif
        
    }
    else
    {
        Assert(0);
    }
}


function LRESULT CALLBACK WindowProc(HWND   hwnd,
                                     UINT   msg,
                                     WPARAM wParam,
                                     LPARAM lParam)
{
    switch(msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        case WM_CHAR: {
            
            if (wParam > 0)
            {
                u16 hi = HIWORD(wParam);
                u16 lo = LOWORD(wParam);
                
                u16 wc = lo;
                if (hi > 0) // unicode
                {
                    wc = (hi << 8) | (lo & 0xFF);
                }
            }
            
            break;
            
        }
        case WM_SIZE:
        {
            // TODO: handle opengl resize
            u32 WindowWidth = LOWORD(lParam);
            u32 WindowHeight = HIWORD(lParam);
            glViewport(0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight);
            break;
        }
        default: {
            return DefWindowProc(hwnd, msg, wParam, lParam);;
        }
    }
    
    return 0;
}

function HWND create_main_window(HINSTANCE hInst, float WindowWidth, float WindowHeight)
{
    TCHAR *class_name = _T("opengl_window");
    
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = class_name;
    //wc.style        = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.hIcon        = LoadIcon(NULL, IDI_INFORMATION);
    wc.hCursor      = LoadCursor(NULL, IDC_ARROW);
    RegisterClass(&wc);
    
    HWND hwnd_parent = 0;
    HWND hwnd = CreateWindow(
                             class_name,
                             _T("opengl_tpl_class"),
                             WS_OVERLAPPEDWINDOW,
                             CW_USEDEFAULT,
                             CW_USEDEFAULT,
                             (u32)WindowWidth,
                             (u32)WindowHeight,
                             hwnd_parent,
                             0,
                             hInst,
                             0);
    
    if (hwnd == NULL)
    {
        DWORD err = GetLastError();
        Assert(0);
    }
    
    return hwnd;
}

function void opengl_draw_triangle() {
    glBegin(GL_TRIANGLES);
    float p = 1.0f;
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-p, -p);
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(p, -p);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(p, p);
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-p, -p);
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(p, p);
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-p, p);
    
    glEnd();
    
#if 0
    glBegin(GL_POLYGON);
    glColor3f(1, 0, 0); glVertex3f(-0.6f, -0.75f, 0.5f);
    glColor3f(0, 1, 0); glVertex3f(0.6f, -0.75f, 0);
    glColor3f(0, 0, 1); glVertex3f(0, 0.75f, 0);
    glEnd();
#endif
}

int CALLBACK  WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    float WindowWidth = 1080;
    float WindowHeight = 780;
    HWND hwnd = create_main_window(hInst, WindowWidth, WindowHeight);
    
    if (hwnd)
    {
        HDC hdc = GetDC(hwnd);
        
        win32_opengl_init(hwnd, hdc);
        
        CaptureContext context = {};
        u32 width = 256;
        u32 height = 256;
        u32 image_buffer_size = 2048 * 2048 * 4;
        u8* image_buffer = (u8*)malloc(image_buffer_size);
        u32 channels = 4; // rgba
        
        enum TestImageType {
            TEST_IMAGE_COLOR_GEN,
            TEST_IMAGE_FILE,
            TEST_IMAGE_CAPTURE_BLT,
            TEST_IMAGE_CAPTURE_DX,
            
            TEST_IMAGE_TYPE_COUNT, // count value
        };
        
        TestImageType test_image_type = TEST_IMAGE_COLOR_GEN;
        bool test_init = false;
        
        ShowWindow(hwnd, SW_SHOWNORMAL);
        
        UpdateWindow(hwnd); 
        
        RECT rect = {};
        GetClientRect(hwnd, &rect);
        glViewport(rect.left, rect.top, (GLsizei)rect.right - rect.left, (GLsizei)rect.bottom - rect.top);
        
        GLuint TextureHandle = 0;
        glGenTextures(1, &TextureHandle);
        int opengl_image_buffer_format = GL_BGRA_EXT;
        
        u64 frame_number = 0;
        u64 fps_frame_count = 0;
        double fps_spent_time = 0;
        
        // Start the message loop. 
        PerfCounter perf = {};
        MSG msg = {};
        while(WM_QUIT != msg.message)
        { 
            perf.begin();
            
            ++frame_number;
            
            if(PeekMessage( &msg, NULL, 0, 0, PM_REMOVE))
            {
                if (msg.message == WM_KEYDOWN)
                {
                    if (msg.wParam == VK_SPACE)
                    {
                        test_image_type = (TestImageType)((test_image_type + 1) % TEST_IMAGE_TYPE_COUNT);
                        test_init = false;
                    }
                }
                TranslateMessage(&msg); 
                DispatchMessage(&msg); 
            }
            
            if (!test_init)
            {
                test_init = true;
                
                if (test_image_type == TEST_IMAGE_COLOR_GEN)
                {
                    width = height = 256;
                    u8 *p = image_buffer;
                    u32 x, y;
                    for (y = 0; y < width; y++)
                        for (x = 0; x < height; x++) {
                        *p++ = (unsigned char)x;                /* R */
                        *p++ = (unsigned char)y;                /* G */
                        *p++ = 128;                             /* B */
                        *p++ = (unsigned char)((x + y) / 2);    /* A */
                    }
                }
                else if (test_image_type == TEST_IMAGE_FILE)
                {
                    int png_channels = 0;
                    stbi_set_flip_vertically_on_load(true); 
                    image_buffer = stbi_load("desktop.png", (int*)&width, (int*)&height, &png_channels, channels);
                    
                    opengl_image_buffer_format = GL_RGBA;
                }
                if (test_image_type == TEST_IMAGE_CAPTURE_DX)
                {
                    if (!context.factory)
                        dx_init(&context);
                }
            }
            
            
            if (test_image_type == TEST_IMAGE_CAPTURE_DX)
            {
                dx_capture(&context, image_buffer, image_buffer_size, &width, &height);
                stbi__vertical_flip(image_buffer, width, height, 4);
            } 
            else if (test_image_type == TEST_IMAGE_CAPTURE_BLT)
            {
                blt_capture_screen(image_buffer, image_buffer_size, &width, &height);
            }
            
            glBindTexture(GL_TEXTURE_2D, TextureHandle);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         opengl_internal_image_format,
                         width,
                         height,
                         0,
                         opengl_image_buffer_format,
                         GL_UNSIGNED_BYTE,
                         image_buffer);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR  /*GL_NEAREST*/ );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR /*GL_NEAREST*/ );
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
            //glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
            
            glTexEnvi(GL_TEXTURE_ENV,
                      GL_TEXTURE_ENV_MODE,
                      GL_MODULATE);
            
            glEnable(GL_TEXTURE_2D);
            
            //glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            //glClear(GL_COLOR_BUFFER_BIT);
            
            // Any point will transform by multiply  vertext * modelview * projection
            // we make it look like multiply by 1
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            
            opengl_draw_triangle();
            
            
            SwapBuffers(hdc);
            
#if 1
            double elapsed = perf.end();
            
            fps_frame_count++;
            fps_spent_time += elapsed;
            if (fps_frame_count > 10) {
                
                TCHAR window_title[256] = {};
                sprintf_s(window_title, _T("FrameTime: %f s fps: %d"), elapsed, (int)(fps_frame_count / fps_spent_time));
                SetWindowText(hwnd, window_title);
                
                fps_frame_count = 0;
                fps_spent_time = 0.0f;
            }
#endif 
            
            
        } 
        
        dx_destroy(&context);
        
        ReleaseDC(hwnd, hdc);
    }
    
    return 0; 
}