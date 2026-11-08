#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <sddl.h>
#include <shlobj.h>
#include <tlhelp32.h>

bool ExtractBlankIcon(char *outPath, DWORD maxLen) {
  char targetDir[MAX_PATH];
  if (ExpandEnvironmentStringsA("%ProgramData%\\DeLNK", targetDir,
                                sizeof(targetDir)) == 0) {
    return false;
  }

  CreateDirectoryA(targetDir, NULL);

  wsprintfA(outPath, "%s\\blank.ico", targetDir);

  HRSRC hRes = FindResourceA(NULL, MAKEINTRESOURCE(101), RT_RCDATA);
  if (hRes == NULL)
    return false;

  HGLOBAL hData = LoadResource(NULL, hRes);
  if (hData == NULL)
    return false;

  DWORD size = SizeofResource(NULL, hRes);
  void *pData = LockResource(hData);
  if (pData == NULL || size == 0)
    return false;

  SetFileAttributesA(outPath, FILE_ATTRIBUTE_NORMAL);
  HANDLE hFile = CreateFileA(outPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL, NULL);
  if (hFile == INVALID_HANDLE_VALUE) {
    return false;
  }

  DWORD written = 0;
  WriteFile(hFile, pData, size, &written, NULL);
  CloseHandle(hFile);

  return (written == size);
}

