#include "WinUtility.h"
#include <QMessageBox>
#include <cstring>
#include <propkey.h>
#include <psapi.h>
#include <QScreen>
#include <QApplication>
#include <QSettings>
#include <QDir>
#include <QProcess>
#include <QDomDocument>
#include <QIcon>
#include <shlobj_core.h>
#include <appmodel.h>
#include <atlbase.h>
#include <ShlObj.h>
#include <propvarutil.h>
#include <QDirIterator>
#include <QtWin>

void Win::setAlwaysTop(HWND hwnd, bool isTop)
{
    SetWindowPos(hwnd, isTop ? HWND_TOPMOST : HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW); //持续置顶
}

void Win::jumpToTop(HWND hwnd)
{
    setAlwaysTop(hwnd, true);
    setAlwaysTop(hwnd, false);
}

bool Win::isInSameThread(HWND hwnd_1, HWND hwnd_2)
{
    if (hwnd_1 && hwnd_2)
        return GetWindowThreadProcessId(hwnd_1, NULL) == GetWindowThreadProcessId(hwnd_2, NULL);
    return false;
}

bool Win::isTopMost(HWND hwnd)
{
    if (hwnd == nullptr) return false;
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    return style & WS_EX_TOPMOST;
}

QString Win::getWindowText(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    static WCHAR text[128];
    GetWindowTextW(hwnd, text, _countof(text)); //sizeof(text)字节数256 内存溢出
    return QString::fromWCharArray(text);
}

void Win::miniAndShow(HWND hwnd)
{
    ShowWindow(hwnd, SW_MINIMIZE); //组合操作不要异步 要等前一步完成
    ShowWindow(hwnd, SW_NORMAL);
}

DWORD Win::getProcessID(HWND hwnd)
{
    DWORD PID = -1; //not NULL
    GetWindowThreadProcessId(hwnd, &PID);
    return PID;
}

QString Win::getProcessName(HWND hwnd)
{
    if (hwnd == nullptr) return QString();

    DWORD PID = getProcessID(hwnd);

    static WCHAR path[128];
    HANDLE Process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, PID);
    GetProcessImageFileNameW(Process, path, _countof(path)); //sizeof(path)字节数256 内存溢出
    CloseHandle(Process);

    QString pathS = QString::fromWCharArray(path);
    return pathS.mid(pathS.lastIndexOf('\\') + 1);
}

QString Win::getProcessExePath(HWND hwnd)
{
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);

    if (processId == 0)
        return "";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processId);
    if (hProcess == NULL) { // 游戏客户端和加速器可能做了保护，导致无法获取
        qDebug() << "#OpenProcess failed:" << GetLastError();
        return "";
    }

    TCHAR processName[MAX_PATH] = TEXT("<unknown>");
    // https://www.cnblogs.com/mooooonlight/p/14491399.html
    if (GetModuleFileNameEx(hProcess, NULL, processName, MAX_PATH)) {
        CloseHandle(hProcess);
        return QString::fromWCharArray(processName);
    }

    CloseHandle(hProcess);
    return "";
}

void Win::getInputFocus(HWND hwnd)
{
    HWND foreHwnd = GetForegroundWindow();
    DWORD foreTID = GetWindowThreadProcessId(foreHwnd, NULL);
    DWORD threadId = GetWindowThreadProcessId(hwnd, NULL); //GetCurrentThreadId(); //增加泛用性扩大到其他窗口
    if (foreHwnd == hwnd) {
        qDebug() << "#Already getFocus";
        return;
    }
    bool res = AttachThreadInput(threadId, foreTID, TRUE); //会导致短暂的Windows误认为this==QQ激活状态 导致点击任务栏图标 持续最小化（参见下方解决法案）
    qDebug() << "#Attach:" << res;
    if (res == false) { //如果遇到系统窗口而失败 只能最小化再激活获取焦点
        miniAndShow(hwnd);
    } else {
        SetForegroundWindow(foreHwnd); //刷新QQ任务栏图标状态 防止保持焦点状态 不更新 导致点击后 最小化 而非获取焦点
        SetForegroundWindow(hwnd);
        SetFocus(hwnd);
        AttachThreadInput(threadId, foreTID, FALSE);
    }
}

