#include "vastpch.h"
#include "Core/Core.h"

#include "Rendering/Imgui.h"

#ifdef VAST_PLATFORM_WINDOWS

// Note: Reduces the size of windows.h by excluding less commonly used parts of the API
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h> // GET_X_LPARAM(), GET_Y_LPARAM()

// Note: Keypad Enter is not natively handled by VK_ (it's encoded in lParam & KF_EXTENDED), so for
// ImGui we map it to VK_RETURN + an arbitrary value.
#define VK_RETURN_EXT (VK_RETURN + KF_EXTENDED)

namespace vast
{

	constexpr LPTSTR TranslateToWin32Cursor(ImGuiMouseCursor cursor)
	{
		switch (cursor)
		{
		case ImGuiMouseCursor_None:			return nullptr;
		case ImGuiMouseCursor_Arrow:        return IDC_ARROW;
		case ImGuiMouseCursor_TextInput:    return IDC_IBEAM;
		case ImGuiMouseCursor_ResizeAll:    return IDC_SIZEALL;
		case ImGuiMouseCursor_ResizeEW:     return IDC_SIZEWE;
		case ImGuiMouseCursor_ResizeNS:     return IDC_SIZENS;
		case ImGuiMouseCursor_ResizeNESW:   return IDC_SIZENESW;
		case ImGuiMouseCursor_ResizeNWSE:   return IDC_SIZENWSE;
		case ImGuiMouseCursor_Hand:         return IDC_HAND;
		case ImGuiMouseCursor_NotAllowed:   return IDC_NO;
		default:							return IDC_ARROW;
		}
	}