void RemoveBlankIconFile(bool delayUntilReboot = false) {
  char targetPath[MAX_PATH];
  if (ExpandEnvironmentStringsA("%ProgramData%\\DeLNK\\blank.ico",
                                targetPath, sizeof(targetPath)) > 0) {
    SetFileAttributesA(targetPath, FILE_ATTRIBUTE_NORMAL);
    if (delayUntilReboot) {
      MoveFileExA(targetPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    } else {
      DeleteFileA(targetPath);
    }
  }
  if (ExpandEnvironmentStringsA("%ProgramData%\\DeLNK", targetPath,
                                sizeof(targetPath)) > 0) {
    if (delayUntilReboot) {
      MoveFileExA(targetPath, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    } else {
      RemoveDirectoryA(targetPath);
    }
  }
  // Also clean up legacy %ProgramData%\DeArrow and %LOCALAPPDATA%\DeArrow if present
  if (ExpandEnvironmentStringsA("%ProgramData%\\DeArrow\\blank.ico",
                                targetPath, sizeof(targetPath)) > 0) {
    SetFileAttributesA(targetPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(targetPath);
  }
  if (ExpandEnvironmentStringsA("%ProgramData%\\DeArrow", targetPath,
                                sizeof(targetPath)) > 0) {
    RemoveDirectoryA(targetPath);
  }
  if (ExpandEnvironmentStringsA("%LOCALAPPDATA%\\DeArrow\\blank.ico",
                                targetPath, sizeof(targetPath)) > 0) {
    SetFileAttributesA(targetPath, FILE_ATTRIBUTE_NORMAL);
    DeleteFileA(targetPath);
  }
  if (ExpandEnvironmentStringsA("%LOCALAPPDATA%\\DeArrow", targetPath,
                                sizeof(targetPath)) > 0) {
    RemoveDirectoryA(targetPath);
  }
}

bool EnablePrivilege(const char *privilegeName) {
  HANDLE hToken = NULL;
  if (OpenProcessToken(GetCurrentProcess(),
                       TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
    TOKEN_PRIVILEGES tp;
    LUID luid;
    if (LookupPrivilegeValueA(NULL, privilegeName, &luid)) {
      tp.PrivilegeCount = 1;
      tp.Privileges[0].Luid = luid;
      tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
      AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL,
                            NULL);
    }
    CloseHandle(hToken);
  }
  return true;
}

void ClearIconCacheInDir(const char *localAppData) {
  if (!localAppData || localAppData[0] == '\0') {
    return;
  }

  char iconCacheFile[MAX_PATH];
  wsprintfA(iconCacheFile, "%s\\IconCache.db", localAppData);
  SetFileAttributesA(iconCacheFile, FILE_ATTRIBUTE_NORMAL);
  for (int r = 0; r < 5; r++) {
    if (DeleteFileA(iconCacheFile) || GetLastError() == ERROR_FILE_NOT_FOUND) {
      break;
    }
    Sleep(50);
  }

  char explorerDir[MAX_PATH];
  wsprintfA(explorerDir, "%s\\Microsoft\\Windows\\Explorer\\iconcache_*.db", localAppData);

  WIN32_FIND_DATAA fd;
  HANDLE hFind = FindFirstFileA(explorerDir, &fd);
  if (hFind != INVALID_HANDLE_VALUE) {
    do {
      char filePath[MAX_PATH];
      wsprintfA(filePath, "%s\\Microsoft\\Windows\\Explorer\\%s", localAppData, fd.cFileName);
      SetFileAttributesA(filePath, FILE_ATTRIBUTE_NORMAL);
      for (int r = 0; r < 5; r++) {
        if (DeleteFileA(filePath) || GetLastError() == ERROR_FILE_NOT_FOUND) {
          break;
        }
        Sleep(50);
      }
    } while (FindNextFileA(hFind, &fd));
    FindClose(hFind);
  }
}

void ClearAllIconCaches(HANDLE hUserToken) {
  char localAppData[MAX_PATH];
  if (ExpandEnvironmentStringsA("%LOCALAPPDATA%", localAppData, sizeof(localAppData)) > 0) {
    ClearIconCacheInDir(localAppData);
  }

  if (hUserToken != NULL) {
    DWORD dwSize = 0;
    GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize > 0) {
      BYTE *pBuf = (BYTE *)LocalAlloc(LPTR, dwSize);
      if (pBuf != NULL) {
        if (GetTokenInformation(hUserToken, TokenUser, pBuf, dwSize, &dwSize)) {
          TOKEN_USER *pTokenUser = (TOKEN_USER *)pBuf;
          LPSTR szSid = NULL;
          if (ConvertSidToStringSidA(pTokenUser->User.Sid, &szSid)) {
            char profileKey[MAX_PATH];
            wsprintfA(profileKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\ProfileList\\%s", szSid);
            HKEY hKey;
            if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, profileKey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
              char profilePath[MAX_PATH];
              DWORD pathSize = sizeof(profilePath);
              DWORD type = 0;
              if (RegQueryValueExA(hKey, "ProfileImagePath", NULL, &type, (LPBYTE)profilePath, &pathSize) == ERROR_SUCCESS) {
                char expandedPath[MAX_PATH];
                if (ExpandEnvironmentStringsA(profilePath, expandedPath, sizeof(expandedPath)) > 0) {
                  char userLocalAppData[MAX_PATH];
                  wsprintfA(userLocalAppData, "%s\\AppData\\Local", expandedPath);
                  if (lstrcmpiA(localAppData, userLocalAppData) != 0) {
                    ClearIconCacheInDir(userLocalAppData);
                  }
                }
              }
              RegCloseKey(hKey);
            }
            LocalFree(szSid);
          }
        }
        LocalFree(pBuf);
      }
    }
  }
}

void TerminateAllExplorerProcesses() {
  HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
  if (hSnap == INVALID_HANDLE_VALUE) return;

  PROCESSENTRY32 pe;
  pe.dwSize = sizeof(pe);
  if (Process32First(hSnap, &pe)) {
    do {
      if (lstrcmpiA(pe.szExeFile, "explorer.exe") == 0) {
        HANDLE hProc = OpenProcess(PROCESS_TERMINATE | SYNCHRONIZE, FALSE, pe.th32ProcessID);
        if (hProc != NULL) {
          TerminateProcess(hProc, 0);
          WaitForSingleObject(hProc, 1000);
          CloseHandle(hProc);
        }
      }
    } while (Process32Next(hSnap, &pe));
  }
  CloseHandle(hSnap);
}

HANDLE GetTargetUserToken() {
  HANDLE hNewToken = NULL;
  HWND hWndTray = FindWindowA("Shell_TrayWnd", NULL);
  if (hWndTray != NULL) {
    DWORD dwPID = 0;
    GetWindowThreadProcessId(hWndTray, &dwPID);
    if (dwPID != 0) {
      HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
      if (hProc != NULL) {
        HANDLE hToken = NULL;
        if (OpenProcessToken(hProc, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY, &hToken)) {
          DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hNewToken);
          CloseHandle(hToken);
        }
        CloseHandle(hProc);
      }
    }
  }

  if (hNewToken == NULL) {
    HWND hWndProg = FindWindowA("Progman", NULL);
    if (hWndProg != NULL) {
      DWORD dwPID = 0;
      GetWindowThreadProcessId(hWndProg, &dwPID);
      if (dwPID != 0) {
        HANDLE hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwPID);
        if (hProc != NULL) {
          HANDLE hToken = NULL;
          if (OpenProcessToken(hProc, TOKEN_DUPLICATE | TOKEN_ASSIGN_PRIMARY | TOKEN_QUERY, &hToken)) {
            DuplicateTokenEx(hToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hNewToken);
            CloseHandle(hToken);
          }
          CloseHandle(hProc);
        }
      }
    }
  }

  if (hNewToken == NULL) {
    HMODULE hWts = LoadLibraryA("wtsapi32.dll");
    if (hWts != NULL) {
      typedef BOOL (WINAPI *pfnWTSQueryUserToken)(ULONG, PHANDLE);
      pfnWTSQueryUserToken pWTSQueryUserToken = (pfnWTSQueryUserToken)GetProcAddress(hWts, "WTSQueryUserToken");
      if (pWTSQueryUserToken != NULL) {
        HANDLE hUserToken = NULL;
        if (pWTSQueryUserToken(WTSGetActiveConsoleSessionId(), &hUserToken)) {
          DuplicateTokenEx(hUserToken, TOKEN_ALL_ACCESS, NULL, SecurityImpersonation, TokenPrimary, &hNewToken);
          CloseHandle(hUserToken);
        }
      }
      FreeLibrary(hWts);
    }
  }

  return hNewToken;
}