void Win::simulateKeyEvent(const QList<BYTE>& keys) //注意顺序：如 Ctrl+Shift+A 要同按下顺序相同
{
    for (auto key : keys) //按下
        keybd_event(key, 0, 0, 0);

    std::for_each(keys.rbegin(), keys.rend(), [=](BYTE key) { //释逆序放
        keybd_event(key, 0, KEYEVENTF_KEYUP, 0);
    });
}

bool Win::isForeWindow(HWND hwnd)
{
    return GetForegroundWindow() == hwnd;
}

QString Win::getWindowClass(HWND hwnd)
{
    WCHAR buffer[128]; // 不要使用static，防止获取ClassName失败后，返回上一次的值
    int sz = GetClassNameW(hwnd, buffer, _countof(buffer)); //sizeof字节数 会溢出
    if (sz == 0) return QString();
    return QString::fromWCharArray(buffer);
}

HWND Win::windowFromPoint(const QPoint& pos)
{
    return WindowFromPoint({pos.x(), pos.y()});
}

HWND Win::topWinFromPoint(const QPoint& pos)
{
    HWND hwnd = WindowFromPoint({pos.x(), pos.y()});
    while (GetParent(hwnd) != NULL)
        hwnd = GetParent(hwnd);
    return hwnd;
}

