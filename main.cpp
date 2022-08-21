
#include "windows.h"
#include <gl/gl.h>

#define function static
#define Assert(e) {if(!e) {*((void**)(0)) = 0;}}

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


LRESULT CALLBACK WindowProc(HWND   hwnd,
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

HWND create_main_window(HINSTANCE hInst, float WindowWidth, float WindowHeight)
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
        glClearColor(1.0f, 1.0f, 0.3f, 0.0f);
        
        
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
                    glViewport(0, 0, (GLsizei)WindowWidth, (GLsizei)WindowHeight);
                }
            }
            
            glClear(GL_COLOR_BUFFER_BIT);
            
            glBegin(GL_POLYGON);
            glColor3f(1, 0, 0); glVertex3f(-0.6f, -0.75f, 0.5f);
            glColor3f(0, 1, 0); glVertex3f(0.6f, -0.75f, 0);
            glColor3f(0, 0, 1); glVertex3f(0, 0.75f, 0);
            glEnd();
            
            glFlush();
            
            SwapBuffers(hdc);
            
            TranslateMessage(&msg); 
            DispatchMessage(&msg); 
        } 
        
        ReleaseDC(hwnd, hdc);
    }
    
    return 0; 
}