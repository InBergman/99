#include <windows.h>
#include <xinput.h>
#include <stdint.h>
#include <dsound.h>

#define internal static
#define global_variable static
#define local_persist static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void*      Memory;
	int        Height;
	int        Width;
	int        Pitch;
	int        BytesPerPixel;
};

struct win32_window_dimension
{
	int       Width;
	int       Height;
};

global_variable bool GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackBuffer = {};
global_variable BITMAPINFO BitmapInfo;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuideDevice, LPDIRECTSOUND *ppDS,LPUNKNOWN pUnkouter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;


#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;

#define XInputGetState XInputGetState_
#define XInputSetState XInputSetState_




internal void 
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibrary("xinput1_4.dll");
	if(!XInputLibrary)
	{
		XInputLibrary = LoadLibrary("xinput1_3.dll");
	}
	
	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state *) GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *) GetProcAddress(XInputLibrary, "XInputSetState");
	}
}


internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	OutputDebugStringA("Entering Init DSOUND func");
	HMODULE DSoundLibrary = LoadLibrary("dsound.dll");
	if(DSoundLibrary)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *) GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0,&DirectSound ,0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample ) / 8;
			WaveFormat.cbSize = 0;
			
			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
				
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
				{
					
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{
						OutputDebugStringA("First Buffer Created");
					}
					else
					{
						// TODO(mehdi): Logging
					}
				}
				else
				{
					// TODO(mehdi): Logging 
				}
			}
			else
			{
				// TODO(mehdi): Logging
			}
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;
			LPDIRECTSOUNDBUFFER PrimaryBuffer;
			if(SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0)))
			{
				OutputDebugStringA("Second Buffer Created");
			}
			else
			{
				// TODO(mehdi): Logging
				
			}
		}
		
	}
	else
	{
		// TODO(mehdi): Logging
	}
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;
	
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	
	return(Result);
}


internal void 
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	
	
	Buffer->Width         = Width;
	Buffer->Height        = Height;
	Buffer->BytesPerPixel = 4;
	Buffer->Info.bmiHeader.biSize        = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth       = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight      = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes      = 1;
	Buffer->Info.bmiHeader.biBitCount    = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
	
	
	// NOTE(mehdi): Casey did explain that, to caclulate a square area 
	// you need to multiply the height * Width, same way it apply 
	// to calculate the memory needed to our bitmap square.
	
	
	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	
	Buffer->Memory       = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch        = Width * Buffer->BytesPerPixel;
}

internal void
RenderGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset)
{
	uint8 *Row = (uint8 *)Buffer.Memory;
	
	for(int Y = 0; Y < Buffer.Height; ++Y)
	{
		uint32 *Pixel = (uint32*)Row;
		for(int X = 0; X < Buffer.Width; ++X)
		{
			uint8 Blue  = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);
			
			*Pixel++ = ((Green << 8) | Blue);
		}
		Row += Buffer.Pitch;
		
	}
}


internal void
Win32DisplayBufferInWindow(HDC DeviceContext,
						   int WindowWidth, 
						   int WindowHeight,
						   win32_offscreen_buffer Buffer,
						   int X, int Y, int Width ,int Height)
{
	
	StretchDIBits(DeviceContext,
				  /*
				  X, Y, Width, Height,
				  X, Y, Width, Height,
				  */
				  0, 0, WindowWidth, WindowHeight,
				  0, 0, Buffer.Width, Buffer.Height,
				  Buffer.Memory, 
				  &Buffer.Info,
				  DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK 
Win32MainWindowCallBack(HWND Window,
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
			GlobalRunning = false;
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		
		case WM_CLOSE:
		{
			GlobalRunning = false;
			OutputDebugStringA("WM_CLOSE\n");
		} break;
		
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			
			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left ;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
			
			win32_window_dimension Dimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferInWindow(DeviceContext, 
									   Dimension.Width, 
									   Dimension.Height,
									   GlobalBackBuffer,
									   X, Y, Width, Height);
			EndPaint(Window, &Paint);
			OutputDebugStringA("WM_PAINT22\n");
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
	Win32LoadXInput();
	WNDCLASS WindowClass = {};
	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);
	WindowClass.style = CS_HREDRAW|CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallBack;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "Dude";
	
	if(RegisterClassA(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(0,
											WindowClass.lpszClassName,
											"99",
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
			int XOffset = 1;
			int YOffset = 1;
			
			GlobalRunning = true;
			Win32InitDSound(WindowHandle, 48000, 48000*sizeof(int16)*2);
			
			while(GlobalRunning)
			{
				// @NOTE(mehdi): Message has been declare in the scope if 
				// this while to avoid someone else modifing it.  
				
				MSG Message;
				while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
				{
					if(Message.message == WM_QUIT)
					{
						GlobalRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}
				
				for (DWORD Index_Controlleur = 0;
					 Index_Controlleur < XUSER_MAX_COUNT;
					 ++Index_Controlleur)
				{
					XINPUT_STATE State_Controlleur;
					
					if (XInputGetState(Index_Controlleur, &State_Controlleur) == ERROR_SUCCESS)
					{
						XINPUT_GAMEPAD *GamePad = &State_Controlleur.Gamepad;
						
						bool Up_Button = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down_Button = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left_Button = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right_Button = (GamePad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start_Button = (GamePad->wButtons & XINPUT_GAMEPAD_START);
						bool Back_Button = (GamePad->wButtons & XINPUT_GAMEPAD_BACK);
						bool Left_Shoulder = (GamePad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool Right_Shoulder = (GamePad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool A_Button = (GamePad->wButtons & XINPUT_GAMEPAD_A);
						bool B_Button = (GamePad->wButtons & XINPUT_GAMEPAD_B);
						bool X_Button = (GamePad->wButtons & XINPUT_GAMEPAD_X);
						bool Y_Button = (GamePad->wButtons & XINPUT_GAMEPAD_Y);
						
						if(A_Button)
						{
							YOffset += 2;
						}
						if(B_Button)
						{
							XOffset += 2;
						}
					}
				}
				
				
				RenderGradient(GlobalBackBuffer, XOffset, YOffset);
				HDC DeviceContext = GetDC(WindowHandle);
				
				win32_window_dimension Dimension = Win32GetWindowDimension(WindowHandle);
				Win32DisplayBufferInWindow(DeviceContext,
										   Dimension.Width, 
										   Dimension.Height,
										   GlobalBackBuffer,
										   0, 0,
										   Dimension.Width, 
										   Dimension.Height);
				ReleaseDC(WindowHandle, DeviceContext);
			}
		}
		else
		{
			//TODO(mehdi): Logging
		}
	}
	else
	{
		//TODO(mehdi): Logging
	}
	
	return (0);
}