QRect Win::getClipCursor()
{
    RECT rect;
    GetClipCursor(&rect);
    return QRect(rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
}

bool Win::isCursorVisible()
{
    CURSORINFO info;
    info.cbSize = sizeof(CURSORINFO);
    GetCursorInfo(&info);
    return info.flags == CURSOR_SHOWING;
}

bool Win::isUnderCursor(HWND Hwnd)
{
    HWND hwnd = topWinFromPoint(QCursor::pos());
    return hwnd == Hwnd;
}

bool Win::isPowerOn()
{
    static SYSTEM_POWER_STATUS powerSatus;
    GetSystemPowerStatus(&powerSatus);
    return powerSatus.ACLineStatus;
}

bool Win::isForeFullScreen()
{
    QRect Screen = qApp->primaryScreen()->geometry();
    HWND Hwnd = GetForegroundWindow();

    HWND H_leftBottom = Win::topWinFromPoint(Screen.bottomLeft()); //获取左下角像素所属窗口，非全屏是任务栏
    if (Hwnd != H_leftBottom) return false;

    RECT Rect;
    GetWindowRect(Hwnd, &Rect);
    if (Rect.right - Rect.left >= Screen.width() && Rect.bottom - Rect.top >= Screen.height()) //确保窗口大小(二重验证)
        return true;
    return false;
}

void Win::setAutoRun(const QString& AppName, bool isAuto)
{
    static const QString Reg_AutoRun = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"; //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    static const QString AppPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    if (isAuto)
        reg.setValue(AppName, AppPath);
    else
        reg.remove(AppName);
}

bool Win::isAutoRun(const QString& AppName)
{
    static const QString Reg_AutoRun = "HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"; //HKEY_CURRENT_USER仅仅对当前用户有效，但不需要管理员权限
    static const QString AppPath = QDir::toNativeSeparators(QApplication::applicationFilePath());
    QSettings reg(Reg_AutoRun, QSettings::NativeFormat);
    return reg.value(AppName).toString() == AppPath;
}

void Win::adjustBrightness(bool isUp, int delta)
{
    QProcess process;
    process.setProgram("Powershell");
    process.setArguments(QStringList() << "-Command"
                                       << QString("&{$info = Get-Ciminstance -Namespace root/WMI -ClassName WmiMonitorBrightness;"
                                                  "$monitor = Get-WmiObject -ns root/wmi -class wmiMonitorBrightNessMethods;"
                                                  "$monitor.WmiSetBrightness(0,$info.CurrentBrightness %1 %2)}")
                                              .arg(isUp ? '+' : '-')
                                              .arg(delta));
    process.start();
    process.waitForFinished();
    qDebug() << "#Changed Brightness";
}

void Win::setBrightness(int brightness)
{
    if (brightness < 0) return;

    QProcess process;
    process.setProgram("Powershell");
    process.setArguments(QStringList() << "-Command"
                                       << QString("&{$info = Get-Ciminstance -Namespace root/WMI -ClassName WmiMonitorBrightness;"
                                                  "$monitor = Get-WmiObject -ns root/wmi -class wmiMonitorBrightNessMethods;"
                                                  "$monitor.WmiSetBrightness(0, %1)}")
                                              .arg(brightness));
    process.start();
    process.waitForFinished();
    qDebug() << "#Set Brightness:" << brightness;
}

WORD Win::registerHotKey(HWND hwnd, UINT modifiers, UINT key, QString str, ATOM* atom)
{
    *atom = GlobalAddAtomA(str.toStdString().c_str());
    ATOM HotKeyId = (*atom - 0xC000) + 1; //防止return 0
    bool bHotKey = RegisterHotKey(hwnd, HotKeyId, modifiers, key); //只会响应一个线程的热键，因为响应后，该消息被从队列中移除，无法发送给其他窗口
    qDebug() << "RegisterHotKey: " << *atom << HotKeyId << bHotKey;
    return bHotKey ? HotKeyId : 0;
}

bool Win::unregisterHotKey(ATOM atom, WORD hotKeyId, HWND hwnd)
{
    bool del = GlobalDeleteAtom(atom); //=0
    bool unReg = UnregisterHotKey(hwnd, hotKeyId); //!=0
    bool ret = !del && unReg;
    qDebug() << "UnregisterHotKey:" << ret;
    return ret;
}

bool Win::setScreenReflashRate(int rate)
{
    if (rate < 0) return false;
    if (rate == (int)getCurrentScreenReflashRate()) return true;

    /*QRes
    QProcess pro;
    pro.setProgram("QRes");
    pro.setArguments(QStringList() << QString("/r:%1").arg(rate));
    pro.start();
    pro.waitForFinished(8000);
    */
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDisplayFrequency = rate;
    lpDevMode.dmFields = DM_DISPLAYFREQUENCY;
    LONG ret = ChangeDisplaySettings(&lpDevMode, CDS_UPDATEREGISTRY); //&保存于注册表 如果使用0，会导致Apex时有可能重置回60HZ（注册表
    qDebug() << "#Change Screen Reflash Rate(API):" << rate << (ret == DISP_CHANGE_SUCCESSFUL) << ret;
    return (ret == DISP_CHANGE_SUCCESSFUL);
}

DWORD Win::getCurrentScreenReflashRate()
{
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDriverExtra = 0;
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &lpDevMode);
    return lpDevMode.dmDisplayFrequency;
}

QSet<DWORD> Win::getAvailableScreenReflashRates()
{
    DEVMODE lpDevMode;
    memset(&lpDevMode, 0, sizeof(DEVMODE));
    lpDevMode.dmSize = sizeof(DEVMODE);
    lpDevMode.dmDriverExtra = 0;
    int iModeNum = 0;
    QSet<DWORD> list;
    while (EnumDisplaySettings(NULL, iModeNum++, &lpDevMode)) //about 98 modes(not only 2 reflash rates)
        list << lpDevMode.dmDisplayFrequency;
    return list;
}

