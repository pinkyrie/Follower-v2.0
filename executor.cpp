#include "executor.h"
#include <QDebug>
#include <QDesktopServices>
#include <QJSEngine>
#include <QRegExp>
#include <QTextStream>
#include <QUrl>
#include <windows.h>
#include <QMessageBox>
#include <QtConcurrent>
#include "Utils/util.h"
#include <Utils/WinUtility.h>

Executor::Executor(QObject* parent)
    : QObject(parent)
{
    //QDir().mkdir(dirPath);
    //cmdEditor = new CmdEditor(cmdListPath);
    //noteEditor = new NoteEditor(noteListPath);
    readCmdList();

    connect(qApp, &QApplication::aboutToQuit, this, [=]() { //write
        QFile file(runTimesDataPath);
        if (!file.open(QFile::WriteOnly)) {
            QMessageBox::critical(nullptr, "File Write Error", "Cannot open \"runTimesMap.dat\"");
            return;
        }
        QDataStream data(&file);
        data << runTimesMap;
        qDebug() << "#Write runTimesMap.dat";
    });

    qRegisterMetaType<QList<Command>>("QList<Command>");
    connect(this, &Executor::appListChanged, this, [=](QList<Command> list) { // 信号与槽，线程安全，无需加锁
        appList = list;
        qDebug() << "#Updated AppList";
    });

    if (QFile::exists(runTimesDataPath)) {
        QFile file(runTimesDataPath); //read
        if (!file.open(QFile::ReadOnly)) {
            QMessageBox::critical(nullptr, "File Read Error", "Cannot open \"runTimesMap.dat\"");
            return;
        }
        QDataStream data(&file);
        data >> runTimesMap;
        qDebug() << "#Read runTimesMap.dat";
    }
}

void Executor::readCmdList()
{
    if (sys->cmdEditor == nullptr) return;
    CmdEditor::TableList list = sys->cmdEditor->getContentList();
    cmdList.clear();
    for (QStringList& line : list)
        cmdList.append({line.at(0), line.at(1), QDir::toNativeSeparators(line.at(2)), line.at(3)});
    std::sort(cmdList.begin(), cmdList.end(), [](const Command& a, const Command& b) {
        return a.code.length() < b.code.length();
    });
}

// ShellExecute
void Executor::runApp(const QString& filename, const QString& parameter, bool runAsAdmin, int nShowMode)
{
    if (filename == "") return; //""代表本工作目录
    int pos = filename.lastIndexOf('\\');
    QString dirPath = filename.left(pos); //缺省目录，防止找不到缺省文件//if pos == -1 return entire string
    qDebug() << "#run:" << filename << parameter;
    // The default verb is used, if available. If not, the "open" verb is used. If neither verb is available, the system uses the first verb listed in the registry.
    ShellExecuteW(0, runAsAdmin ? L"runas" : NULL, filename.toStdWString().c_str(), parameter.toStdWString().c_str(), dirPath.toStdWString().c_str(), nShowMode);
    // 对于.lnk，如果使用"open"，则会在控制台输出很多shell信息，造成卡顿
    //宽字符(Unicode)才能完美转换，否则可能编码错误
    //神奇现象：ShellExecute会新开线程，不返回虽然阻塞 但是事件循环继续进行
}

void Executor::editCmd()
{ //hide
    emit askHide();
    sys->cmdEditor->exec();
    readCmdList();
    sys->sysTray->showMessage("Update", "CMD List has [Updated]");
    emit askShow();
}

void Executor::editInputM()
{
    emit askHide();
    InputMethodEditor inputMEditor(inputMethodListPath);
    inputMEditor.exec();
    sys->inputM->readListFile(); //更新
    sys->sysTray->showMessage("Update", "InputMethod List has [Updated]");
    emit askShow();
}