void DeleteShellIconsSlot(HKEY hRoot, const char *subKey) {
  HKEY hKey;
  if (RegOpenKeyExA(hRoot, subKey, 0, KEY_READ | KEY_WRITE, &hKey) == ERROR_SUCCESS) {
    RegDeleteValueA(hKey, "29");
    DWORD numSubKeys = 0;
    DWORD numValues = 0;
    if (RegQueryInfoKeyA(hKey, NULL, NULL, NULL, &numSubKeys, NULL, NULL, &numValues, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
      if (numSubKeys == 0 && numValues == 0) {
        RegCloseKey(hKey);
        RegDeleteKeyA(hRoot, subKey);
        return;
      }
    }
    RegCloseKey(hKey);
  }
}

void RestartExplorerUnelevated() {
  EnablePrivilege("SeImpersonatePrivilege");

  HANDLE hNewToken = GetTargetUserToken();

  HWND hWndTray = FindWindowA("Shell_TrayWnd", NULL);
  if (hWndTray != NULL) {
    PostMessageA(hWndTray, WM_USER + 436, 0, 0);
  }

  Sleep(500);

  // Terminate all remaining explorer.exe instances so iconcache files are completely unlocked
  TerminateAllExplorerProcesses();

  // Poll until all Explorer processes are dead (up to 3 seconds)
  for (int i = 0; i < 30; i++) {
    HWND tray = FindWindowA("Shell_TrayWnd", NULL);
    HWND prog = FindWindowA("Progman", NULL);
    if (tray == NULL && prog == NULL) {
      break;
    }
    Sleep(100);
  }

  Sleep(200);

  // Purge corrupted/stale icon caches while Explorer is dead
  ClearAllIconCaches(hNewToken);

  // Restart Explorer unelevated
  if (hNewToken != NULL) {
    STARTUPINFOW si = {sizeof(si)};
    si.lpDesktop = (LPWSTR)L"winsta0\\default";
    PROCESS_INFORMATION pi = {0};

    wchar_t cmd[MAX_PATH];
    if (ExpandEnvironmentStringsW(L"%SystemRoot%\\explorer.exe", cmd,
                                  MAX_PATH) == 0) {
      lstrcpyW(cmd, L"C:\\Windows\\explorer.exe");
    }

    if (CreateProcessWithTokenW(hNewToken, 0, NULL, cmd, 0,
                                NULL, NULL, &si, &pi)) {
      CloseHandle(pi.hProcess);
      CloseHandle(pi.hThread);
    }
    CloseHandle(hNewToken);
  }

  Sleep(300);
  SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
}

void ClearScreen() {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  if (!GetConsoleScreenBufferInfo(hOut, &csbi)) return;
  DWORD cellCount = csbi.dwSize.X * csbi.dwSize.Y;
  COORD topLeft = {0, 0};
  DWORD written;
  FillConsoleOutputCharacterA(hOut, ' ', cellCount, topLeft, &written);
  FillConsoleOutputAttribute(hOut, csbi.wAttributes, cellCount, topLeft, &written);
  SetConsoleCursorPosition(hOut, topLeft);
}

void PrintStr(const char *str) {
  HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
  if (hOut && hOut != INVALID_HANDLE_VALUE) {
    DWORD written = 0;
    WriteFile(hOut, str, lstrlenA(str), &written, NULL);
  }
}

bool ReadLine(char *buf, DWORD maxLen) {
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  if (!hIn || hIn == INVALID_HANDLE_VALUE)
    return false;

  DWORD read = 0;
  if (!ReadFile(hIn, buf, maxLen - 1, &read, NULL) || read == 0) {
    return false;
  }
  buf[read] = '\0';
  for (DWORD i = 0; i < read; i++) {
    if (buf[i] == '\r' || buf[i] == '\n') {
      buf[i] = '\0';
      break;
    }
  }
  return true;
}

bool StringEqualsIgnoreCase(const char *a, const char *b) {
  while (*a && *b) {
    char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
    char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
    if (ca != cb)
      return false;
    a++;
    b++;
  }
  return (*a == '\0' && *b == '\0');
}

void WaitKey() {
  HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
  if (hIn && hIn != INVALID_HANDLE_VALUE) {
    DWORD mode = 0;
    GetConsoleMode(hIn, &mode);
    SetConsoleMode(hIn, 0);
    INPUT_RECORD rec;
    DWORD read = 0;
    while (ReadConsoleInputA(hIn, &rec, 1, &read)) {
      if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.bKeyDown) {
        break;
      }
    }
    SetConsoleMode(hIn, mode);
  }
}