/// 测试全局光标形状
/// - 因为Qt的cursor::shape()只能检测本程序窗口
/// - ref: https://bbs.csdn.net/topics/392329000
bool Win::testGlobalCursorShape(LPCWSTR cursorID)
{
    CURSORINFO info;
    info.cbSize = sizeof(CURSORINFO); // 必需
    GetCursorInfo(&info);
    if (info.flags != CURSOR_SHOWING) return false; // 光标隐藏时，hCurosr为NULL
    // Windows API 没有直接获取光标形状的函数，只能通过比较光标句柄
    // LoadCursor [arg0:NULL] 意为加载预定义的系统游标
    // 原理：仅当游标资源尚未加载时，LoadCursor 函数才会加载游标资源; 否则，它将检索现有资源的句柄
    return info.hCursor == LoadCursor(NULL, cursorID);
}

// AUMID(App User Model Id): e.g. `Microsoft.WindowsSoundRecorder_8wekyb3d8bbwe!App`
QString Win::getUWPNameFromAUMID(const QString& AUMID)
{
    QString str = AUMID.split('!')[0];
    auto underscore = str.lastIndexOf('_');
    if (underscore == -1) return str;
    return str.left(underscore);
}

// 普通API很难获取UWP的图标，遂手动解析AppxManifest.xml
// ref: https://github.com/microsoft/PowerToys/blob/5b616c9eed776566e728ee6dd710eb706e73e300/src/modules/launcher/Plugins/Microsoft.Plugin.Program/Programs/UWPApplication.cs#L394
// 函数名：LogoUriFromManifest 说明是从AppxManifest.xml中获取Logo路径
// ref: https://learn.microsoft.com/zh-cn/windows/uwp/app-resources/images-tailored-for-scale-theme-contrast
// ref: https://stackoverflow.com/questions/39910625/how-to-properly-structure-uwp-app-icons-in-appxmanifest-xml-file-for-a-win32-app
QString Win::getLogoPathFromAppxManifest(const QString& manifestPath)
{
    QFile file(manifestPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开AppxManifest.xml文件" << manifestPath;
        return QString();
    }

    QDomDocument doc;
    if (!doc.setContent(&file)) {
        file.close();
        qDebug() << "无法解析XML文件";
        return QString();
    }
    file.close();

    QDomElement root = doc.documentElement();
    QDomElement properties = root.firstChildElement("Properties");
    QDomElement logo = properties.firstChildElement("Logo");

    if (!logo.isNull()) {
        return logo.text();
    }

    return QString();
}

// 匹配某个变体，如：StoreLogo.scale-200.png
QIcon Win::loadUWPLogo(const QString& logoPath)
{
    QFileInfo fileInfo(logoPath);
    QDir dir = fileInfo.absoluteDir();
    QString wildcard = fileInfo.baseName() + "*." + fileInfo.suffix();

    if (!dir.exists()) {
        qDebug() << "Directory does not exist!";
        return QIcon();
    }

    QStringList filters;
    filters << wildcard; // 例如 "StoreLogo*.png"

    QStringList matchingFiles = dir.entryList(filters, QDir::Files);

    if (!matchingFiles.isEmpty()) {
        QString logoFile = dir.absoluteFilePath(matchingFiles.first());
        return QIcon(logoFile);
    } else {
        qWarning() << "No matching files found!";
    }
    return QIcon();
}

// "{1AC14E77-02E7-4E5D-B744-2EB1AE5198B7}" to "C:\Windows\System32"
QString Win::GUID2Path(const QString& guid)
{
    GUID folderID;
    if (CLSIDFromString(guid.toStdWString().c_str(), &folderID) != NOERROR) {
        qWarning() << "Failed to convert GUID to CLSID.";
        return "";
    }

    PWSTR path = nullptr;
    HRESULT hr = SHGetKnownFolderPath(folderID, 0, nullptr, &path);

    QString folderPath;
    if (SUCCEEDED(hr)) {
        folderPath = QString::fromWCharArray(path);
    } else
        qWarning() << "Failed to get known folder path from:" << guid;
    CoTaskMemFree(path);

    return folderPath;
}

