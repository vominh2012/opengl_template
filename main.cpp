
#include "windows.h"
#define GL_GLEXT_PROTOTYPES
#include <gl/gl.h>

#define function static
#define Assert(e) {if(!e) {*((void**)(0)) = 0;}}
#define ArrayCount(arr) (sizeof(arr)/sizeof(arr[0]))

#define u8 unsigned char
#define u16 unsigned short
#define u32 unsigned int

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
        //glGenTextures(1, &TextureHandle);
    }
    else
    {
        Assert(0);
    }
}


function LRESULT CALLBACK WindowProc(HWND   hwnd,
                                     UINT   msg,
                                     WPARAM wParam,
                                     LPARAM lParam
                                     )
{
    
    switch(msg) {
        case WM_DESTROY: {
            PostQuitMessage(0);
            return 0;
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
                
                return 0;
            }
            
            break;
            
        }
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);;
}

function HWND create_main_window(HINSTANCE hInst, float WindowWidth, float WindowHeight)
{
    WCHAR *class_name = L"opengl_window";
    
    WNDCLASS wc = { };
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInst;
    wc.lpszClassName = class_name;
    RegisterClass(&wc);
    
    HWND hwnd_parent = 0;
    HWND hwnd = CreateWindow(
                             class_name,
                             L"opengl_tpl_class",
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

int CALLBACK  WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
    float WindowWidth = 1280;
    float WindowHeight = 720;
    HWND hwnd = create_main_window(hInst, WindowWidth, WindowHeight);
    
    if (hwnd)
    {
        HDC hdc = GetDC(hwnd);
        
        win32_opengl_init(hwnd, hdc);
        
        
        ShowWindow(hwnd, SW_SHOWNORMAL);
        
        UpdateWindow(hwnd); 
        
        glViewport(0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight);
        //glClearColor(1.0f, 1.0f, 0.3f, 0.0f);
        
        GLuint TextureHandle = 0;
        glGenTextures(1, &TextureHandle);
        
        const u32 width = 200;
        const u32 height = 200;
        
        u32 r = 0;
        u32 g = 0;
        u32 b = 128;
        u32 a = 0;
        
        u32 image_buffer[width * height]= {0};
        
        // generate some bitmap
        u32 *p = image_buffer;
        for (int i = 0; i < width; ++i)
            for (int j = 0; j < height; ++j)
        {
            r = i % 256;
            g = j % 256;
            a = ((i + j) / 2) % 256;
            
            u32 bgra =  b | g << 8 | r << 16 | a << 24;
            *p++ = bgra;
        }
        
        // Start the message loop. 
        MSG msg;
        while( (GetMessage( &msg, NULL, 0, 0 )) != 0)
        { 
            switch(msg.message)
            {
                case WM_SIZE:
                {
                    // TODO: handle opengl resize
                    WindowWidth = LOWORD(msg.lParam);
                    WindowHeight = HIWORD(msg.lParam);
                    //glViewport(0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight);
                }
            }
            
            
            glBindTexture(GL_TEXTURE_2D, TextureHandle);
            glTexImage2D(GL_TEXTURE_2D,
                         0,
                         GL_RGBA8,
                         width,
                         height,
                         0,
                         GL_BGRA_EXT,
                         GL_UNSIGNED_BYTE,
                         &image_buffer);
            
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
            
            glTexEnvi(GL_TEXTURE_ENV,
                      GL_TEXTURE_ENV_MODE,
                      GL_MODULATE);
            
            glEnable(GL_TEXTURE_2D);
            
            glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            
            // Any point will transform by multiply  vertext * modelview * projection
            // we make it look like multiply by 1
            glMatrixMode(GL_TEXTURE);
            glLoadIdentity();
            
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            
            
#if 0
            glBegin(GL_POLYGON);
            glColor3f(1, 0, 0); glVertex3f(-0.6f, -0.75f, 0.5f);
            glColor3f(0, 1, 0); glVertex3f(0.6f, -0.75f, 0);
            glColor3f(0, 0, 1); glVertex3f(0, 0.75f, 0);
            glEnd();
#endif
            
            glBegin(GL_TRIANGLES);
            float c = 0.1f;
            float p = 1.0f;
            //glColor3f(c, c * 2, c * 3);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-p, -p);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(p, -p);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(p, p);
            
            
            c += 0.1f;
            //glColor3f(c, c * 2, c * 3);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-p, -p);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(p, p);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-p, p);
            
            glEnd();
            //glFlush();
            
            SwapBuffers(hdc);
            
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
        
        ReleaseDC(hwnd, hdc);
    }
    
    return 0; 
}