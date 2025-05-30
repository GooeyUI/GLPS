#include <glps_common.h>
#include "utils/logger/pico_logger.h"
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601 // Windows 7 or newer
#endif

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
#define MAX_FILES 128
#define MAX_PATH_LENGTH MAX_PATH
#define MAX_MIME_LENGTH 64
#ifndef GET_X_LPARAM
#define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#endif

#ifndef GET_Y_LPARAM
#define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
ssize_t __get_window_id_from_hwnd(glps_WindowManager *wm, HWND hwnd)
{
  if (wm == NULL)
  {
    return -1;
  }
  for (SIZE_T i = 0; i < wm->window_count; ++i)
  {
    if (wm->windows[i]->hwnd == hwnd)
    {
      return i;
    }
  }

  return -1;
}

void __get_special_key_name(UINT wParam, char *char_value, size_t size)
{
  switch (wParam)
  {
  case VK_ESCAPE:
    strncpy(char_value, "Escape", size);
    break;
  case VK_BACK:
    strncpy(char_value, "Backspace", size);
    break;
  case VK_RETURN:
    strncpy(char_value, "Enter", size);
    break;
  case VK_TAB:
    strncpy(char_value, "Tab", size);
    break;
  case VK_DELETE:
    strncpy(char_value, "Delete", size);
    break;
  case VK_INSERT:
    strncpy(char_value, "Insert", size);
    break;
  case VK_HOME:
    strncpy(char_value, "Home", size);
    break;
  case VK_END:
    strncpy(char_value, "End", size);
    break;
  case VK_PRIOR:
    strncpy(char_value, "PageUp", size);
    break;
  case VK_NEXT:
    strncpy(char_value, "PageDown", size);
    break;
  case VK_LEFT:
    strncpy(char_value, "ArrowLeft", size);
    break;
  case VK_RIGHT:
    strncpy(char_value, "ArrowRight", size);
    break;
  case VK_UP:
    strncpy(char_value, "ArrowUp", size);
    break;
  case VK_DOWN:
    strncpy(char_value, "ArrowDown", size);
    break;
  case VK_F1:
    strncpy(char_value, "F1", size);
    break;
  case VK_F2:
    strncpy(char_value, "F2", size);
    break;
  case VK_F3:
    strncpy(char_value, "F3", size);
    break;
  case VK_F4:
    strncpy(char_value, "F4", size);
    break;
  case VK_F5:
    strncpy(char_value, "F5", size);
    break;
  case VK_F6:
    strncpy(char_value, "F6", size);
    break;
  case VK_F7:
    strncpy(char_value, "F7", size);
    break;
  case VK_F8:
    strncpy(char_value, "F8", size);
    break;
  case VK_F9:
    strncpy(char_value, "F9", size);
    break;
  case VK_F10:
    strncpy(char_value, "F10", size);
    break;
  case VK_F11:
    strncpy(char_value, "F11", size);
    break;
  case VK_F12:
    strncpy(char_value, "F12", size);
    break;
  default:
    char_value[0] = '\0';
  }
}

void glps_win32_attach_to_clipboard(glps_WindowManager *wm, char *mime,
                                    char *data)
{

  if (!OpenClipboard(NULL))
  {
    LOG_ERROR("Failed to open clipboard.");
    return;
  }

  if (!EmptyClipboard())
  {
    LOG_ERROR("Failed to empty clipboard.");
    CloseClipboard();
    return;
  }

  HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, strlen(data) + 1);
  if (!hGlobal)
  {
    LOG_ERROR("Failed to allocate global memory.");
    CloseClipboard();
    return;
  }

  char *pGlobal = (char *)GlobalLock(hGlobal);
  if (pGlobal)
  {
    strcpy(pGlobal, data);
    GlobalUnlock(hGlobal);
  }
  else
  {
    LOG_ERROR("Failed to lock global memory.\n");
    GlobalFree(hGlobal);
    CloseClipboard();
    return;
  }

  if (!SetClipboardData(CF_TEXT, hGlobal))
  {
    LOG_ERROR("Failed to set clipboard data.");
    GlobalFree(hGlobal);
  }
  CloseClipboard();
}