// 用于比较包版本的辅助函数
bool comparePackageFullNames(const wchar_t* a, const wchar_t* b) {
    PACKAGE_VERSION versionA, versionB;
    UINT32 lengthA = 0, lengthB = 0;

    // Get appropriate length
    if (SUCCEEDED(PackageIdFromFullName(a, PACKAGE_INFORMATION_BASIC, &lengthA, nullptr)) &&
        SUCCEEDED(PackageIdFromFullName(b, PACKAGE_INFORMATION_BASIC, &lengthB, nullptr))) {
        std::vector<BYTE> bufferA(lengthA), bufferB(lengthB);
        PACKAGE_ID* idA = reinterpret_cast<PACKAGE_ID*>(bufferA.data());
        PACKAGE_ID* idB = reinterpret_cast<PACKAGE_ID*>(bufferB.data());

        if (SUCCEEDED(PackageIdFromFullName(a, PACKAGE_INFORMATION_BASIC, &lengthA, bufferA.data())) &&
            SUCCEEDED(PackageIdFromFullName(b, PACKAGE_INFORMATION_BASIC, &lengthB, bufferB.data()))) {
            versionA = idA->version;
            versionB = idB->version;

            if (versionA.Major != versionB.Major) return versionA.Major > versionB.Major;
            if (versionA.Minor != versionB.Minor) return versionA.Minor > versionB.Minor;
            if (versionA.Build != versionB.Build) return versionA.Build > versionB.Build;
            return versionA.Revision > versionB.Revision;
        }
    }

    // 如果无法比较版本，则按字符串比较
    return wcscmp(a, b) > 0;
}

QString parsePackageFamilyName(const QString& AUMID)
{
    auto aumid = AUMID.toStdWString();
    UINT32 pfnLength = 0;
    UINT32 praidLength = 0;

    // 第一次调用获取需要的缓冲区大小
    LONG result = ParseApplicationUserModelId(aumid.c_str(), &pfnLength, nullptr, &praidLength, nullptr);

    QString packageFamilyName;
    if (result == ERROR_INSUFFICIENT_BUFFER) {
        wchar_t* _packageFamilyName = new wchar_t[pfnLength];
        wchar_t* _packageRelativeApplicationId = new wchar_t[praidLength];

        // 再次调用以填充缓冲区
        result = ParseApplicationUserModelId(aumid.c_str(), &pfnLength, _packageFamilyName, &praidLength, _packageRelativeApplicationId);

        if (result == ERROR_SUCCESS) {
            packageFamilyName = QString::fromWCharArray(_packageFamilyName);
        } else {
            qWarning() << "Error parsing AUMID: " << result;
        }

        delete[] _packageFamilyName;
        delete[] _packageRelativeApplicationId;
    } else {
        qWarning() << "Error getting buffer size: " << result;
    }

    return packageFamilyName;
}

// ref: https://www.cnblogs.com/xyycare/p/18265865/cpp-get-msix-lnk-location
QString Win::getUWPInstallDirByAUMID(const QString& AUMID)
{
    // 1. PackageManager (C++/WinRT) 的方法崩溃（可能需要管理员权限），遂弃之
    // 2. PowerShell + Get-AppxPackage 的方法不需要管理员权限，但速度较慢
    // "Get-AppxPackage -Name '%1' | Select-Object -ExpandProperty InstallLocation"

    QString installPath;
    // 不包含版本号
    // QString packageFamilyName = AUMID.split('!').first();
    QString packageFamilyName = parsePackageFamilyName(AUMID);

    UINT32 count = 0;
    UINT32 bufferLength = 0;
    // 先获取包的数量和缓冲区大小
    LONG result = GetPackagesByPackageFamily(packageFamilyName.toStdWString().c_str(), &count, nullptr, &bufferLength, nullptr);
    if (result == ERROR_INSUFFICIENT_BUFFER) {
        std::vector<PWSTR> fullNames(count);
        std::vector<wchar_t> buffer(bufferLength);
        // 获取包的全名（包含版本号 + 开发商）
        result = GetPackagesByPackageFamily(packageFamilyName.toStdWString().c_str(), &count, fullNames.data(), &bufferLength, buffer.data());

        if (result == ERROR_SUCCESS && count > 0) {
            // 按版本排序包名，最新版本在前
            std::sort(fullNames.begin(), fullNames.end(), comparePackageFullNames);

            for (UINT32 i = 0; i < count; ++i) {
                UINT32 pathLength = 0;
                // Get pathLength
                result = GetPackagePathByFullName(fullNames[i], &pathLength, nullptr);

                if (result == ERROR_INSUFFICIENT_BUFFER) {
                    std::vector<wchar_t> pathBuffer(pathLength);
                    result = GetPackagePathByFullName(fullNames[i], &pathLength, pathBuffer.data());

                    if (result == ERROR_SUCCESS) {
                        installPath = QString::fromWCharArray(pathBuffer.data());
                        break;  // 找到最新版本的包就退出
                    }
                }
            }
        }
    }

    if (installPath.isEmpty()) {
        qWarning() << "Failed to find package for AUMID:" << AUMID;
    }

    return installPath;
}