	constexpr int32 TranslateToImguiMouseButton(UINT msg, WPARAM wParam)
	{
		switch (msg)
		{
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
			return ImGuiMouseButton_Left;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			return ImGuiMouseButton_Right;
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			return ImGuiMouseButton_Middle;
		case WM_XBUTTONDBLCLK:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			return (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4;
		default:
			return 0;
		}
	}

	constexpr ImGuiKey TranslateToImguiKey(WPARAM wParam)
	{
		switch (wParam)
		{
		case VK_TAB:	   	return ImGuiKey_Tab;
		case VK_LEFT:	   	return ImGuiKey_LeftArrow;
		case VK_RIGHT:	   	return ImGuiKey_RightArrow;
		case VK_UP:		   	return ImGuiKey_UpArrow;
		case VK_DOWN:	   	return ImGuiKey_DownArrow;
		case VK_PRIOR:	   	return ImGuiKey_PageUp;
		case VK_NEXT:	   	return ImGuiKey_PageDown;
		case VK_HOME:	   	return ImGuiKey_Home;
		case VK_END:	   	return ImGuiKey_End;
		case VK_INSERT:	   	return ImGuiKey_Insert;
		case VK_DELETE:	   	return ImGuiKey_Delete;
		case VK_BACK:	   	return ImGuiKey_Backspace;
		case VK_SPACE:	   	return ImGuiKey_Space;
		case VK_RETURN:	   	return ImGuiKey_Enter;
		case VK_ESCAPE:	   	return ImGuiKey_Escape;
		case VK_OEM_7:	   	return ImGuiKey_Apostrophe;
		case VK_OEM_COMMA: 	return ImGuiKey_Comma;
		case VK_OEM_MINUS: 	return ImGuiKey_Minus;
		case VK_OEM_PERIOD:	return ImGuiKey_Period;
		case VK_OEM_2:	   	return ImGuiKey_Slash;
		case VK_OEM_1:	   	return ImGuiKey_Semicolon;
		case VK_OEM_PLUS:  	return ImGuiKey_Equal;
		case VK_OEM_4:	   	return ImGuiKey_LeftBracket;
		case VK_OEM_5:	   	return ImGuiKey_Backslash;
		case VK_OEM_6:	   	return ImGuiKey_RightBracket;
		case VK_OEM_3:	   	return ImGuiKey_GraveAccent;
		case VK_CAPITAL:   	return ImGuiKey_CapsLock;
		case VK_SCROLL:	   	return ImGuiKey_ScrollLock;
		case VK_NUMLOCK:   	return ImGuiKey_NumLock;
		case VK_SNAPSHOT:  	return ImGuiKey_PrintScreen;
		case VK_PAUSE:	   	return ImGuiKey_Pause;
		case VK_NUMPAD0:   	return ImGuiKey_Keypad0;
		case VK_NUMPAD1:   	return ImGuiKey_Keypad1;
		case VK_NUMPAD2:   	return ImGuiKey_Keypad2;
		case VK_NUMPAD3:   	return ImGuiKey_Keypad3;
		case VK_NUMPAD4:   	return ImGuiKey_Keypad4;
		case VK_NUMPAD5:   	return ImGuiKey_Keypad5;
		case VK_NUMPAD6:   	return ImGuiKey_Keypad6;
		case VK_NUMPAD7:   	return ImGuiKey_Keypad7;
		case VK_NUMPAD8:   	return ImGuiKey_Keypad8;
		case VK_NUMPAD9:   	return ImGuiKey_Keypad9;
		case VK_DECIMAL:   	return ImGuiKey_KeypadDecimal;
		case VK_DIVIDE:	   	return ImGuiKey_KeypadDivide;
		case VK_MULTIPLY:  	return ImGuiKey_KeypadMultiply;
		case VK_SUBTRACT:  	return ImGuiKey_KeypadSubtract;
		case VK_ADD:	   	return ImGuiKey_KeypadAdd;
		case VK_RETURN_EXT:	return ImGuiKey_KeypadEnter;
		case VK_LSHIFT:		return ImGuiKey_LeftShift;
		case VK_LCONTROL:	return ImGuiKey_LeftCtrl;
		case VK_LMENU:		return ImGuiKey_LeftAlt;
		case VK_LWIN:		return ImGuiKey_LeftSuper;
		case VK_RSHIFT:		return ImGuiKey_RightShift;
		case VK_RCONTROL:	return ImGuiKey_RightCtrl;
		case VK_RMENU:		return ImGuiKey_RightAlt;
		case VK_RWIN:		return ImGuiKey_RightSuper;
		case VK_APPS:		return ImGuiKey_Menu;
		case '0':			return ImGuiKey_0;
		case '1':			return ImGuiKey_1;
		case '2':			return ImGuiKey_2;
		case '3':			return ImGuiKey_3;
		case '4':			return ImGuiKey_4;
		case '5':			return ImGuiKey_5;
		case '6':			return ImGuiKey_6;
		case '7':			return ImGuiKey_7;
		case '8':			return ImGuiKey_8;
		case '9':			return ImGuiKey_9;
		case 'A':			return ImGuiKey_A;
		case 'B':			return ImGuiKey_B;
		case 'C':			return ImGuiKey_C;
		case 'D':			return ImGuiKey_D;
		case 'E':			return ImGuiKey_E;
		case 'F':			return ImGuiKey_F;
		case 'G':			return ImGuiKey_G;
		case 'H':			return ImGuiKey_H;
		case 'I':			return ImGuiKey_I;
		case 'J':			return ImGuiKey_J;
		case 'K':			return ImGuiKey_K;
		case 'L':			return ImGuiKey_L;
		case 'M':			return ImGuiKey_M;
		case 'N':			return ImGuiKey_N;
		case 'O':			return ImGuiKey_O;
		case 'P':			return ImGuiKey_P;
		case 'Q':			return ImGuiKey_Q;
		case 'R':			return ImGuiKey_R;
		case 'S':			return ImGuiKey_S;
		case 'T':			return ImGuiKey_T;
		case 'U':			return ImGuiKey_U;
		case 'V':			return ImGuiKey_V;
		case 'W':			return ImGuiKey_W;
		case 'X':			return ImGuiKey_X;
		case 'Y':			return ImGuiKey_Y;
		case 'Z':			return ImGuiKey_Z;
		case VK_F1:			return ImGuiKey_F1;
		case VK_F2:			return ImGuiKey_F2;
		case VK_F3:			return ImGuiKey_F3;
		case VK_F4:			return ImGuiKey_F4;
		case VK_F5:			return ImGuiKey_F5;
		case VK_F6:			return ImGuiKey_F6;
		case VK_F7:			return ImGuiKey_F7;
		case VK_F8:			return ImGuiKey_F8;
		case VK_F9:			return ImGuiKey_F9;
		case VK_F10:		return ImGuiKey_F10;
		case VK_F11:		return ImGuiKey_F11;
		case VK_F12:		return ImGuiKey_F12;
		default:			return ImGuiKey_None;
		}
	}

	static bool IsVirtualKeyStateDown(int vk)
	{
		return (::GetKeyState(vk) & 0x8000) != 0;
	}

	LRESULT CALLBACK Imgui_WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext() == nullptr)
			return 0;

		ImGuiIO& io = ImGui::GetIO();

		static bool bIsMouseTracking = false;
		static int32 mouseButtonsDownMask = 0;

