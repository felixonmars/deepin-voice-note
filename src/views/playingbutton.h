#ifndef PLAYINGBUTTON_H
#define PLAYINGBUTTON_H

#include <QWidget>
#include <DImageButton>
#include <QVBoxLayout>

DWIDGET_USE_NAMESPACE

//录音回放按钮
class PlayingButton : public QWidget
{
    Q_OBJECT

public:
    PlayingButton(QWidget *parent = 0);

signals:
    void pause();
    void resume();
    void stop();

public slots:
    void handlePause();
    void handleResume();
    void handleStop();

private:
    DImageButton *pauseButton;
    DImageButton *resumeButton;
    QVBoxLayout *layout;
};

#endif // PLAYINGBUTTON_H
