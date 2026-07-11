#include <windows.h>
#include <cstdio>  // Легковесная библиотека для вывода текста (printf, fprintf)
#include <cstring> // Легковесная библиотека для сравнения и копирования строк (strcmp, strncpy)
#include <cstdlib> // Библиотека для конвертации (atoi, atof) и работы с памятью (malloc, free)

int main(int argc, char* argv[]) {
    // Установка стандартных значений (default values)
    const char* NAME =     "Monitor Refresh Change";
    const char* VERSION =  "0.2.5";
    int monitor_index = 1; // второй монитор по умолчанию
    int freq1 = 50;
    int freq2 = 60;
    double delay_seconds = 2.0;

    // Флаги для отслеживания того, что именно пользователь ввел в консоли (argument tracking)
    bool f1_specified = false;
    bool f2_specified = false;
    
    // Всегда выводим название и версию при старте
    printf("%s v%s\n\n", NAME, VERSION);  

    // ПРОВЕРКА: Если программа запущена БЕЗ ключей (argc == 1)
    if (argc == 1) {
        // 1. Выводим справку по использованию флагов
        printf("Usage: monitor_hz.exe [options]\n");
        printf("Options:\n");
        printf("  -m <index>   Monitor index (0 for primary, 1 for secondary, etc. Default: 1)\n");
        printf("  -f1 <hz>     First refresh rate frequency in Hz (Default: 50)\n");
        printf("  -f2 <hz>     Second refresh rate frequency in Hz (Default: 60)\n");
        printf("  -d <seconds> Delay between switches in seconds (Default: 2.0)\n\n");

        // 2. Сканируем и выводим список всех мониторов в системе
        printf("Available Monitors in the system:\n");
        printf("--------------------------------------------------\n");

        DISPLAY_DEVICE list_dd;
        int index = 0;

        // Цикл перебора всех видео-адаптеров в системе
        while (true) {
            SecureZeroMemory(&list_dd, sizeof(list_dd));
            list_dd.cb = sizeof(list_dd);
			char monitor_friendly_name[250] = "";			

            if (!EnumDisplayDevices(NULL, index, &list_dd, 0)) {
                break;
            }

            if (list_dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) {
                DEVMODE list_dm;
                SecureZeroMemory(&list_dm, sizeof(list_dm));
                list_dm.dmSize = sizeof(list_dm);

                if (EnumDisplaySettings(list_dd.DeviceName, ENUM_CURRENT_SETTINGS, &list_dm)) {
                    printf("Monitor Index: %d\n", index);
                    printf("  System Name:  %s\n", list_dd.DeviceName);  
                    printf("  GPU Model:    %s\n", list_dd.DeviceString); 

                    bool found_friendly_name = false;

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

        return 0;
    }

    // Парсинг аргументов командной строки (работает, только если переданы ключи)
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            printf("Usage: monitor_hz.exe [options]\n");
            printf("Options:\n");
            printf("  -m <index>   Monitor index (Default: 1)\n");
            printf("  -f1 <hz>     First refresh rate (Default: 50)\n");
            printf("  -f2 <hz>     Second refresh rate (Default: 60)\n");
            printf("  -d <seconds> Delay in seconds (Default: 2.0)\n");
            return 0;
        }
        else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            monitor_index = atoi(argv[++i]);
        }
        else if (strcmp(argv[i], "-f1") == 0 && i + 1 < argc) {
            freq1 = atoi(argv[++i]);
            f1_specified = true; // Фиксируем, что первая частота задана вручную
        }
        else if (strcmp(argv[i], "-f2") == 0 && i + 1 < argc) {
            freq2 = atoi(argv[++i]);
            f2_specified = true; // Фиксируем, что вторая частота задана вручную
        }
        else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay_seconds = atof(argv[++i]);
        }
    }

    // Блок вывода текущих рабочих параметров
    printf("Running with parameters:\n");
    printf(" -> Monitor Index: %d\n", monitor_index);
    printf(" -> First Frequency: %d Hz\n", freq1);
    
    // Выводим инфу о второй частоте и задержке только если не включен однократный режим
    if (!(f1_specified && !f2_specified)) {
        printf(" -> Second Frequency: %d Hz\n", freq2);
        printf(" -> Delay: %.2f seconds\n", delay_seconds);
    }
    printf("\n");

    // Шаг 1: Инициализация структуры устройства отображения
    DISPLAY_DEVICE dd;
    SecureZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    if (!EnumDisplayDevices(NULL, monitor_index, &dd, 0)) {
        fprintf(stderr, "Error: Could not find monitor with index %d.\n", monitor_index);
        return 1;
    }

    printf("Found target monitor: %s\n", dd.DeviceName);

    // Шаг 2: Получение структуры текущих настроек экрана
    DEVMODE dm;
    SecureZeroMemory(&dm, sizeof(dm));
    dm.dmSize = sizeof(dm);

    if (!EnumDisplaySettings(dd.DeviceName, ENUM_CURRENT_SETTINGS, &dm)) {
        fprintf(stderr, "Error: Could not retrieve display settings.\n");
        return 1;
    }
	
	// Если не заданы частоты пишем об этом и выходим.
    if (!f1_specified && !f2_specified) {
        printf("\nFrequency not set. Use -f1 <hz> to set it.\n", monitor_index); 
        return 1;
    }	

    // Шаг 3: Переключение на первую частоту
    printf("Switching refresh rate to %d Hz...\n", freq1);
    dm.dmDisplayFrequency = freq1;
    dm.dmFields = DM_DISPLAYFREQUENCY;

    LONG result = ChangeDisplaySettingsEx(dd.DeviceName, &dm, NULL, 0, NULL);
    if (result == DISP_CHANGE_SUCCESSFUL) {
        printf("Successfully changed to %d Hz.\n", freq1);
    } else {
        fprintf(stderr, "Failed to change to %d Hz. Error code: %ld\n", freq1, result);
        return 1;
    }

    // НОВАЯ ЛОГИКА: Если была задана ТОЛЬКО первая частота (без второй), 
    // то прерываем выполнение и выходим из программы без ожидания (early exit)
    if (f1_specified && !f2_specified) {
        printf("Single frequency mode active. Exiting successfully.\n");
        return 0;
    }

    // Шаг 4: Ожидание (выполняется, только если нужны оба переключения)
    printf("Waiting for %.2f seconds...\n", delay_seconds);
    DWORD delay_ms = static_cast<DWORD>(delay_seconds * 1000.0);
    Sleep(delay_ms); 

    // Шаг 5: Переключение на вторую частоту
    printf("Switching refresh rate to %d Hz...\n", freq2);
    dm.dmDisplayFrequency = freq2;
    dm.dmFields = DM_DISPLAYFREQUENCY;

    result = ChangeDisplaySettingsEx(dd.DeviceName, &dm, NULL, 0, NULL);
    if (result == DISP_CHANGE_SUCCESSFUL) {
        printf("Successfully changed to %d Hz.\n", freq2);
    } else {
        fprintf(stderr, "Failed to change to %d Hz. Error code: %ld\n", freq2, result);
        return 1;
    }

    return 0;
}