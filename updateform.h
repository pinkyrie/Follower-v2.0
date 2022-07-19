#ifndef UPDATEFORM_H
#define UPDATEFORM_H

#include <QWidget>
#include <QDialog>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QSslSocket>
#include <QtDebug>
#include <functional>
#include "systemapi.h"
namespace Ui {
class UpdateForm;
}

class UpdateForm : public QDialog {
    Q_OBJECT

public:
    explicit UpdateForm(QWidget *parent = nullptr);
    ~UpdateForm();

    QUrl getRedirectTarget(QNetworkReply* reply);
    void download(const QUrl& url, const QString& path, std::function<void(bool)> todo);
    void getDownloadUrl(const QString& htmlUrl, std::function<void(const QString&, const QString&)> todo);
    void parseHtml(const QString& html, QString& url, QString& version);
    void updateLatestGiteeRelease(const QString& releaseUrl, const QString& filePath);
    void writeBat(const QString& batPath, const QString& zipPath, const QString& folder);

private:
    Ui::UpdateForm *ui;
    QNetworkAccessManager* manager = nullptr;
    const QString releaseUrl = "https://gitee.com/mrbeanc/follower-v2.0/releases";

    const QString ver = "v2.4.0";

    // QWidget interface
protected:
    void showEvent(QShowEvent* event) override;
};

#endif // UPDATEFORM_H