QString readPropertyStr(IPropertyStore* store, PROPERTYKEY pk)
{
    QString res;
    PROPVARIANT var;
    PropVariantInit(&var);
    auto hr = store->GetValue(pk, &var);
    if (SUCCEEDED(hr) && var.vt == VT_LPWSTR) {
        res = QString::fromWCharArray(var.pwszVal);
    }
    PropVariantClear(&var);
    return res;
}

// Only String properties
// 某些属性字符串没有对应的PKEY定义，只能遍历
QString getPropertyByPKString(IPropertyStore* store, const QString& property)
{
    static QMap<QString, PROPERTYKEY> pk_cache; // 缓存已知的属性

    if (pk_cache.contains(property))
        return readPropertyStr(store, pk_cache[property]);

    DWORD count = 0;
    store->GetCount(&count);
    for (DWORD i = 0; i < count; i++) {
        PROPERTYKEY pk;
        auto hr = store->GetAt(i, &pk);
        if (FAILED(hr)) {
            qWarning() << "Failed to get property key.";
            continue;
        }

        CComHeapPtr<wchar_t> pkName;
        PSGetNameFromPropertyKey(pk, &pkName);

        auto _property = QString::fromStdWString(pkName.m_pData);
        if (_property == property) {
            pk_cache[property] = pk;
            return readPropertyStr(store, pk);
        }
    }
    return "";
}