void glps_win32_get_from_clipboard(glps_WindowManager *wm, char *data,
                                   size_t data_size)
{
  if (!OpenClipboard(NULL))
  {
    LOG_ERROR("Failed to open clipboard.");
    return;
  }

  HANDLE hData = GetClipboardData(CF_TEXT);
  if (!hData)
  {
    LOG_ERROR("Failed to get clipboard data.");
    CloseClipboard();
    return;
  }

  char *pText = (char *)GlobalLock(hData);
  if (!pText)
  {
    printf("Failed to lock clipboard data.");
    CloseClipboard();
    return;
  }

// strncpy(data, strdup(pText), data_size - 1);
  data[data_size - 1] = '\0';

  GlobalUnlock(hData);

  CloseClipboard();
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam,
                                LPARAM lParam)
{
  glps_WindowManager *wm =
      (glps_WindowManager *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
  ssize_t window_id = __get_window_id_from_hwnd(wm, hwnd);
  POINT p = {.x = -1, .y = -1};
  static bool key_states[256] = {false};

  switch (msg)
  {
    case WM_DESTROY:
    {
      if (window_id < 0 || !wm || (size_t) window_id >= wm->window_count)
        break;

      bool is_parent = (window_id == 0);
      glps_Win32Window* window = wm->windows[window_id];
      if (!window)
        break;

      if (window->hdc) {
        wglMakeCurrent(NULL, NULL);
        ReleaseDC(window->hwnd, window->hdc);
        window->hdc = NULL;
      }

      if (is_parent) {
        for (SIZE_T j = 1; j < wm->window_count; j++) {
          if (wm->windows[j]) {
            DestroyWindow(wm->windows[j]->hwnd);
          }
        }

        if (wm->win32_ctx && wm->win32_ctx->hglrc) {
          wglDeleteContext(wm->win32_ctx->hglrc);
          wm->win32_ctx->hglrc = NULL;
        }
      }

      free(window);
      wm->windows[window_id] = NULL;

      if (!is_parent) {
        for (SIZE_T j = window_id; j < wm->window_count - 1; j++) {
          wm->windows[j] = wm->windows[j + 1];
        }
        wm->windows[wm->window_count - 1] = NULL;
        wm->window_count--;
      } else {
        wm->window_count = 0;
        PostQuitMessage(0);
      }
      break;
    }

  /* ======== Keyboard Input ========= */
  case WM_KEYDOWN:

    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (!(lParam & 0x40000000))
    { // Prevent auto-repeat
      if (wParam < 256 && !key_states[wParam])
      {
        key_states[wParam] = true;

        if (wm->callbacks.keyboard_callback)
        {
          char key_name[32] = {0};
          GetKeyNameTextA(lParam, key_name, sizeof(key_name));

          BYTE keyboardState[256];
          GetKeyboardState(keyboardState);

          WCHAR unicodeChar = 0;
          char char_value[32] = {0};

          if (ToUnicode(wParam, (lParam >> 16) & 0xFF, keyboardState,
                        &unicodeChar, 1, 0) == 1)
          {
            WideCharToMultiByte(CP_UTF8, 0, &unicodeChar, 1, char_value,
                                sizeof(char_value), NULL, NULL);
          }
          unsigned long keycode = MapVirtualKey(wParam, MAPVK_VK_TO_VSC);
          if (char_value[0] == '\0')
          {
            __get_special_key_name(wParam, char_value, sizeof(char_value));
            if (char_value[0] == '\0')
              strncpy(char_value, key_name, sizeof(char_value) - 1);
          }

          wm->callbacks.keyboard_callback(window_id, true, char_value, keycode,
                                          wm->callbacks.keyboard_data);
        }
      }
    }
    break;

  case WM_KEYUP:

    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (wParam < 256)
    {
      key_states[wParam] = false;

      if (wm->callbacks.keyboard_callback)
      {
        char key_name[32] = {0};
        GetKeyNameTextA(lParam, key_name, sizeof(key_name));

        BYTE keyboardState[256];
        GetKeyboardState(keyboardState);

        WCHAR unicodeChar = 0;
        char char_value[32] = {0};

        if (ToUnicode(wParam, (lParam >> 16) & 0xFF, keyboardState,
                      &unicodeChar, 1, 0) == 1)
        {
          WideCharToMultiByte(CP_UTF8, 0, &unicodeChar, 1, char_value,
                              sizeof(char_value), NULL, NULL);
        }
        unsigned long keycode = MapVirtualKey(wParam, MAPVK_VK_TO_VSC);

        if (char_value[0] == '\0')
        {
          __get_special_key_name(wParam, char_value, sizeof(char_value));
          if (char_value[0] == '\0')
            strncpy(char_value, key_name, sizeof(char_value) - 1);
        }

        wm->callbacks.keyboard_callback(window_id, false, char_value, keycode,
                                        wm->callbacks.keyboard_data);
      }
    }
    break;

    /* ======= Window focus ======= */
  case WM_SETFOCUS:

    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (wm->callbacks.keyboard_enter_callback)
    {
      wm->callbacks.keyboard_enter_callback(window_id,
                                            wm->callbacks.keyboard_enter_data);
    }

    break;

  case WM_KILLFOCUS:

    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (wm->callbacks.keyboard_leave_callback)
    {
      wm->callbacks.keyboard_leave_callback(window_id,
                                            wm->callbacks.keyboard_leave_data);
    }

    break;

    /* ======== Rendering ========= */
  case WM_PAINT:
  {
    PAINTSTRUCT ps;

    HDC hdc = BeginPaint(hwnd, &ps);
    if (window_id < 0 || wm == NULL)
    {
      EndPaint(hwnd, &ps);
      break;
    }

    if (wm->callbacks.window_frame_update_callback)
    {
      wm->callbacks.window_frame_update_callback(
          window_id, wm->callbacks.window_frame_update_data);
    }

    EndPaint(hwnd, &ps);
    break;
  }


  case WM_SIZE:
    if (window_id < 0 || wm == NULL)
    {
      return -1;
    }
    RECT rect;
    if (GetWindowRect(hwnd, &rect))
    {
      int width = rect.right - rect.left;
      int height = rect.bottom - rect.top;
      if (wm->callbacks.window_resize_callback)
      {
        wm->callbacks.window_resize_callback(window_id, width, height,
                                             wm->callbacks.window_resize_data);
      }
    }

    break;
  /* =========== Mouse Input ============ */

    case WM_SETCURSOR: {
      if (LOWORD(lParam) == HTCLIENT) {
        SetCursor(wm->win32_ctx->user_cursor);
        return TRUE;
      }
      break;
    }

  case WM_MOUSEMOVE:
    if (window_id < 0 || wm == NULL)
    {
      break;
    }


    static bool is_mouse_in_window = false;
      double x = GET_X_LPARAM(lParam);
      double y = GET_Y_LPARAM(lParam);

    if (!is_mouse_in_window)
    {
      is_mouse_in_window = true;

      if (wm->callbacks.mouse_enter_callback)
      {
        wm->callbacks.mouse_enter_callback(window_id, (double)x, (double)y,
                                           wm->callbacks.mouse_enter_data);
      }

      TRACKMOUSEEVENT tme;
      tme.cbSize = sizeof(TRACKMOUSEEVENT);
      tme.dwFlags = TME_LEAVE;
      tme.hwndTrack = hwnd;
      TrackMouseEvent(&tme);
    }

    if (wm->callbacks.mouse_move_callback)
    {
      wm->callbacks.mouse_move_callback(window_id, (double)x, (double)y,
                                        wm->callbacks.mouse_move_data);

    }
    break;

  case WM_MOUSELEAVE:
    is_mouse_in_window = false;

    if (wm && wm->callbacks.mouse_leave_callback)
    {
      wm->callbacks.mouse_leave_callback(window_id,
                                         wm->callbacks.mouse_leave_data);
    }
    break;

  case WM_LBUTTONDOWN:
    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (wm->callbacks.mouse_click_callback)
    {
      wm->callbacks.mouse_click_callback(window_id, true,
                                         wm->callbacks.mouse_click_data);
    }
    break;

  case WM_LBUTTONUP:
    if (window_id < 0 || wm == NULL)
    {
      break;
    }

    if (wm->callbacks.mouse_click_callback)
    {
      wm->callbacks.mouse_click_callback(window_id, false,
                                         wm->callbacks.mouse_click_data);
    }
    break;

  case WM_MOUSEWHEEL:
    if (window_id < 0 || wm == NULL)
    {
      break;
    }
    DOUBLE delta = (DOUBLE)(GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA);
    DWORD extra_info = GetMessageExtraInfo();

    // TODO: Improve this to have wider source support.
    GLPS_SCROLL_SOURCE source =
        extra_info == 0 ? GLPS_SCROLL_SOURCE_WHEEL : GLPS_SCROLL_SOURCE_FINGER;

    if (wm->callbacks.mouse_scroll_callback)
    {
      // TODO: impl discrete and is_stopped
      wm->callbacks.mouse_scroll_callback(window_id, GLPS_SCROLL_V_AXIS, source,
                                          delta, -1, false,
                                          wm->callbacks.mouse_scroll_data);
    }
    break;
/*
  case WM_DROPFILES:
  {
    if (window_id < 0 || wm == NULL)
    {
      return -1;
    }

    HDROP hDropInfo = (HDROP)wParam;
    WCHAR filename[MAX_PATH_LENGTH] = {0};
    WCHAR mime[MAX_MIME_LENGTH] = {0};
    CHAR files[MAX_PATH_LENGTH * MAX_FILES] = {0};
    CHAR mime_types[MAX_MIME_LENGTH * MAX_FILES] = {0};

    UINT count = DragQueryFileW(hDropInfo, 0xFFFFFFFF, NULL, 0);
    if (count == 0)
    {
      LOG_ERROR("No files dropped.\n");
      DragFinish(hDropInfo);
      return -1;
    }

    for (UINT i = 0; i < count; ++i)
    {
      if (DragQueryFileW(hDropInfo, i, filename, MAX_PATH_LENGTH) == 0)
      {
        LOG_ERROR("Failed to get filename for file %u.\n", i);
        continue;
      }

      CHAR utf8_filename[MAX_PATH_LENGTH] = {0};
      WideCharToMultiByte(CP_UTF8, 0, filename, -1, utf8_filename,
                          MAX_PATH_LENGTH, NULL, NULL);

      strcat(files, utf8_filename);

      WCHAR *extension = wcsrchr(filename, L'.');
      if (extension == NULL)
      {
        LOG_ERROR("File %s has no extension.\n", utf8_filename);
        continue;
      }

      DWORD data_size = sizeof(mime);
      LONG result = RegGetValueW(HKEY_CLASSES_ROOT, extension, L"Content Type",
                                 RRF_RT_REG_SZ, NULL, mime, &data_size);

      CHAR utf8_mime[MAX_MIME_LENGTH] = {0};
      if (result == ERROR_SUCCESS)
      {
        WideCharToMultiByte(CP_UTF8, 0, mime, -1, utf8_mime, MAX_MIME_LENGTH,
                            NULL, NULL);
      }
      else
      {
        strcpy(utf8_mime, "unknown");
      }

      strcat(mime_types, utf8_mime);

      if (i != count - 1)
      {
        strcat(mime_types, ",");
        strcat(files, ",");
      }
    }

    if (wm->callbacks.drag_n_drop_callback)
    {
   //   wm->callbacks.drag_n_drop_callback(window_id, mime_types, -1, -1, files,
    //                                     wm->callbacks.drag_n_drop_data);
    }

    DragFinish(hDropInfo);
    break;
  }
*/
  default:
    return DefWindowProc(hwnd, msg, wParam, lParam);
  }
  return 0;
}

bool glps_win32_should_close(glps_WindowManager* wm) {
  MSG msg;
  while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
    if (msg.message == WM_QUIT) {
      return true;
    }

    TranslateMessage(&msg);
    DispatchMessage(&msg);

    if (msg.message == WM_DESTROY) {
      ssize_t window_id = -1;
      for (SIZE_T i = 0; i < wm->window_count; i++) {
        if (wm->windows[i] && wm->windows[i]->hwnd == msg.hwnd) {
          window_id = (ssize_t)i;
          break;
        }
      }

      if (window_id >= 0 && wm->callbacks.window_close_callback) {
        wm->callbacks.window_close_callback((SIZE_T)window_id, wm->callbacks.window_close_data);
      }
    }
  }

  return false;
}