bool ToggleArrows(bool remove) {
  const char *subKey = "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Shell Icons";
  bool success = false;

  HANDLE hUserToken = GetTargetUserToken();
  LPSTR szUserSid = NULL;
  if (hUserToken != NULL) {
    DWORD dwSize = 0;
    GetTokenInformation(hUserToken, TokenUser, NULL, 0, &dwSize);
    if (dwSize > 0) {
      BYTE *pBuf = (BYTE *)LocalAlloc(LPTR, dwSize);
      if (pBuf != NULL) {
        if (GetTokenInformation(hUserToken, TokenUser, pBuf, dwSize, &dwSize)) {
          TOKEN_USER *pTokenUser = (TOKEN_USER *)pBuf;
          ConvertSidToStringSidA(pTokenUser->User.Sid, &szUserSid);
        }
        LocalFree(pBuf);
      }
    }
    CloseHandle(hUserToken);
  }

  if (remove) {
    char iconPath[MAX_PATH];
    char regValue[MAX_PATH + 10];
    if (ExtractBlankIcon(iconPath, sizeof(iconPath))) {
      wsprintfA(regValue, "%s,0", iconPath);

      HKEY hKey;
      LSTATUS status = RegCreateKeyExA(
          HKEY_LOCAL_MACHINE, subKey, 0,
          NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_WOW64_64KEY, NULL, &hKey,
          NULL);

      if (status == ERROR_SUCCESS) {
        status = RegSetValueExA(hKey, "29", 0, REG_SZ, (const BYTE *)regValue,
                                lstrlenA(regValue) + 1);
        RegCloseKey(hKey);
        if (status == ERROR_SUCCESS) {
          success = true;
        } else {
          PrintStr("\nERROR: Failed to write registry value.\n");
        }
      } else {
        PrintStr("\nERROR: Failed to open registry key.\n");
      }

      // Remove any conflicting override from HKCU and active user's hive
      DeleteShellIconsSlot(HKEY_CURRENT_USER, subKey);
      if (szUserSid != NULL) {
        char userSubKey[MAX_PATH];
        wsprintfA(userSubKey, "%s\\%s", szUserSid, subKey);
        DeleteShellIconsSlot(HKEY_USERS, userSubKey);
      }
    } else {
      PrintStr("\nERROR: Failed to create hidden blank icon file.\n");
    }
  } else {
    // Restore default: remove slot 29 and remove empty Shell Icons keys from HKLM, HKCU, and HKEY_USERS
    DeleteShellIconsSlot(HKEY_LOCAL_MACHINE, subKey);
    DeleteShellIconsSlot(HKEY_CURRENT_USER, subKey);
    if (szUserSid != NULL) {
      char userSubKey[MAX_PATH];
      wsprintfA(userSubKey, "%s\\%s", szUserSid, subKey);
      DeleteShellIconsSlot(HKEY_USERS, userSubKey);
    }
    success = true;
  }

  if (szUserSid != NULL) {
    LocalFree(szUserSid);
  }

  return success;
}