// 从AppsFolder过滤获取UWP列表
QList<QPair<QString, QString>> Win::getUWPList()
{
    auto co_init = CoInitialize(NULL);

    IShellItem* psi = nullptr;
    // PowerToys没有采用这种方法，应该是直接遍历StartMenu文件夹 & 枚举UWP（Get-StartApps、or WinRT API ？）
    HRESULT hr = SHCreateItemInKnownFolder(FOLDERID_AppsFolder, 0, nullptr, IID_PPV_ARGS(&psi));

    QList<QPair<QString, QString>> appList;

    if (SUCCEEDED(hr)) {
        IEnumShellItems* pEnum = nullptr;
        hr = psi->BindToHandler(nullptr, BHID_EnumItems, IID_PPV_ARGS(&pEnum));

        if (SUCCEEDED(hr)) {
            IShellItem* pChildItem = nullptr;
            while (pEnum->Next(1, &pChildItem, nullptr) == S_OK) {
                PWSTR _displayName = nullptr;
                PWSTR _relativePath = nullptr;
                auto hr_name = pChildItem->GetDisplayName(SIGDN_NORMALDISPLAY, &_displayName);
                auto hr_path = pChildItem->GetDisplayName(SIGDN_PARENTRELATIVEPARSING, &_relativePath);

                QString name, path;
                if (SUCCEEDED(hr_path) && SUCCEEDED(hr_name)) {
                    name = QString::fromWCharArray(_displayName);
                    path = APPS_FOLDER + QString::fromWCharArray(_relativePath);
                } else
                    qWarning() << "Failed to get display name or path.";

                // properties, ref: https://github.com/microsoft/PowerToys/blob/dca8b7ac3560a77a57202398dd7e1e68e6eb9006/src/modules/Workspaces/WorkspacesLib/AppUtils.cpp
                CComPtr<IPropertyStore> store;
                // 可以GetCount枚举所有属性
                hr = pChildItem->BindToHandler(NULL, BHID_PropertyStore, IID_PPV_ARGS(&store));
                if (FAILED(hr)) {
                    qWarning() << "Failed to bind to property store handler.";
                    continue;
                }

                // WARN: 对于非UWP的.lnk文件，貌似很难获取其自身绝对路径，只能获取目标路径，导致ICON获取困难，故改用遍历 StartMenu

                // PROPVARIANT var;
                // PropVariantInit(&var);

                // // 获取 System.Link.TargetParsingPath 属性 还可以获取 "System.AppUserModel.PackageInstallPath"
                // hr = store->GetValue(PKEY_Link_TargetParsingPath, &var);
                // if (SUCCEEDED(hr) && var.vt == VT_LPWSTR) {
                //     path = QString::fromWCharArray(var.pwszVal);
                // }
                // PropVariantClear(&var);

                // PropVariantInit(&var);
                // hr = store->GetValue(PKEY_Link_Arguments, &var);
                // if (SUCCEEDED(hr) && var.vt == VT_LPWSTR) {
                //     args = QString::fromWCharArray(var.pwszVal);
                // }
                // PropVariantClear(&var);

                auto isUWP = getPropertyByPKString(store, "System.AppUserModel.PackageFullName") != "";
                if (isUWP)
                    appList << qMakePair(name, path);

                CoTaskMemFree(_relativePath);
                CoTaskMemFree(_displayName);
                pChildItem->Release();
            }
            pEnum->Release();
        }
        psi->Release();
    }

    if (SUCCEEDED(co_init))
        CoUninitialize();

    return appList;
}

