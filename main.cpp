#include <windows.h>
#include <wrl.h>
#include <WebView2.h>
#include <shellapi.h>
#include <string>
#include <vector>

using namespace Microsoft::WRL;

struct Tab
{
    ComPtr<ICoreWebView2Controller> controller;
    ComPtr<ICoreWebView2> webview;
};

std::vector<Tab> tabs;
int currentTab = -1;

HWND btnBack, btnForward, btnReload, btnNewTab, btnCloseTab;

// ================= TAB CREATION =================
void CreateNewTab(HWND hwnd, std::wstring url)
{
    CreateCoreWebView2EnvironmentWithOptions(
        nullptr, nullptr, nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [hwnd, url](HRESULT result, ICoreWebView2Environment* env) -> HRESULT
            {
                if (FAILED(result) || !env)
                {
                    MessageBoxA(NULL, "WebView2 ENV FAILED", "ERROR", MB_OK);
                    return S_OK;
                }

                env->CreateCoreWebView2Controller(
                    hwnd,
                    Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                        [hwnd, url](HRESULT result, ICoreWebView2Controller* ctrl) -> HRESULT
                        {
                            if (FAILED(result) || !ctrl)
                            {
                                MessageBoxA(NULL, "Controller FAILED", "ERROR", MB_OK);
                                return S_OK;
                            }

                            Tab tab;
                            tab.controller = ctrl;
                            ctrl->get_CoreWebView2(&tab.webview);

                            RECT bounds;
                            GetClientRect(hwnd, &bounds);
                            bounds.top = 40;
                            tab.controller->put_Bounds(bounds);

                            // 🔥 Hide all existing tabs
                            for (auto& t : tabs)
                            {
                                if (t.controller)
                                    t.controller->put_IsVisible(FALSE);
                            }

                            // 🔥 Show this tab
                            tab.controller->put_IsVisible(TRUE);

                            tab.webview->Navigate(url.c_str());

                            tabs.push_back(tab);
                            currentTab = tabs.size() - 1;

                            return S_OK;
                        }).Get());

                return S_OK;
            }).Get());
}

// ================= SWITCH TAB =================
void ShowTab(int index)
{
    if (index < 0 || index >= (int)tabs.size()) return;

    for (int i = 0; i < tabs.size(); i++)
    {
        if (tabs[i].controller)
            tabs[i].controller->put_IsVisible(i == index);
    }

    currentTab = index;
}

// ================= WINDOW PROC =================
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
        btnBack = CreateWindowA("BUTTON", "<", WS_VISIBLE | WS_CHILD, 5, 5, 30, 30, hwnd, (HMENU)1, NULL, NULL);
        btnForward = CreateWindowA("BUTTON", ">", WS_VISIBLE | WS_CHILD, 40, 5, 30, 30, hwnd, (HMENU)2, NULL, NULL);
        btnReload = CreateWindowA("BUTTON", "R", WS_VISIBLE | WS_CHILD, 75, 5, 30, 30, hwnd, (HMENU)3, NULL, NULL);
        btnNewTab = CreateWindowA("BUTTON", "+", WS_VISIBLE | WS_CHILD, 110, 5, 30, 30, hwnd, (HMENU)4, NULL, NULL);
        btnCloseTab = CreateWindowA("BUTTON", "x", WS_VISIBLE | WS_CHILD, 145, 5, 30, 30, hwnd, (HMENU)5, NULL, NULL);

        CreateNewTab(hwnd, L"https://www.google.com");
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case 1: // Back
            if (currentTab >= 0) tabs[currentTab].webview->GoBack();
            break;

        case 2: // Forward
            if (currentTab >= 0) tabs[currentTab].webview->GoForward();
            break;

        case 3: // Reload
            if (currentTab >= 0) tabs[currentTab].webview->Reload();
            break;

        case 4: // New Tab
            CreateNewTab(hwnd, L"https://www.google.com");
            break;

        case 5: // Close Tab
            if (currentTab >= 0 && !tabs.empty())
            {
                tabs.erase(tabs.begin() + currentTab);

                if (tabs.empty())
                {
                    currentTab = -1;
                }
                else
                {
                    currentTab = max(0, currentTab - 1);
                    ShowTab(currentTab);
                }
            }
            break;
        }
        break;

    case WM_SIZE:
        if (currentTab >= 0)
        {
            RECT bounds;
            GetClientRect(hwnd, &bounds);
            bounds.top = 40;
            tabs[currentTab].controller->put_Bounds(bounds);
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ================= MAIN =================
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
    const char CLASS_NAME[] = "HyperViewWindow";

    WNDCLASSA wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    RegisterClassA(&wc);

    HWND hwnd = CreateWindowExA(
        0,
        CLASS_NAME,
        "HyperView Browser",
        WS_OVERLAPPEDWINDOW,
        100, 100, 1200, 800,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);

    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}