bool ArgEquals(LPCWSTR arg, LPCWSTR opt1, LPCWSTR opt2 = NULL, LPCWSTR opt3 = NULL, LPCWSTR opt4 = NULL, LPCWSTR opt5 = NULL) {
  if (lstrcmpiW(arg, opt1) == 0) return true;
  if (opt2 && lstrcmpiW(arg, opt2) == 0) return true;
  if (opt3 && lstrcmpiW(arg, opt3) == 0) return true;
  if (opt4 && lstrcmpiW(arg, opt4) == 0) return true;
  if (opt5 && lstrcmpiW(arg, opt5) == 0) return true;
  return false;
}

void PrintHelp() {
  PrintStr("DeLNK CLI - Toggle Windows Shortcut Arrow Overlays\n\n");
  PrintStr("Usage: delnk-cli [action] [restart-option]\n\n");
  PrintStr("Actions:\n");
  PrintStr("  -rm, --remove, /rm    Hide shortcut arrows\n");
  PrintStr("  -rs, --restore, /rs   Restore default shortcut arrows\n\n");
  PrintStr("Restart Options:\n");
  PrintStr("  -y,  --yes,    /y     Restart Windows Explorer automatically\n");
  PrintStr("  -n,  --no,     /n     Do not restart Windows Explorer\n\n");
  PrintStr("Other:\n");
  PrintStr("  -h,  --help,   /?     Show this help message\n\n");
  PrintStr("If run with no arguments, interactive mode will launch.\n");
}