// name path args
QList<std::tuple<QString, QString>> Win::getAppList()
{
    QList<std::tuple<QString, QString>> appList;

    static auto getApps = [](const QString& path) -> QList<QPair<QString, QString>> {
        // qDebug() << "Searching in:" << path;
        QList<QPair<QString, QString>> apps;
        // QDir::System: on Windows, .lnk files are included, 不加的话某些.lnk扫不出来，例如桌面上的DeepL
        QDirIterator it(path, QStringList() << "*.lnk" << "*.url", QDir::System | QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (it.hasNext())
            apps << qMakePair(it.fileInfo().completeBaseName(), QDir::toNativeSeparators(it.next()));
        return apps;
    };

    auto list = getUWPList();

    decltype(list) linkList;
    linkList << getApps(getKnownFolderPath(CSIDL_COMMON_PROGRAMS));
    linkList << getApps(getKnownFolderPath(CSIDL_PROGRAMS));
    linkList << getApps(getKnownFolderPath(CSIDL_DESKTOP));

    // 去重
    QSet<QPair<QString, QString>> name_target_set;
    for (auto& link: linkList) {
        // TODO: 这里.symLinkTarget()获取得不太精准，桌面上的DeepL.lnk的target返回空，建议使用COM接口获取
        // 对于该文件，COM接口获取target也是空（但是args是正确的），怪，且该文件不用QDir::System扫不出来，aaa
        auto [target, args] = getShortcutInfo(link.second);
        auto pair = qMakePair(link.first.toLower(), target.toLower() + args);
        if (name_target_set.contains(pair)) {
            // qDebug() << "Skip duplicate link:" << link;
            continue;
        }
        name_target_set.insert(pair);
        list << link;
    }

    for (const auto& [name, path] : qAsConst(list))
        appList << std::make_tuple(name, path);


    return appList;
}

// SHGetFolderPath 的封装
QString Win::getKnownFolderPath(int csidl)
{
    TCHAR szPath[MAX_PATH]; // 文档里说用 MAX_PATH (260)
    SHGetFolderPath(nullptr, csidl, nullptr, SHGFP_TYPE_CURRENT, szPath);
    return QString::fromWCharArray(szPath);
}

// 获取快捷方式目标路径和参数，用QFileInfo不能获取args，而且对于某些文件，target也是空，哼
QPair<QString, QString> Win::getShortcutInfo(const QString& lnkPath)
{
    HRESULT hr;
    IShellLink* psl;
    IPersistFile* ppf;

    QString target, args;

    auto co_init = CoInitialize(NULL);
    if (SUCCEEDED(co_init)) {
        hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
        if (SUCCEEDED(hr)) {
            hr = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
            if (SUCCEEDED(hr)) {
                hr = ppf->Load(reinterpret_cast<LPCOLESTR>(lnkPath.utf16()), STGM_READ);
                if (SUCCEEDED(hr)) {
                    wchar_t szTargetPath[MAX_PATH];
                    hr = psl->GetPath(szTargetPath, MAX_PATH, NULL, SLGP_UNCPRIORITY);
                    if (SUCCEEDED(hr))
                        target = QString::fromWCharArray(szTargetPath);

                    wchar_t szArgs[MAX_PATH];
                    hr = psl->GetArguments(szArgs, MAX_PATH);
                    if (SUCCEEDED(hr))
                        args = QString::fromWCharArray(szArgs);
                }
                ppf->Release();
            }
            psl->Release();
        }
        if (SUCCEEDED(co_init))
            CoUninitialize();
    }

    return qMakePair(target, args);
}

// .url (Text File)
// - return (url, icon)
QPair<QString, QString> Win::parseInternetShortcut(const QString& urlPath)
{
    if (!urlPath.endsWith(".url")) {
        qWarning() << "Not a .url file.";
        return {};
    }

    QFile file(urlPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Failed to open file:" << urlPath;
        return {};  // 如果文件打不开，返回空结果
    }

    QTextStream stream(&file);
    QString url;
    QString iconPath;

    // 遍历 .url 文件内容，提取 URL 和 IconFile
    while (!stream.atEnd()) {
        QString line = stream.readLine();
        if (line.startsWith("URL=", Qt::CaseInsensitive)) {
            url = line.mid(4);  // 提取 URL
        }
        if (line.startsWith("IconFile=", Qt::CaseInsensitive)) {
            iconPath = line.mid(9);  // 提取图标路径
        }
    }
    file.close();

    if (url.isEmpty())
        qWarning() << "No URL found in .url file." << urlPath;

    return qMakePair(url, iconPath);
}

// QFileIconProvider::icon()只能在主线程调用，不能后台缓存，非常坑
QIcon Win::getFileIcon(QString filePath, UINT sizeHint) {
    filePath = QDir::toNativeSeparators(filePath); // IMPORTANT: 否则会找不到文件

    // S_FALSE，说明本线程已经初始化过
    auto co_init = CoInitialize(NULL); // important for SHGetFileInfo，否则失败
    SHFILEINFO sfi;
    memset(&sfi, 0, sizeof(SHFILEINFO));

    QIcon icon;
    if (SHGetFileInfo(filePath.toStdWString().c_str(), NULL, &sfi, sizeof(SHFILEINFO), SHGFI_ICON | sizeHint | SHGFI_ADDOVERLAYS)) {
        icon = QtWin::fromHICON(sfi.hIcon);
        DestroyIcon(sfi.hIcon);
    }
    if (SUCCEEDED(co_init)) // 每次成功调用 CoInitialize 或 CoInitializeEx（包括返回S_FALSE）都必须通过对 CoUninitialize 的相应调用来平衡
        CoUninitialize(); // 频繁调用会耗时（大概0.05ms）

    return icon;
}
