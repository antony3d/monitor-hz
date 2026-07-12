#include <windows.h>
#include <cstdio>  // Легковесная библиотека для вывода текста (printf, fprintf)
#include <cstring> // Легковесная библиотека для сравнения и копирования строк (strcmp, strncpy)
#include <cstdlib> // Библиотека для конвертации (atoi, atof) и работы с памятью (malloc, free)

void print_help() {
    printf("Usage: monitor_hz.exe [options]\n");
    printf("Options:\n");
    printf("  -m <index> [index2]  Monitor index(es) (Default: 0)\n");
    printf("  -f <hz1> [hz2]       Refresh rate(s) in Hz (Default: 50 60)\n");
    printf("  -d <seconds>         Delay between switches in seconds (Default: 2.0)\n\n");
}

// ============================================================================
// 2. ФУНКЦИЯ СКАНИРОВАНИЯ МОНИТОРОВ (Monitor Enumeration Function)
// Перебирает все активные дисплеи в системе, получает их системные имена,
// модели видеокарт, понятные пользователю имена (Friendly Name) и текущий режим.
// ============================================================================
void list_monitors() {
    printf("Available Monitors in the system:\n");
    printf("--------------------------------------------------\n");

    DISPLAY_DEVICE list_dd;
    int index = 0;

    // Цикл перебора всех видео-адаптеров в системе (display devices enumeration)
    while (true) {
        SecureZeroMemory(&list_dd, sizeof(list_dd));
        list_dd.cb = sizeof(list_dd);
        char monitor_friendly_name[250] = "";

        // Если адаптер по данному индексу не найден, выходим из цикла
        if (!EnumDisplayDevices(NULL, index, &list_dd, 0)) {
            break;
        }

        // Проверяем, подключен ли конкретный адаптер к рабочему столу
        if (list_dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
            DEVMODE list_dm;
            SecureZeroMemory(&list_dm, sizeof(list_dm));
            list_dm.dmSize = sizeof(list_dm);

            // Получаем текущие настройки дисплея (разрешение и частоту)
            if (EnumDisplaySettings(list_dd.DeviceName, ENUM_CURRENT_SETTINGS, &list_dm)) {
                printf("Monitor Index: %d\n", index);
                printf("  System Name:  %s\n", list_dd.DeviceName);
                printf("  GPU Model:    %s\n", list_dd.DeviceString);

                bool found_friendly_name = false;

                // Использование Advanced CCD API для получения реального "маркетингового" имени монитора
                UINT32 numPathIdentifiers = 0, numModeIdentifiers = 0;
                if (GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS, &numPathIdentifiers, &numModeIdentifiers) == ERROR_SUCCESS) {
                    DISPLAYCONFIG_PATH_INFO* pathArray = (DISPLAYCONFIG_PATH_INFO*)malloc(sizeof(DISPLAYCONFIG_PATH_INFO) * numPathIdentifiers);
                    DISPLAYCONFIG_MODE_INFO* modeArray = (DISPLAYCONFIG_MODE_INFO*)malloc(sizeof(DISPLAYCONFIG_MODE_INFO) * numModeIdentifiers);

                    if (pathArray && modeArray) {
                        if (QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS, &numPathIdentifiers, pathArray, &numModeIdentifiers, modeArray, NULL) == ERROR_SUCCESS) {
                            for (UINT32 p = 0; p < numPathIdentifiers; ++p) {
                                DISPLAYCONFIG_SOURCE_DEVICE_NAME sourceName;
                                SecureZeroMemory(&sourceName, sizeof(sourceName));
                                sourceName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME;
                                sourceName.header.size = sizeof(sourceName);
                                sourceName.header.adapterId = pathArray[p].sourceInfo.adapterId;
                                sourceName.header.id = pathArray[p].sourceInfo.id;

                                if (DisplayConfigGetDeviceInfo(&sourceName.header) == ERROR_SUCCESS) {
                                    char currentGdiName[32];
                                    WideCharToMultiByte(CP_ACP, 0, sourceName.viewGdiDeviceName, -1, currentGdiName, sizeof(currentGdiName), NULL, NULL);

                                    // Если имя совпадает с текущим GDI-именем устройства
                                    if (strcmp(currentGdiName, list_dd.DeviceName) == 0) {
                                        DISPLAYCONFIG_TARGET_DEVICE_NAME targetName;
                                        SecureZeroMemory(&targetName, sizeof(targetName));
                                        targetName.header.type = DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME;
                                        targetName.header.size = sizeof(targetName);
                                        targetName.header.adapterId = pathArray[p].targetInfo.adapterId;
                                        targetName.header.id = pathArray[p].targetInfo.id;

                                        if (DisplayConfigGetDeviceInfo(&targetName.header) == ERROR_SUCCESS) {
                                            if (targetName.monitorFriendlyDeviceName[0] != L'\0') {
                                                WideCharToMultiByte(CP_ACP, 0, targetName.monitorFriendlyDeviceName, -1, monitor_friendly_name, sizeof(monitor_friendly_name), NULL, NULL);
                                                found_friendly_name = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    free(pathArray);
                    free(modeArray);
                }

                // Резервный метод (Fallback), если CCD API не вернуло friendly name монитора
                if (!found_friendly_name) {
                    DISPLAY_DEVICE monitor_dd;
                    SecureZeroMemory(&monitor_dd, sizeof(monitor_dd));
                    monitor_dd.cb = sizeof(monitor_dd);
                    if (EnumDisplayDevices(list_dd.DeviceName, 0, &monitor_dd, 0)) {
                        strncpy(monitor_friendly_name, monitor_dd.DeviceString, sizeof(monitor_friendly_name) - 1);
                    } else {
                        strncpy(monitor_friendly_name, "Generic PnP Monitor", sizeof(monitor_friendly_name) - 1);
                    }
                }

                printf("  Monitor Name: %s\n", monitor_friendly_name);
                printf("  Current Mode: %ldx%ld @ %ld Hz\n",
                       list_dm.dmPelsWidth,
                       list_dm.dmPelsHeight,
                       list_dm.dmDisplayFrequency);
                printf("--------------------------------------------------\n");
            }
        }
        index++;
    }
}

// ============================================================================
// 3. ФУНКЦИЯ ИЗМЕНЕНИЯ ЧАСТОТЫ ОБНОВЛЕНИЯ (Display Switch Execution Function)
// Ищет монитор по индексу, считывает его текущие низкоуровневые настройки DEVMODE,
// подменяет поле частоты dmDisplayFrequency и динамически применяет изменения.
// ============================================================================
bool switch_monitor(int idx, int target_hz) {
    DISPLAY_DEVICE dd;
    SecureZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    // Проверяем существование монитора по его числовому индексу
    if (!EnumDisplayDevices(NULL, idx, &dd, 0)) {
        fprintf(stderr, "Error: Could not find monitor with index %d.\n", idx);
        return false;
    }
    printf("Found target monitor: %s\n", dd.DeviceName);

    DEVMODE dm;
    SecureZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    // Подгружаем структуру текущих экранных настроек целевого монитора
    if (!EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
        fprintf(stderr, "Error: Could not retrieve display settings for monitor %d.\n", idx);
        return false;
    }

    printf("Switching refresh rate to %d Hz...\n", target_hz);
    dm.dmDisplayFrequency = target_hz;          // Задаем новую частоту развертки
    dm.dmFields = DM_DISPLAYFREQUENCY;           // Указываем Windows, что меняется именно флаг частоты

    // Пробуем применить изменения динамически через WinAPI функцию ChangeDisplaySettingsEx
    LONG result = ChangeDisplaySettingsEx(dd.DeviceName, &dm, NULL, 0, NULL);
    if (result == DISP_CHANGE_SUCCESSFUL) {
        printf("Successfully changed to %d Hz.\n", target_hz);
        return true;
    } else {
        fprintf(stderr, "Failed to change to %d Hz on monitor %d. Error code: %ld\n", target_hz, idx, result);
        return false;
    }
}

// ============================================================================
// MAIN ОРКЕСТРАТОР (Основная точка входа в программу)
// ============================================================================
int main(int argc, char* argv[]) {
    // Установка стандартных значений (default values)
    const char* NAME =     "Monitor Refresh Change";
    const char* VERSION =  "0.2.7";
	const char* AUTHOR = "(C) antony3d | https://github.com/antony3d/monitor-hz";
    int monitor_index = 0;
    int monitor_index2 = -1;
    int freq1 = 50;
    int freq2 = 60;
    double delay_seconds = 2.0;

    // Флаги для отслеживания того, что именно пользователь ввел в консоли (argument tracking)
    bool f_specified = false;
    bool f2_specified = false;
    bool m2_specified = false;

    // Всегда выводим название и версию при старте (application branding)
    printf("%s v%s\n", NAME, VERSION);
	printf("%s\n", AUTHOR);

    // ПРОВЕРКА: Если программа запущена БЕЗ ключей (argc == 1)
    if (argc == 1) {
        print_help();      // Вызываем функцию справки
        list_monitors();   // Вызываем независимую функцию сканирования системы
        return 0;          // Успешный выход из программы
    }

    // Парсинг аргументов командной строки (работает, только если переданы ключи)
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_help();
            return 0;
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            monitor_index = atoi(argv[++i]);
            // Проверяем, является ли следующий аргумент вторым индексом монитора (без префикса '-')
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                monitor_index2 = atoi(argv[++i]);
                m2_specified = true;
            }
        }
        else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            freq1 = atoi(argv[++i]);
            f_specified = true;
            // Проверяем, является ли следующий аргумент также частотой (без префикса '-')
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                freq2 = atoi(argv[++i]);
                f2_specified = true;
            }
        }
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay_seconds = atof(argv[++i]);
        }
    }

    // Если не заданы частоты — выходим.
    if (!f_specified) {
        printf("\nFrequency not set. Use -f <hz> to set it.\n\n");
		print_help();
        return 1;
    }

    // Определяем режим работы (operation mode):
    //   - два монитора  (m2_specified)     → мульти-монитор, без задержки
    //   - один монитор + две частоты       → последовательное переключение с задержкой
    //   - один монитор + одна частота      → единственное переключение
    bool multi_monitor = m2_specified;

    // Блок вывода текущих рабочих параметров (runtime configuration print)
    printf("Running with parameters:\n");
    if (multi_monitor) {
        printf(" -> Monitor 1 Index: %d @ %d Hz\n", monitor_index, freq1);
        printf(" -> Monitor 2 Index: %d @ %d Hz\n", monitor_index2, f2_specified ? freq2 : freq1);
    } else {
        printf(" -> Monitor Index: %d\n", monitor_index);
        printf(" -> First Frequency: %d Hz\n", freq1);
        if (f2_specified) {
            printf(" -> Second Frequency: %d Hz\n", freq2);
            printf(" -> Delay: %.2f seconds\n", delay_seconds);
        }
    }
    printf("\n");

    // --- Режим 1: два монитора (Dual-monitor mode) ---
    if (multi_monitor) {
        if (!switch_monitor(monitor_index, freq1)) return 1;
        int Hz_for_second = f2_specified ? freq2 : freq1;
        if (!switch_monitor(monitor_index2, Hz_for_second)) return 1;
        return 0;
    }

    // --- Режим 2: один монитор, одна частота (Single switch mode) ---
    if (!f2_specified) {
        if (!switch_monitor(monitor_index, freq1)) return 1;
        printf("Single frequency mode active. Exiting successfully.\n");
        return 0;
    }

    // --- Режим 3: один монитор, две частоты с задержкой (Ping-pong delay mode) ---
    if (!switch_monitor(monitor_index, freq1)) return 1;

    printf("Waiting for %.2f seconds...\n", delay_seconds);
    DWORD delay_ms = static_cast<DWORD>(delay_seconds * 1000.0);
    Sleep(delay_ms);

    if (!switch_monitor(monitor_index, freq2)) return 1;

    return 0;
}