static void __init_window_class(glps_WindowManager *wm,
                                const char *class_name)
{
  HINSTANCE hInstance = GetModuleHandle(NULL);

  wm->wc = (WNDCLASSEX){0};
  wm->wc.cbSize = sizeof(WNDCLASSEX);
  wm->wc.style = 0;
  wm->wc.lpfnWndProc = WndProc;
  wm->wc.cbClsExtra = 0;
  wm->wc.cbWndExtra = 0;
  wm->wc.hInstance = hInstance;
  wm->wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
  wm->wc.hCursor =  LoadCursor(NULL, IDC_CROSS);
  wm->wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
  wm->wc.lpszMenuName = NULL;
  wm->wc.lpszClassName = class_name;
  wm->wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

  if (!RegisterClassEx(&wm->wc))
  {
    MessageBox(NULL, "Window Registration Failed!", "Error!",
               MB_ICONEXCLAMATION | MB_OK);
    return;
  }
}

void glps_win32_init(glps_WindowManager *wm)
{

  wm->windows = malloc(sizeof(glps_Win32Window *) * MAX_WINDOWS);
  if (!wm->windows)
  {
    LOG_ERROR("Failed to allocate memory for windows array");
    free(wm);
    return;
  }

  wm->win32_ctx = malloc(sizeof(glps_Win32Context));
  *wm->win32_ctx = (glps_Win32Context){0};
  if (!wm->win32_ctx)
  {
    LOG_ERROR("Failed to allocate memory for WIN32 context");
    free(wm->windows);
    free(wm);
    return;
  }
  wm->win32_ctx->user_cursor = LoadCursor(NULL, IDC_ARROW);
  wm->window_count = 0;

  __init_window_class(wm, "glpsWindowClass");

}