void Executor::editNote()
{
    emit askHide();
    sys->noteEditor->exec();
    emit askShow();
}
// TODO: 加入对描述信息的匹配，以及打分排序
bool Executor::isMatch(const QString& dst, const QString& str, Qt::CaseSensitivity cs) //以空格分割，乱序关键字匹配
{
    if (isBlank(str)) return false;
    if (symbol(dst) != symbol(str)) return false; //不同级别命令不比较
    //if ((str.at(0) == '#' && dst.at(0) != '#') || (str.at(0) != '#' && dst.at(0) == '#')) return false; //加入匹配size 符号类别二次匹配 getsymbol!!!!!!!!!!!!!!!!!!!
    bool extra = str.endsWith(' '); //(如："qt "->"qt"：false 以保证"qt code"快捷匹配)
    static const QRegExp reg("\\W+"); //匹配至少一个[非字母数字]
    QStringList dstList = dst.simplified().split(reg, Qt::SkipEmptyParts);
    QStringList strList = str.simplified().split(reg, Qt::SkipEmptyParts);

    if (symbol(dst) == Inner_Cmd) dstList.push_front(InnerMark); //复原上一步清除的'#'
    if (symbol(str) == Inner_Cmd) strList.push_front(InnerMark);

    if (strList.empty()) return false;
    if (dstList.size() < strList.size() + extra) return false; //' '顶一个size

    for (const QString& _str : qAsConst(strList)) { //存在多次匹配同一个单词问题(不过问题不大)
        bool flag = false;
        for (const QString& _dst : qAsConst(dstList)) {
            if (_dst.indexOf(_str, 0, cs) == 0) {
                flag = true;
                break;
            }
        }
        if (!flag) return false;
    }
    return true;
}

bool Executor::isBlank(const QString& str)
{
    return str.simplified() == "";
}

Executor::CmdSymbol Executor::symbol(const QString& str)
{
    QChar ch = str.simplified().at(0);
    if (ch == InnerMark)
        return Inner_Cmd;
    else if (ch == JsMark)
        return Js_Cmd;
    else if (ch == TransMark)
        return Trans_Cmd;
    else
        return Normal_Cmd;
}

void Executor::clearText()
{
    echoText.clear();
}

QString Executor::cleanPath(QString path)
{
    return path.replace('\"', "");
}

bool Executor::isExistPath(const QString& str)
{
    static auto isAbsolutePath = [](const QString& str)->bool {
        static QRegularExpression regex("^[A-Za-z]:"); //no need for '\\' or '/' ; for fast input D:
        return regex.match(str).hasMatch();
    };
    QString _str = cleanPath(str);
    return QFileInfo::exists(_str) && isAbsolutePath(_str); //增加绝对路径判断，否则可能查询系统目录（如 Windows\System32 (\ja)）
}

void Executor::updateAppList()
{
    QtConcurrent::run([=](){
        static QSet<QString> lastLaunchCmdSet;
        QSet<QString> launchCmdSet;
        for (const Command& cmd : qAsConst(cmdList))
            launchCmdSet << (cmd.path.toLower() + cmd.param);

        QList<Command> list;
        static QList<std::tuple<QString, QString>> lastApps;
        auto apps = Win::getAppList();
        if (launchCmdSet == lastLaunchCmdSet && apps == lastApps) {
            qDebug() << "#AppList & cmdList Unchanged";
            return;
        }
        lastLaunchCmdSet = launchCmdSet;
        lastApps = apps;

        for (const auto& app : qAsConst(apps)) {
            auto [name, path] = app;
            if (QFileInfo(path).isShortcut()) { // .lnk, not include .url
                auto [target, args] = Win::getShortcutInfo(path);
                auto exeCmd = target.toLower() + args;
                // 和自定义命令去重
                if ((!exeCmd.isEmpty() && launchCmdSet.contains(exeCmd)) || launchCmdSet.contains(path.toLower())) {
                    qDebug() << "#Duplicated:" << name;
                    continue;
                }
            }
            list.append({name, "", path, ""});
        }
        emit appListChanged(list);
    });

}