extern "C" void __stdcall mainCRTStartup() {
  int numArgs = 0;
  LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &numArgs);

  if (argv != NULL && numArgs > 1) {
    bool hasAction = false;
    bool remove = false;
    bool hasRestart = false;
    bool restart = false;
    bool showHelp = false;

    for (int i = 1; i < numArgs; i++) {
      if (ArgEquals(argv[i], L"-h", L"--help", L"/?", L"-?", L"/h")) {
        showHelp = true;
      } else if (ArgEquals(argv[i], L"-rm", L"--remove", L"/rm", L"remove", L"rm")) {
        hasAction = true;
        remove = true;
      } else if (ArgEquals(argv[i], L"-rs", L"--restore", L"/rs", L"restore", L"rs")) {
        hasAction = true;
        remove = false;
      } else if (ArgEquals(argv[i], L"-y", L"--yes", L"/y", L"yes", L"y")) {
        hasRestart = true;
        restart = true;
      } else if (ArgEquals(argv[i], L"-n", L"--no", L"/n", L"no", L"n")) {
        hasRestart = true;
        restart = false;
      } else {
        PrintStr("\nUnknown option: ");
        char buf[128];
        WideCharToMultiByte(CP_ACP, 0, argv[i], -1, buf, sizeof(buf), NULL, NULL);
        PrintStr(buf);
        PrintStr("\n\n");
        showHelp = true;
      }
    }

    LocalFree(argv);

    if (showHelp) {
      PrintHelp();
      ExitProcess(0);
    }

    if (hasAction) {
      if (remove) {
        PrintStr("> Removing shortcut arrows...\n");
      } else {
        PrintStr("> Restoring default shortcut arrows...\n");
      }

      if (ToggleArrows(remove)) {
        PrintStr("\nRegistry updated successfully!\n");
        if (hasRestart) {
          if (restart) {
            PrintStr("> Restarting Windows Explorer...\n");
            RestartExplorerUnelevated();
            if (!remove) {
              RemoveBlankIconFile(false);
            }
          } else {
            PrintStr("> Changes will apply on next boot or when Windows Explorer is restarted.\n");
            if (!remove) {
              RemoveBlankIconFile(true);
            }
          }
          ExitProcess(0);
        } else {
          while (true) {
            PrintStr("\n> Would you like to restart Windows Explorer now to apply changes? (y/n): ");
            char input[64];
            if (!ReadLine(input, sizeof(input))) {
              ExitProcess(0);
            }
            if (StringEqualsIgnoreCase(input, "y")) {
              RestartExplorerUnelevated();
              if (!remove) {
                RemoveBlankIconFile(false);
              }
              ExitProcess(0);
            } else if (StringEqualsIgnoreCase(input, "n")) {
              PrintStr("\n> Changes will apply on next boot or when Windows Explorer is restarted.\n");
              if (!remove) {
                RemoveBlankIconFile(true);
              }
              ExitProcess(0);
            } else {
              PrintStr("\nInvalid choice. Please type 'y' or 'n'.\n");
            }
          }
        }
      } else {
        ExitProcess(1);
      }
    }
  } else if (argv != NULL) {
    LocalFree(argv);
  }

  char input[64];

  while (true) {
    PrintStr("> Type 'remove' (or 'rm') to hide shortcut arrows, 'restore' (or 'rs') to show them, or 'exit' to exit.\n\n");
    PrintStr("> Choice: ");

    if (!ReadLine(input, sizeof(input))) {
      break;
    }

    if (StringEqualsIgnoreCase(input, "exit")) {
      ExitProcess(0);
    }

    bool remove = false;
    if (StringEqualsIgnoreCase(input, "remove") || StringEqualsIgnoreCase(input, "rm")) {
      remove = true;
    } else if (StringEqualsIgnoreCase(input, "restore") || StringEqualsIgnoreCase(input, "rs")) {
      remove = false;
    } else {
      PrintStr("\nInvalid Input!\n");
      Sleep(1500);
      ClearScreen();
      continue;
    }

    if (ToggleArrows(remove)) {
      while (true) {
        PrintStr("\nRegistry updated successfully!\n\n");
        PrintStr("> Would you like to restart Windows Explorer now to apply "
                 "changes? (y/n): ");

        if (!ReadLine(input, sizeof(input))) {
          ExitProcess(0);
        }

        if (StringEqualsIgnoreCase(input, "y")) {
          RestartExplorerUnelevated();
          if (!remove) {
            RemoveBlankIconFile(false);
          }
          ExitProcess(0);
        } else if (StringEqualsIgnoreCase(input, "n")) {
          PrintStr("\n> Changes will apply on next boot or when windows "
                   "explorer is restarted\n\n");
          PrintStr("> Press any key to close...\n");
          WaitKey();
          if (!remove) {
            RemoveBlankIconFile(true);
          }
          ExitProcess(0);
        } else {
          PrintStr("\nInvalid choice. Please type 'y' or 'n'.\n");
        }
      }
    } else {
      PrintStr("\nPress any key to close...\n");
      WaitKey();
      ExitProcess(1);
    }
  }
  ExitProcess(0);
}
