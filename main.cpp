#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QPlainTextEdit>
#include <QLabel>
#include <QProcess>
#include <QFontDatabase>
#include <QTextCursor>
#include <QDir>
#include <QIcon>

class CommandRunner : public QWidget {
    Q_OBJECT
public:
    CommandRunner(QWidget* parent = nullptr) : QWidget(parent) {
        setWindowTitle("命令执行器");
        resize(820, 520);

        auto* layout = new QVBoxLayout(this);

        //顶部：命令输入和按钮
        auto* top = new QHBoxLayout;
        cmdEdit = new QLineEdit(this);
        cmdEdit->setPlaceholderText("输入命令，例如: ls -l   或   dir");
        runBtn = new QPushButton("执行", this);
        stopBtn = new QPushButton("停止", this);
        stopBtn->setEnabled(false);
        top->addWidget(new QLabel("命令:", this));
        top->addWidget(cmdEdit, 1);
        top->addWidget(runBtn);
        top->addWidget(stopBtn);
        layout->addLayout(top);

        //输出
        output = new QPlainTextEdit(this);
        output->setReadOnly(true);
        output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        layout->addWidget(output, 1);

        status = new QLabel("就绪",this);
        layout->addWidget(status);

        //进程
        proc = new QProcess(this);
        connect(proc, &QProcess::readyReadStandardOutput, this, &CommandRunner::onStdout);
        connect(proc, &QProcess::readyReadStandardError, this, &CommandRunner::onStderr);
        connect(proc, &QProcess::finished, this, &CommandRunner::onFinished);
        connect(proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
            append("[进程错误] " + proc->errorString() + "\n");
            setBusy(false);
            });

        connect(runBtn, &QPushButton::clicked, this, &CommandRunner::run);
        connect(stopBtn, &QPushButton::clicked, this, &CommandRunner::stop);
        connect(cmdEdit, &QLineEdit::returnPressed, this, &CommandRunner::run);

        currentDir = QDir::currentPath();
    }

private slots:
    void run() {
        if (proc->state() != QProcess::NotRunning) return;
        const QString cmd = cmdEdit->text().trimmed();
        if (cmd.isEmpty()) return;

        append("> " + cmd + "\n");

        if (cmd.startsWith("cd ") || cmd == "cd") {
            QString target = cmd.mid(2).trimmed();
            target.remove('"'); 

            if (target.isEmpty()) {
                currentDir = QDir::currentPath();
            }
            else {
                QDir dir(currentDir);
                if (dir.cd(target)) {
                    currentDir = dir.absolutePath();
                }
                else {
                    append("[错误] 找不到路径: " + target + "\n");
                    cmdEdit->clear();
                    return;
                }
            }
            append("当前目录: " + currentDir + "\n");
            cmdEdit->clear();
            return;
        }

        //执行普通命令
#ifdef Q_OS_WIN
        proc->setProgram("cmd.exe");
        proc->setArguments({ "/c", cmd });
#else
        proc->setProgram("/bin/sh");
        proc->setArguments({ "-c", cmd });
#endif
        proc->setWorkingDirectory(currentDir);

        proc->setProcessChannelMode(QProcess::SeparateChannels);
        proc->start();
        setBusy(true);
        status->setText("运行中…");
        cmdEdit->clear();
    }

    void stop() {
        if (proc->state() != QProcess::NotRunning) {
            proc->kill();
            append("\n[已终止]\n");
        }
    }

    void onStdout() { append(QString::fromLocal8Bit(proc->readAllStandardOutput())); }
    void onStderr() { append(QString::fromLocal8Bit(proc->readAllStandardError())); }

    void onFinished(int code, QProcess::ExitStatus st) {
        setBusy(false);
        if (st == QProcess::CrashExit)
            status->setText("已崩溃 / 被终止");
        else
            status->setText(QString("完成，退出码: %1").arg(code));
    }

private:
    void append(const QString& s) {
        output->moveCursor(QTextCursor::End);
        output->insertPlainText(s);
    }
    void setBusy(bool b) {
        runBtn->setEnabled(!b);
        stopBtn->setEnabled(b);
        cmdEdit->setEnabled(!b);
    }

    QLineEdit* cmdEdit;
    QPushButton* runBtn;
    QPushButton* stopBtn;
    QPlainTextEdit* output;
    QLabel* status;
    QProcess* proc;
    QString currentDir;
};

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/app.ico"));
    app.setWindowIcon(QIcon(":/app.png"));
    CommandRunner w;
    w.show();
    return app.exec();
}
#include "main.moc"