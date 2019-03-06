
#include <windows.h>
#pragma comment(lib,"user32.lib")

LRESULT CALLBACK MainWindowCallBack(HWND Window,
                                    UINT Message,
                                    WPARAM wParam,
                                    LPARAM lParam)
{
    LRESULT result = 0;
    
    switch(Message)
    {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;
        
        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
        } break;
        
        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
        } break;
        
        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;
        
        default:
        {
            // OutputDebugStringA("default");
            result = DefWindowProc(Window, Message, wParam, lParam);
        }
        
    }
    return(result);
}

int CALLBACK WinMain(HINSTANCE Instance, 
                     HINSTANCE prevInstance, 
                     LPSTR commandLine,
                     int showCode)
{
    WNDCLASS WindowClass = {} ;
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW ;
    WindowClass.lpfnWndProc = MainWindowCallBack;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "Bakaryu";
    
    if(RegisterClass(&WindowClass))
    {
        HWND WindowHandle = CreateWindowEx(0,
                                           WindowClass.lpszClassName,
                                           "Bakayaru",
                                           WS_OVERLAPPEDWINDOW|WS_VISIBLE,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           0,
                                           0,
                                           Instance,
                                           0);
        if(WindowHandle)
        {
            for(;;)
            {
                MSG Message;
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if(MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else
                {
                    break;
                }
            }
        }
        else
        {
            //TODO(MEHDI): Logging
        }
    }
    else
    {
        //TODO(MEHDI): Logging
    }
    
    return (0);
}