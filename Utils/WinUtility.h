#ifndef WIN_H
#define WIN_H
#include <QPoint>
#include <QString>
#include <windows.h>
#include <QRect>
//#include <QAudioDeviceInfo>
#include <QDebug>

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
    static QList<std::tuple<QString, QString, QString>> getAppsFolderList(void);

    // C++17 inline
    inline static const QString APPS_FOLDER = "shell:AppsFolder\\";
};

#endif // WIN_H
