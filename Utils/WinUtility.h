#ifndef WIN_H
#define WIN_H
#include <QPoint>
#include <QString>
#include <windows.h>
#include <QRect>
//#include <QAudioDeviceInfo>
#include <QDebug>

/* Warning:
 * 由于STA套件所有的COM组件代码都运行于主STA（第一个调用CoInitialize函数的线程），
 * 如果你的主线程没有调用CoInitialize，那么第一个调用CoInitialize的工作线程就会成为主STA，
 * 而工作线程随时可能中止，这种情况下，一旦工作线程中止主STA也就不复存在了，
 * 因此你需要在主线程中调用CoInitialize初始化主STA，即使主线程不使用任何COM组件。
 *
 * -- https://www.cnblogs.com/manors/archive/2010/05/17/COM_Initialize_STA_MTA.html
 *
 * 应用程序中第一个使用 0 调用CoInitialize （或使用 COINIT_APARTMENTTHREADED 调用CoInitializeEx ）的线程必须是最后一个调用CoUninitialize的线程。
 * 否则，后续在 STA 上调用CoInitialize将失败，并且应用程序将无法工作。
 *
 * -- https://learn.microsoft.com/en-us/windows/win32/api/objbase/nf-objbase-coinitialize
*/
// 每个线程都要单独初始化COM
class ComInitializer { // RAII
public:
    ComInitializer() {
        hr = CoInitialize(NULL);
    }
    ~ComInitializer() {
        // 调用 CoInitializeEx 的第一个线程必须是调用 CoUninitialize 的最后一个线程。 否则，对 STA 上的 CoInitialize 的后续调用将失败，应用程序将不起作用。
        // 每次成功调用 CoInitialize 或 CoInitializeEx（包括返回S_FALSE）都必须通过对 CoUninitialize 的相应调用来平衡
        if (SUCCEEDED(hr)) {
            CoUninitialize();
        }
    }
private:
    HRESULT hr;
};

class Win //Windows API
{
public:
    Win() = delete;
    static void setAlwaysTop(HWND hwnd, bool isTop = true);
    static void jumpToTop(HWND hwnd);
    static bool isInSameThread(HWND hwnd_1, HWND hwnd_2);
    static bool isTopMost(HWND hwnd);
    static QString getWindowText(HWND hwnd);
    static void miniAndShow(HWND hwnd); //最小化然后弹出以获取焦点
    static DWORD getProcessID(HWND hwnd);
    static QString getProcessName(HWND hwnd);
    static QString getProcessExePath(HWND hwnd);
    static void getInputFocus(HWND hwnd);
    static void simulateKeyEvent(const QList<BYTE>& keys);
    static bool isForeWindow(HWND hwnd);
    static QString getWindowClass(HWND hwnd);
    static HWND windowFromPoint(const QPoint& pos);
    static HWND topWinFromPoint(const QPoint& pos);
    static QRect getClipCursor(void);
    static bool isCursorVisible(void);
    static bool isUnderCursor(HWND Hwnd);
    static bool isPowerOn(void);
    static bool isForeFullScreen(void);
    static void setAutoRun(const QString& AppName, bool isAuto);
    static bool isAutoRun(const QString& AppName);
    static void adjustBrightness(bool isUp, int delta = 10);
    static void setBrightness(int brightness);
    static WORD registerHotKey(HWND hwnd, UINT modifiers, UINT key, QString str, ATOM* atom);
    static bool unregisterHotKey(ATOM atom, WORD hotKeyId, HWND hwnd);
    static bool setScreenReflashRate(int rate);
    static DWORD getCurrentScreenReflashRate(void);
    static QSet<DWORD> getAvailableScreenReflashRates(void);
    static bool testGlobalCursorShape(LPCWSTR cursorID);
    static QString getUWPInstallDirByName(const QString& appName);
    Q_DECL_DEPRECATED_X("Use parsePackageFamilyName instead")
    static QString getUWPNameFromAUMID(const QString& AUMID);
    static QString getLogoPathFromAppxManifest(const QString& manifestPath);
    static QIcon loadUWPLogo(const QString& logoPath);
    static QString GUID2Path(const QString& guid);
    static QString getUWPInstallDirByAUMID(const QString& AUMID);
    static QList<QPair<QString, QString>> getUWPList(void);
    static QString getLocalizedFileName(const QString &filePath);
    static QList<std::tuple<QString, QString>> getAppList(void);
    static QString getKnownFolderPath(int csidl);
    static QPair<QString, QString> getShortcutInfo(const QString& lnkPath);
    static QPair<QString, QString> parseInternetShortcut(const QString& urlPath);
    static QIcon getFileIcon(QString filePath, UINT sizeHint = SHGFI_SMALLICON);

    // C++17 inline
    inline static const QString APPS_FOLDER = "shell:AppsFolder\\";
};

#endif // WIN_H
