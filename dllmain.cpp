#include <Windows.h>
#include "pch.h"
#include <iostream>
#include<TlHelp32.h>
#include <tchar.h> // _tcscmp
#include <vector>

#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")
#include "detours.h"
#pragma comment(lib, "detours.lib")




// hook function end scene to show our drawing before fuction is executed so we need the address



HMODULE myhModule;

// hook function end scene to show our drawing before fuction is executed so we need the address
typedef HRESULT(__stdcall* endScene)(IDirect3DDevice9* pDevice);
endScene pEndScene;

LPD3DXFONT font;


DWORD GetModuleBaseAddress(TCHAR* lpszModuleName, DWORD procID) {
    DWORD dwModuleBaseAddress = 0;
    HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, procID); // make snapshot of all modules within process
    MODULEENTRY32 ModuleEntry32 = { 0 };
    ModuleEntry32.dwSize = sizeof(MODULEENTRY32);

    if (Module32First(hSnapshot, &ModuleEntry32)) //store first Module in ModuleEntry32
    {
        do {
            if (_tcscmp(ModuleEntry32.szModule, lpszModuleName) == 0) // if Found Module matches Module we look for -> done!
            {
                dwModuleBaseAddress = (DWORD)ModuleEntry32.modBaseAddr;
                break;
            }
        } while (Module32Next(hSnapshot, &ModuleEntry32)); // go through Module entries in Snapshot and store in ModuleEntry32


    }
    CloseHandle(hSnapshot);
    return dwModuleBaseAddress;
}

HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) {
    //now here we can create our own graphics
    int padding = 2;
    int rectx1 = 100, rectx2 = 300, recty1 = 50, recty2 = 100;
    D3DRECT rectangle = { rectx1, recty1, rectx2, recty2 };
    pDevice->Clear(1, &rectangle, D3DCLEAR_TARGET, D3DCOLOR_ARGB(255, 0, 0, 0), 0.0f, 0); // this draws a rectangle
    if (!font)
        D3DXCreateFont(pDevice, 16, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);
    RECT textRectangle;
    SetRect(&textRectangle, rectx1 + padding, recty1 + padding, rectx2 - padding, recty2 - padding);
    font->DrawText(NULL, "[C] Infinite Health", -1, &textRectangle, DT_NOCLIP | DT_LEFT, D3DCOLOR_ARGB(255, 153, 255, 153)); //draw text;
    //font->DrawText(NULL, "[X] to Unload Menu", -1, &textRectangle, DT_NOCLIP | DT_LEFT, D3DCOLOR_ARGB(255, 153, 255, 153)); //draw text;
    return pEndScene(pDevice); // call original endScene 
}

void hookEndScene() {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION); // create IDirect3D9 object
    if (!pD3D)
        return;

    D3DPRESENT_PARAMETERS d3dparams = { 0 };
    d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dparams.hDeviceWindow = GetForegroundWindow();
    d3dparams.Windowed = true;

    IDirect3DDevice9* pDevice = nullptr;

    HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dparams.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dparams, &pDevice);
    if (FAILED(result) || !pDevice) {
        pD3D->Release();
        return;
    }
    //if device creation worked out -> lets get the virtual table:
    void** vTable = *reinterpret_cast<void***>(pDevice);

    //now detour:

    pEndScene = (endScene)DetourFunction((PBYTE)vTable[42], (PBYTE)hookedEndScene);

    pDevice->Release();
    pD3D->Release();
}



//free Library Function to Enable editing while dll is attached to game
DWORD __stdcall EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(myhModule, 0);
}







DWORD WINAPI Menue() {
    //
    AllocConsole(); // alocate console in memory
    FILE* fp; // create dummy file poiting to original
    freopen_s(&fp, "CONOUT$", "w", stdout); // output only

    hookEndScene();
    std::cout << "[-] Console Aloccated" << std::endl;
    std::cout << "[-] DLL Injected\n" << std::endl;
    // Offsets*************************************************************************
    
    DWORD procID = NULL; // process id (A DWORD store values like a variable)


    HWND hwnd = FindWindowA(NULL, "Counter-Strike Source"); // function finds process with name we want
    GetWindowThreadProcessId(hwnd, &procID); // takes process it found assigns to procID
    HANDLE handle = NULL;
    handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procID); // open the process id of program with full priviledge

    std::cout << "procID: " << procID << std::endl;
    char baseName[] = "server.dll"; //((((("server.dll")))))))+004EE66C
    DWORD gameBaseAddress = GetModuleBaseAddress(_T(baseName), procID); // How to get base address - start at entry point (server.dll)
    std::cout << "gameBaseAddress: " << std::hex << gameBaseAddress << std::endl;


    //Helath ***************************************************************************************
    DWORD HealthBaseAdress = 0x00512360; // offsetGameToBaseAdress
    std::vector<DWORD> HealthPointOffsets{ 0x20,0x34C,0x54,0x50,0x8,0x20, 0xE4 }; // values that show when you click to
    // change address on cheat engine
    std::cout << "debugginfo: HealthBaseAdress = " << std::hex << HealthBaseAdress << std::endl;

    DWORD healthBaseAddress = NULL;
    //Get value at gamebase+offset -> store it in bulletBaseAddress
    ReadProcessMemory(myhModule, (LPVOID)(gameBaseAddress + HealthBaseAdress), &healthBaseAddress, sizeof(healthBaseAddress), NULL);
    std::cout << "debugginfo: healthBaseaddress = " << std::hex << healthBaseAddress << std::endl;
    DWORD healthCodeAddress = healthBaseAddress; //the Adress we need -> change now while going through offsets
    for (int i = 0; i < HealthPointOffsets.size() - 1; i++) // -1 because we dont want the value at the last offset
    {
        ReadProcessMemory(myhModule, (LPVOID)(healthCodeAddress + HealthPointOffsets.at(i)), &healthCodeAddress, sizeof(healthBaseAddress), NULL);
        std::cout << "debugginfo: Value at offset = " << healthBaseAddress << std::endl;
    }
    healthCodeAddress += HealthPointOffsets.at(HealthPointOffsets.size() - 1); //Add Last offset (0) -> done!!
    // *********************************************************************************************


// *************************************************************************
    
    



    
    std::cout << "[X] to Unload DLL\n" << std::endl;



    while (1) {
        Sleep(100);
        if (GetAsyncKeyState('X') & 0x8000) { // & 0x8000 is used to check if key is pressed at the moment
            std::cout << "\nUnloading DLL";
            DetourRemove((PBYTE)pEndScene, (PBYTE)hookedEndScene); //unhook to avoid game crash
            Sleep(300);
            break;
        }
        if (GetAsyncKeyState('C') & 0x8000) { // & 0x8000 is used to check if key is pressed at the moment
            std::cout << "\nInfinite Health";
            
            int health = 9999;
            
            WriteProcessMemory(myhModule, (LPVOID)(healthCodeAddress), &health, 4, 0);
            Sleep(300);
            //break;
        }

    }
    fclose(fp);
    FreeConsole();
    CreateThread(0, 0, EjectThread, 0, 0, 0); // call create thread function
    return 0;
}


BOOL APIENTRY DllMain(HMODULE hModule,
    DWORD  ul_reason_for_call,
    LPVOID lpReserved
)
{
    switch (ul_reason_for_call)
    {
        //when dll is loaded
    case DLL_PROCESS_ATTACH:
        myhModule = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);// create thread to run our function
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