static BOOL SetPixelFormatForOpenGL(HDC hdc)
{
  PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR),
                               1,
                               PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL |
                                   PFD_DOUBLEBUFFER,
                               PFD_TYPE_RGBA,
                               32,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               0,
                               24,
                               8,
                               0,
                               PFD_MAIN_PLANE,
                               0,
                               0,
                               0,
                               0};

  INT pixelFormat = ChoosePixelFormat(hdc, &pfd);
  if (pixelFormat == 0)
  {
    MessageBox(NULL, "ChoosePixelFormat failed!", "Error!",
               MB_ICONEXCLAMATION | MB_OK);
    return FALSE;
  }

  if (!SetPixelFormat(hdc, pixelFormat, &pfd))
  {
    MessageBox(NULL, "SetPixelFormat failed!", "Error!",
               MB_ICONEXCLAMATION | MB_OK);
    return FALSE;
  }

  return TRUE;
}

void glps_win32_get_window_dimensions(glps_WindowManager *wm, size_t window_id,
                                      int *width, int *height)
{
  RECT rect;
  if (GetClientRect(wm->windows[window_id]->hwnd, &rect))
  {
    *width = rect.right - rect.left;
    *height = rect.bottom - rect.top;
  }
}

ssize_t glps_win32_window_create(glps_WindowManager *wm, const char *title,
                                 int width, int height)
{
    HINSTANCE hInstance = GetModuleHandle(NULL);

    glps_Win32Window *win32_window =
        (glps_Win32Window *)malloc(sizeof(glps_Win32Window));

    if (win32_window == NULL)
    {
        MessageBox(NULL, "Win32 Window allocation failed", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        return -1;
    }

    RECT rect = { 0, 0, width, height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    int real_width  = rect.right - rect.left;
    int real_height = rect.bottom - rect.top;

    win32_window->hwnd = CreateWindowEx(
        0, wm->wc.lpszClassName, title, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
        CW_USEDEFAULT, real_width, real_height, NULL, NULL, hInstance, NULL);

    if (win32_window->hwnd == NULL)
    {
        MessageBox(NULL, "CreateWindowEx failed!", "Error!",
                   MB_ICONEXCLAMATION | MB_OK);
        free(win32_window);
        return -1;
    }

    win32_window->hdc = GetDC(win32_window->hwnd);

    if (!SetPixelFormatForOpenGL(win32_window->hdc))
    {
        ReleaseDC(win32_window->hwnd, win32_window->hdc);
        DestroyWindow(win32_window->hwnd);
        free(win32_window);
        return -1;
    }

    if (wm->window_count == 0)
    {
        wm->win32_ctx->hglrc = wglCreateContext(win32_window->hdc);
        if (!wm->win32_ctx->hglrc)
        {
            MessageBox(NULL, "wglCreateContext failed!", "Error!",
                       MB_ICONEXCLAMATION | MB_OK);
            ReleaseDC(win32_window->hwnd, win32_window->hdc);
            DestroyWindow(win32_window->hwnd);
            free(win32_window);
            return -1;
        }
    }

    wglMakeCurrent(win32_window->hdc, wm->win32_ctx->hglrc);

    ShowWindow(win32_window->hwnd, SW_SHOW);
    UpdateWindow(win32_window->hwnd);
    DragAcceptFiles(win32_window->hwnd, TRUE);

    RECT client_rect;
    GetClientRect(win32_window->hwnd, &client_rect);
    int client_width  = client_rect.right - client_rect.left;
    int client_height = client_rect.bottom - client_rect.top;

    snprintf(win32_window->properties.title,
             sizeof(win32_window->properties.title), "%s", title);
    win32_window->properties.width = client_width;
    win32_window->properties.height = client_height;

    wm->windows[wm->window_count] = win32_window;

    SetWindowLongPtr(win32_window->hwnd, GWLP_USERDATA, (LONG_PTR)wm);

    return wm->window_count++;
}

void glps_win32_destroy(glps_WindowManager *wm)
{
  for (size_t i = 0; i < wm->window_count; ++i)
  {
    if (wm->windows[i] != NULL)
    {
      DragAcceptFiles(wm->windows[i]->hwnd, FALSE);
      free(wm->windows[i]);
      wm->windows[i] = NULL;
    }
  }

  if (wm->windows != NULL)
  {
    free(wm->windows);
    wm->windows = NULL;
  }

  if (wm->win32_ctx != NULL)
  {
    free(wm->win32_ctx);
    wm->win32_ctx = NULL;
  }

  if (wm != NULL)
  {
    free(wm);
    wm = NULL;
  }
}

HDC glps_win32_get_window_hdc(glps_WindowManager *wm, size_t window_id)
{
  return wm->windows[window_id]->hdc;
}


void glps_win32_cursor_change(glps_WindowManager* wm, GLPS_CURSOR_TYPE cursor_type) {
  if (!wm || !wm->win32_ctx) {
    LOG_ERROR("Window manager invalid. Couldn't change cursor.");
    return;
  }

  LPCSTR cursor_id;
  switch(cursor_type) {
    case GLPS_CURSOR_ARROW: cursor_id = IDC_ARROW; break;
    case GLPS_CURSOR_IBEAM: cursor_id = IDC_IBEAM; break;
    case GLPS_CURSOR_CROSSHAIR: cursor_id = IDC_CROSS; break;
    case GLPS_CURSOR_HAND: cursor_id = IDC_HAND; break;
    case GLPS_CURSOR_HRESIZE: cursor_id = IDC_SIZEWE; break;
    case GLPS_CURSOR_VRESIZE: cursor_id = IDC_SIZENS; break;
    case GLPS_CURSOR_NOT_ALLOWED: cursor_id = IDC_NO; break;
    default: cursor_id = IDC_ARROW;
  }

  wm->win32_ctx->user_cursor = LoadCursor(NULL, cursor_id);
  SetCursor(wm->win32_ctx->user_cursor);
}