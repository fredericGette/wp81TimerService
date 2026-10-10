# wp81TimerService

A lightweight Win32 service for **Windows Phone 8.1** that executes a configurable command on a fixed interval (default: 5 minutes).

---

## Features

- Runs any script or executable on a configurable interval (default 5 minutes, minimum 1 minute)
- Task command and interval configurable at runtime via the registry — no recompilation needed
- Kills hung child processes automatically before the next interval begins
- Captures stdout/stderr from the task and writes them to the log file
- File-based structured logging (disabled by default; enabled via registry)

---

## Installation

![Example](Capture001.PNG)

**Install the service:**

```bat
wp81TimerService.exe install
```

**Set the task command via registry** (optional — falls back to the compiled-in default):

```bat
reg add "HKLM\SYSTEM\CurrentControlSet\Services\wp81TimerService\Parameters" ^
    /v TaskCommand /t REG_SZ /d "C:\Scripts\mytask.bat" /f
```

**Set the timer interval** (optional — default is 5 minutes, minimum is 1 minute):

```bat
reg add "HKLM\SYSTEM\CurrentControlSet\Services\wp81TimerService\Parameters" ^
    /v IntervalMinutes /t REG_DWORD /d 10 /f
```

**Enable file logging** (disabled by default):

```bat
reg add "HKLM\SYSTEM\CurrentControlSet\Services\wp81TimerService\Parameters" ^
    /v LogFile /t REG_SZ /d "C:\Logs\wp81TimerService.log" /f
```

**Start the service:**

```bat
sc start wp81TimerService
```

**Uninstall:**

```bat
sc stop wp81TimerService
wp81TimerService.exe uninstall
```

---

## Configuration

| Setting | Default | How to change |
|---|---|---|
| Task command | `ping -n 1 -w 1000 192.168.1.1` | Registry value `TaskCommand` (REG_SZ) under `...\Services\wp81TimerService\Parameters` |
| Timer interval | 5 minutes | Registry value `IntervalMinutes` (REG_DWORD) under `...\Services\wp81TimerService\Parameters` — minimum 1 |
| Log file path | *(disabled)* | Registry value `LogFile` (REG_SZ) under `...\Services\wp81TimerService\Parameters` |

`TaskCommand` and `LogFile` are read on every invocation of the task, so changes take effect at the next scheduled run without restarting the service. `IntervalMinutes` is read once at service start; restart the service to apply a new interval.

---

## Logging

When a `LogFile` registry value is set, the service appends UTF-8 lines in the format:

```
YYYY-MM-DD HH:MM [LEVEL] message
```

Log levels: `CRIT`, `ERROR`, `WARN`, `INFO`, `VERB`.

stdout and stderr from the child process are captured after it exits and logged as `[stdout]` / `[stderr]` lines at INFO / ERROR level respectively.

---

## Deployment

- [Install a telnet server on the phone](https://github.com/fredericGette/wp81documentation/tree/main/telnetOverUsb#readme), in order to run the application.  
- Manually copy the executable from the root of this GitHub repository to the shared folder of the phone.
> [!NOTE]
> When you connect your phone with a USB cable, this folder is visible in the Explorer of your computer. And in the phone, this folder is mounted in `C:\Data\USERS\Public\Documents`