		switch (msg)
		{
		case WM_MOUSEMOVE:
			io.AddMousePosEvent((float)GET_X_LPARAM(lParam), (float)GET_Y_LPARAM(lParam));
			return 0;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_XBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDBLCLK:
		{
			int button = TranslateToImguiMouseButton(msg, wParam);
			io.AddMouseButtonEvent(button, true);
			// Allow window to keep receiving mouse input while any mouse button is down even if the mouse goes outside our window.
			if (mouseButtonsDownMask == 0 && ::GetCapture() == nullptr)
			{
				::SetCapture(hWnd);
			}
			mouseButtonsDownMask |= (1 << button);
			return 0;
		}
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
		case WM_XBUTTONUP:
		{
			int button = TranslateToImguiMouseButton(msg, wParam);
			io.AddMouseButtonEvent(button, false);
			// Release mouse input capture if all mouse buttons are no longer pressed.
			mouseButtonsDownMask &= ~(1 << button);
			if (mouseButtonsDownMask == 0 && ::GetCapture() == hWnd)
			{
				::ReleaseCapture();
			}
			return 0;
		}
		case WM_MOUSEWHEEL:
			io.AddMouseWheelEvent(0.0f, (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA);
			return 0;
		case WM_MOUSEHWHEEL:
			io.AddMouseWheelEvent(-(float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA, 0.0f);
			return 0;
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (wParam < 256)
			{
				const bool bIsKeyDown = (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN);

				// Submit modifiers
				io.AddKeyEvent(ImGuiMod_Ctrl,	IsVirtualKeyStateDown(VK_CONTROL));
				io.AddKeyEvent(ImGuiMod_Shift,	IsVirtualKeyStateDown(VK_SHIFT));
				io.AddKeyEvent(ImGuiMod_Alt,	IsVirtualKeyStateDown(VK_MENU));
				io.AddKeyEvent(ImGuiMod_Super,	IsVirtualKeyStateDown(VK_APPS));

				// Obtain virtual key code
				int32 virtualKey = static_cast<int32>(wParam);
				if ((wParam == VK_RETURN) && (HIWORD(lParam) & KF_EXTENDED))
				{
					virtualKey = VK_RETURN_EXT;
				}

				// Submit key event
				const ImGuiKey imguiKey = TranslateToImguiKey(virtualKey);
				if (imguiKey != ImGuiKey_None)
				{
					io.AddKeyEvent(imguiKey, bIsKeyDown);
				}

				// Submit individual left/right modifier events
				if (virtualKey == VK_SHIFT)
				{
					if (IsVirtualKeyStateDown(VK_LSHIFT) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_LeftShift, bIsKeyDown); }
					if (IsVirtualKeyStateDown(VK_RSHIFT) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_RightShift, bIsKeyDown); }
				}
				else if (virtualKey == VK_CONTROL)
				{
					if (IsVirtualKeyStateDown(VK_LCONTROL) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_LeftCtrl, bIsKeyDown); }
					if (IsVirtualKeyStateDown(VK_RCONTROL) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_RightCtrl, bIsKeyDown); }
				}
				else if (virtualKey == VK_MENU)
				{
					if (IsVirtualKeyStateDown(VK_LMENU) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_LeftAlt, bIsKeyDown); }
					if (IsVirtualKeyStateDown(VK_RMENU) == bIsKeyDown) { io.AddKeyEvent(ImGuiKey_RightAlt, bIsKeyDown); }
				}
			}
			return 0;
		case WM_CHAR:
			if (::IsWindowUnicode(hWnd))
			{
				if (wParam > 0 && wParam < 0x10000)
				{
					io.AddInputCharacter(static_cast<uint32>(wParam));
				}
			}
			else
			{
				wchar_t wch = 0;
				::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, (char*)&wParam, 1, &wch, 1);
				io.AddInputCharacter(static_cast<uint32>(wch));
			}
			return 0;
		case WM_SETFOCUS:
		case WM_KILLFOCUS:
			io.AddFocusEvent(msg == WM_SETFOCUS);
			return 0;
		case WM_SETCURSOR:
			if (io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange)
				return 0;

			if (LOWORD(lParam) == HTCLIENT)
			{
				if (ImGui::GetMouseCursor() == ImGuiMouseCursor_None || io.MouseDrawCursor)
				{
					::SetCursor(nullptr);
				}
				else
				{
					::SetCursor(::LoadCursor(nullptr, TranslateToWin32Cursor(ImGui::GetMouseCursor())));
				}
				return 1;
			}
			return 0;
		}
		return 0;
	}
}

#endif // VAST_PLATFORM_WINDOWS