Executor::State Executor::run(const QString& code, bool asAdmin, bool isWithExtra)
{
    clearText();
    if (code.isEmpty()) return NOCODE;
    if (code == OmitMark) return NOCODE;

    if (symbol(code) == Js_Cmd) {
        QString body = code.simplified().mid(1);
        echoText = JsEngine.evaluate(body).toString();
        return JSCODE;
    } else if (symbol(code) == Trans_Cmd) {
        QString body = code.simplified().mid(1);
        echoText = body.simplified();
        return TRANSLATE;
    }

    if (Util::maybePath(code)) { // 路径和命令的匹配是互斥的，疑似路径后，不再匹配命令，因为UI不会再显示命令候选
        if (isExistPath(code)) {
            runApp(code); // 对于路径，不能用runas运行，否则没反应，必须得用open或NULL(默认)
            return PATH;
        }
        return NOCODE;
    }

    auto apps = this->appList;
    auto iter_app = std::find_if(apps.begin(), apps.end(), [=](const Command& cmd) {
        return isMatch(cmd.code, code);
    });
    auto iter = std::find_if(cmdList.begin(), cmdList.end(), [=](const Command& cmd) { //模糊匹配
        QString sor = cmd.code;
        if (isWithExtra) sor += cmd.extra; //加上extra匹配
        return isMatch(sor, code); //cmd.code.indexOf(code, 0, Qt::CaseInsensitive) == 0
    });
    auto iter_inner = std::find_if(innerCmdList.begin(), innerCmdList.end(), [=](const InnerCommand& cmd) {
        return isMatch(cmd.code, code); //cmd.code.indexOf(code, 0, Qt::CaseInsensitive) == 0
    });

    bool isFind = (iter != cmdList.end());
    bool isFind_app = (iter_app != apps.end());
    bool isFind_inner = (iter_inner != innerCmdList.end());

    if (isFind) {
        Command cmd = *iter;
        runApp(cmd.path, cmd.param, asAdmin);
        // runTimesMap[cmd.filename + cmd.parameter]++; //统计运行次数，filename+param作为唯一标识
        //qDebug() << runTimesMap;
        return CODE;
    } else if (isFind_app) {
        Command cmd = *iter_app;
        runApp(cmd.path, cmd.param, asAdmin);
        // runTimesMap[cmd.filename + cmd.parameter]++;
        return CODE;
    } else if (isFind_inner) {
        InnerCommand cmd = *iter_inner;
        //(this->*(cmd.pfunc))();
        cmd.func(this); //指定对象
        return INNERCODE;
    } else { //最后的尝试//对人类最后的求爱
        runApp("cmd.exe", "/k " + code, asAdmin); //加上"/c"(close)，不然命令不会执行// "/k"(keep)也行
        return CMD;
    }
}

QList<QPair<QString, QString>> Executor::matchString(const QString& str, State* state, int limit, Qt::CaseSensitivity cs) //
{
    clearText();
    if (state) *state = NOCODE;
    QList<QPair<QString, QString>> list;
    if (str.isEmpty()) return list;

    if (symbol(str) == Js_Cmd) {
        echoText = "Inputing JavaScript Code...";
        if (state) *state = JSCODE;
        return list;
    }

    if (symbol(str) == Trans_Cmd) {
        echoText = "Inputing Sth. to Translate...";
        if (state) *state = TRANSLATE;
        return list;
    }

    if (isExistPath(str)) {
        echoText = "Maybe a Path...";
        if (state) *state = PATH;
        return list;
    }

    QMap<QString, Command> codeFile;
    for (const Command& cmd : qAsConst(cmdList))
        if (isMatch(cmd.code, str, cs)) { //忽略大小写//cmd.code.indexOf(str, 0, Qt::CaseInsensitive) == 0)
            list << qMakePair(cmd.code + cmd.extra, cmd.path);
            codeFile[cmd.code + cmd.extra + cmd.path] = cmd; //indexing
        }

    for (const Command& cmd : qAsConst(appList))
        if (isMatch(cmd.code, str, cs)) {
            list << qMakePair(cmd.code + cmd.extra, cmd.path);
            codeFile[cmd.code + cmd.extra + cmd.path] = cmd;
        }

    //无需去重 可能出现同名不同path等 让用户选择
    // if (!list.empty()) {
    //     std::sort(list.begin(), list.end(), [=, &codeFile](const QPair<QString, QString>& a, const QPair<QString, QString>& b) -> bool {
    //         const Command& ca = codeFile[a.first + a.second];
    //         const Command& cb = codeFile[b.first + b.second];
    //         if (ca.code.compare(str, cs) == 0) //全匹配优先级最高
    //             return true;
    //         else if (cb.code.compare(str, cs) == 0)
    //             return false;
    //         else //降序
    //             return runTimesMap.value(ca.filename + ca.parameter) > runTimesMap.value(cb.filename + cb.parameter);
    //     });
    // }

    QSet<QString> codeSet;
    for (const InnerCommand& cmd : qAsConst(innerCmdList))
        if (isMatch(cmd.code, str, cs)) { //忽略大小写
            QString str = cmd.showCode.isEmpty() ? cmd.code : cmd.showCode;
            if (!codeSet.contains(str)) { //去重
                codeSet << str;
                list << qMakePair(str, QString());
            }
        }

    if (state) *state = CODE; //统一标识INNER & CODE 在此不好做区分

    if (list.size() > limit) {
        list = list.mid(0, limit);
        list << qMakePair(OmitMark, QString(""));
    }

    return list;
}

bool Executor::hasText()
{
    return !echoText.isEmpty();
}

QString Executor::text()
{
    return echoText;
}

QList<Command> Executor::getCMDList()
{
    return cmdList;
}
