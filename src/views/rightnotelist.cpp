#include "rightnotelist.h"
#include "textnoteitem.h"
#include "voicenoteitem.h"
#include "intancer.h"
#include <QDebug>
#include <uiutil.h>
#include <DFileDialog>
#include <DToast>
#include <DApplication>
#include <DMessageBox>
#include <QMessageBox>
#include <QScrollBar>
#include <QStandardPaths>
#include <QFileInfo>
#include <DApplicationHelper>
#include <QPainter>


MMenu::MMenu(QWidget *parent)
{

    //this->setFixedSize(QSize(162,100));
}

MMenu::~MMenu()
{

}

void MMenu::leaveEvent(QEvent* event)
{
    emit sigMMenu();
    return QMenu::leaveEvent(event);
}
void MMenu::paintEvent(QPaintEvent *event)
{
    QMenu::paintEvent(event);
}


RightNoteList::RightNoteList(NoteController *noteController) : m_currPlayingItem(nullptr)
  , m_defaultTxtName("文本.TXT")
  //, m_defaultTxtPath("/home/deepin/Desktop")
  , m_defaultAudioName("语音.MP3")
  , m_defaultAudioPath("/home/deepin/Desktop")
  , m_arrowButtonPressed(false)
  , m_actionHoverd(false)
{
    duringTime = 0;
    m_Recodefinised = false;  //ynb 20191109
    m_IsSliderBarReleased = false; //ynb 20191109
    m_addTextBtn = nullptr;
    m_arrowMenu = nullptr;
    m_contextMenu = nullptr;
    m_saveAsAction = nullptr;
    m_delAction = nullptr;
    m_textClicked = false;
    m_textChanged = false;
    m_defaultTxtPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    Intancer::get_Intancer()->clearHeightForRightList();
    initUI();
    initConnection();
}

RightNoteList::~RightNoteList()
{
    Intancer::get_Intancer()->clearHeightForRightList();
}

void RightNoteList::createDArrowMenu()
{
    if(nullptr == m_contextMenu && nullptr == m_saveAsAction && nullptr == m_delAction)
    //if(nullptr == m_arrowMenu && nullptr == m_contextMenu && nullptr == m_saveAsAction && nullptr == m_delAction)
    {
        m_contextMenu = new MMenu;
        m_saveAsAction = new QAction(tr(NOTE_MENU_SAVE_AS_TXT),this);
        m_delAction = new QAction(tr(FOLDER_MENU_DELETE),this);
        m_contextMenu->addAction(m_saveAsAction);
        m_contextMenu->addAction(m_delAction);
        //m_contextMenu->setFixedSize(QSize(162,89));
        m_contextMenu->setVisible(false);

        connect(m_contextMenu, SIGNAL(sigMMenu()), this, SLOT(OnLeaveContentMenu()));
        connect(m_delAction, SIGNAL(triggered(bool)), this, SLOT(handleDelItem(bool)));
        connect(m_delAction, SIGNAL(hovered()), this, SLOT(OnActionHoverd()));
        connect(m_saveAsAction, SIGNAL(triggered(bool)), this, SLOT(handleSaveAsItem(bool)));
        connect(m_saveAsAction, SIGNAL(hovered()), this, SLOT(OnActionHoverd()));
    }
}

void RightNoteList::destroyDArrowMenu()
{
    if(nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    //if(nullptr != m_arrowMenu && nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    {
        disconnect(m_contextMenu, SIGNAL(sigMMenu()), this, SLOT(OnLeaveContentMenu()));
        disconnect(m_delAction, SIGNAL(triggered(bool)), this, SLOT(handleDelItem(bool)));
        disconnect(m_delAction, SIGNAL(hovered()), this, SLOT(OnActionHoverd()));
        disconnect(m_saveAsAction, SIGNAL(triggered(bool)), this, SLOT(handleSaveAsItem(bool)));
        disconnect(m_saveAsAction, SIGNAL(hovered()), this, SLOT(OnActionHoverd()));

        delete m_saveAsAction;
        m_saveAsAction = nullptr;

        delete m_delAction;
        m_delAction = nullptr;

        delete m_contextMenu;
        m_contextMenu = nullptr;

//        delete m_arrowMenu;
//        m_arrowMenu = nullptr;
    }
}

void RightNoteList::showDArrowMenu(int x, int y, NOTE_TYPE type)
{
    if(nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    //if(nullptr != m_arrowMenu && nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    {
        //m_arrowMenu->setVisible(true);
        m_contextMenu->setVisible(true);
        if (type == NOTE_TYPE::TEXT)
        {
            m_saveAsAction->setText(NOTE_MENU_SAVE_AS_TXT);
        }
        else
        {
            m_saveAsAction->setText(NOTE_MENU_SAVE_AS_MP3);
        }
        m_contextMenu->move(x - m_contextMenu->width()/2,y);
        //m_arrowMenu->move(x,y);
    }
}

void RightNoteList::hideDArrowMenu()
{
    if(nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    //if(nullptr != m_arrowMenu && nullptr != m_contextMenu && nullptr != m_saveAsAction && nullptr != m_delAction)
    {
        //m_arrowMenu->setVisible(false);
        m_contextMenu->setVisible(false);
        m_contextMenu->move(-500,-500);
        //m_arrowMenu->move(-500,-500);
    }
}

void RightNoteList::initUI()
{
    this->setFrameShape(QListWidget::NoFrame);
    this->resize(548,this->height());

    m_addTextBtn = nullptr;

    createDArrowMenu();

    m_delConfirmDialog = UiUtil::createChooseDialog(QString(""), QString(tr("您确定要删除这条记事项吗？")), nullptr, QString(tr("取消")), QString(tr("删除")));
    m_saveFileEndDialog = UiUtil::createConfirmDialog(QString(""), QString(tr("")), this);
    m_noticeNotExistDialog = UiUtil::createConfirmDialog(QString(""), QString(tr("该语音记事项已删除")), this);

    m_fileExistsDialog = new FileExistsDialog();
    this->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    audioPlayer = new QMediaPlayer(this);
    audioPlayer->setNotifyInterval(200);
    connect(audioPlayer, SIGNAL(stateChanged(QMediaPlayer::State)), this, SLOT(handlePlayingStateChanged(QMediaPlayer::State)));

    m_myslider = new MySlider(Qt::Horizontal, this);
    //m_TestSlider = new DSlider(Qt::Horizontal, this);
    //m_TestSlider->setVisible(false);
    //m_myslider = new MyCustomSlider(Qt::Horizontal, this);

    //m_myslider->setFixedSize(350, 70);
    //m_myslider->setHandleType(MySlider::HandleType::SharpHandler);
    m_myslider->setPageStep(SLIDER_PAGE_STEP);
    m_myslider->setGeometry(0, 0, 350, m_myslider->m_defaultHeight);
    m_myslider->hide();


//    m_addTextBtn->setNormalPic(":/image/add_text_btn.png");
//    m_addTextBtn->setHoverPic(":/image/add_text_btn.png");
//    m_addTextBtn->setPressPic(":/image/add_text_btn_press.png");

    qApp->installEventFilter(this);

}
void RightNoteList::initConnection()
{
    connect(audioPlayer, SIGNAL(positionChanged(qint64)), this, SLOT(handleAudioPositionChanged(qint64)));
    connect(m_delConfirmDialog, &DDialog::buttonClicked, this, &RightNoteList::handleDelDialogClicked);
    connect(m_delConfirmDialog, &DDialog::closed, this, &RightNoteList::handleCloseDialogClicked);
    connect(m_noticeNotExistDialog, &DDialog::buttonClicked, this, &RightNoteList::sig_checkCurPageVoiceForDelete);
    connect(m_noticeNotExistDialog, &DDialog::closed, this, &RightNoteList::sig_checkCurPageVoiceForDelete);

    connect(audioPlayer,SIGNAL(durationChanged(qint64)),this,SLOT(getduration(qint64))); //ynb 20191109
    connect(m_myslider, SIGNAL(sliderPressed()), this, SLOT(handleSliderPressed()));  //ynb 20191109
    connect(m_myslider, SIGNAL(sliderReleased()), this, SLOT(handleSliderReleased()));
    connect(this->verticalScrollBar(),SIGNAL(valueChanged(int)),this,SLOT(handleVScrollBarChanged(int)));
}

void RightNoteList::addWidgetItem(bool isAddByButton, NOTE note, QString searchKey)
{
    if(note.noteType == NOTE_TYPE::TEXT)
    {
        TextNoteItem *textItem = new TextNoteItem(note, m_noteController, searchKey);
        connect(textItem, SIGNAL(textEditClicked(NOTE)), this, SIGNAL(textEditClicked(NOTE)));
        connect(textItem, SIGNAL(textEditTrueClicked(NOTE)), this, SLOT(onCheckEditState(NOTE)));
        connect(textItem, SIGNAL(textEditClicked(NOTE)), this, SLOT(onCheckEditState(NOTE)));
        connect(textItem, SIGNAL(menuBtnClicked(QPoint, QPoint, QWidget *, NOTE)), this, SLOT(handleMenuBtnClicked(QPoint, QPoint, QWidget *, NOTE)));
        connect(textItem, SIGNAL(sig_menuBtnPressed()), this, SIGNAL(textEditClicked(NOTE)));
        connect(textItem, SIGNAL(sig_menuBtnReleased()), this, SIGNAL(textEditClicked(NOTE)));
        connect(textItem, SIGNAL(sig_menuBtnPressed()), this, SLOT(handleMenuBtnPressed()));
        connect(textItem, SIGNAL(sig_menuBtnReleased()), this, SLOT(handleMenuBtnReleased()));
        connect(textItem, SIGNAL(sig_TextEditNotEmpty(bool)), this, SLOT(onTextChangedFlagChange(bool)));
        connect(textItem, SIGNAL(sig_TextEditNotEmpty(bool)), this, SLOT(onAbleAddBtn(bool)));
        connect(textItem, SIGNAL(sig_TextEditNotEmpty(bool)), this, SIGNAL(sig_TextEditNotEmpty(bool)));
        connect(textItem, SIGNAL(sig_TextEditEmpty()), this, SLOT(onDisableAddBtn()));
        connect(textItem, SIGNAL(sig_TextEditEmpty()), this, SIGNAL(sig_TextEditEmpty()));
        connect(textItem, SIGNAL(sig_fouceOutAndEditEmpty(NOTE)), this, SLOT(onCallDelDialog(NOTE)));
        //connect(textItem, SIGNAL(sig_ItemTimeChanged(NOTE)), this, SLOT(onSortItemByTime(NOTE)));
        connect(textItem, SIGNAL(buttonClicled()), this, SLOT(onfouceOutAllTextItem()));
        //connect(this, SIGNAL(sigBoardPress()), textItem, SLOT(tryToFouceout()));




        QListWidgetItem *item=new QListWidgetItem();
        //QListWidgetItem *item=new QListWidgetItem(this);
        //qDebug() << "text item height: " << textItem->height();
        //item->setSizeHint(QSize(this->width(),92));
        //qDebug()<<"textItem width1:"<<textItem->width();
        //item->setSizeHint(QSize(this->width(),123));  //orig
        item->setSizeHint(QSize(this->width(),TEXTNOTE_HEIGHT));  //orig
        //qDebug()<<"textItem width2:"<<textItem->width();
        int count = this->count();
        this->insertItem(count - 1,item);
        this->setItemWidget(item, textItem);
        textItem->init();
        if(isAddByButton)
        {
            textItem->changeToEditMode();
			adjustWidgetItemWidth();
        }

        //textItem->changeToEditMode();
        //qDebug()<<"textItem width3:"<<textItem->width();
    }
    else if(note.noteType == NOTE_TYPE::VOICE){
        VoiceNoteItem *voiceItem = new VoiceNoteItem(note, m_noteController);
        connect(voiceItem, SIGNAL(menuBtnClicked(QPoint, QPoint, QWidget *, NOTE)), this, SLOT(handleMenuBtnClicked(QPoint, QPoint, QWidget *, NOTE)));
        connect(voiceItem, SIGNAL(sig_menuBtnPressed()), this, SLOT(handleMenuBtnPressed()));
        connect(voiceItem, SIGNAL(sig_menuBtnReleased()), this, SLOT(handleMenuBtnReleased()));


        connect(voiceItem, SIGNAL(pausePlayingSignal()), this, SLOT(pause()));
        connect(voiceItem, SIGNAL(resumePlayingSignal(VoiceNoteItem *, QString, QRect)), this, SLOT(play(VoiceNoteItem *, QString, QRect)));
        connect(voiceItem, SIGNAL(buttonClicled()), this, SLOT(onfouceOutAllTextItem()));
        connect(Intancer::get_Intancer(), SIGNAL(sigDisAbleReplay()), voiceItem, SLOT(setPlayDiseable()));
        connect(Intancer::get_Intancer(), SIGNAL(sigEnAbleReplay()), voiceItem, SLOT(setPlayEnable()));

        QListWidgetItem *item=new QListWidgetItem();
        item->setSizeHint(QSize(this->width(), 98));
        this->insertItem(this->count() - 1,item);
        this->setItemWidget(item, voiceItem);
        voiceItem->init();
    }
}

void RightNoteList::addAddTextBtn()
{
    if(nullptr == m_addTextBtn)
    {
        m_addTextBtn = new AddTextBtn();
        m_addTextBtn->init();
        connect(m_addTextBtn, SIGNAL(addTextItem()), this, SIGNAL(addTextItem()));
        connect(m_addTextBtn, SIGNAL(addTextItem()), this, SLOT(onDisableAddBtn()));
        QListWidgetItem *item=new QListWidgetItem();
        item->setSizeHint(QSize(this->width(),m_addTextBtn->height()));
        this->addItem(item);
        this->setItemWidget(item, m_addTextBtn);
    }
}

void RightNoteList::delAddTextBtn()
{
    if(nullptr != m_addTextBtn)
    {
        QListWidgetItem *item = takeItem(count() - 1);
        if(nullptr != item)
        {
            disconnect(m_addTextBtn, SIGNAL(addTextItem()), this, SIGNAL(addTextItem()));
            disconnect(m_addTextBtn, SIGNAL(addTextItem()), this, SLOT(onDisableAddBtn()));
            m_addTextBtn = (AddTextBtn*)itemWidget(item);
            delete m_addTextBtn;
            m_addTextBtn = nullptr;
        }
    }
}

void RightNoteList::listAddTextShow()
{
    if(nullptr != m_addTextBtn)
    {
        //addAddTextBtn();
        m_addTextBtn->setFixedHeight(ADDBUTTON_HEIGHT_HEIGHT);
//        if(this->count() > 0)
//        {
//            QListWidgetItem* pItem = item(this->count());
//            if(nullptr != pItem)
//            {
//                pItem->setHidden(false);
//                update();
//            }
//        }
        //m_addTextBtn->show();
        //m_addTextBtn->setVisible(true);
    }
}

void RightNoteList::listAddTextHide()
{
    if(nullptr != m_addTextBtn)
    {
        //delAddTextBtn();

        m_addTextBtn->setFixedHeight(0);

//        if(this->count() > 0)
//        {
//            QListWidgetItem* pItem = item(this->count());
//            if(nullptr != pItem)
//            {
//                pItem->setHidden(true);
//                update();
//            }
//        }
        //m_addTextBtn->hide();
        //m_addTextBtn->setVisible(false);
    }
}

void RightNoteList::fouceOutAllTextItem()
{
    int count = this->count();
    for(int i = 0; i < count - 1; i++)
    {
        TextNoteItem *pTextItem = (TextNoteItem *)this->itemWidget(this->item(i));
        if(TEXT == pTextItem->getType())
        {
            pTextItem->tryToFouceout();
        }
    }

//    if(m_textChanged && m_textClicked)
//    {
//        emit sigChangeCurFolderToTop();
//    }
//    m_textChanged = false;
//    m_textClicked = false;
}

void RightNoteList::delAllEmptyText()
{
    this->fouceOutAllTextItem();
//    for(int i = 0; i < this->count(); i++)
//    {

//    }
//m_noteController->deleteNote(m_currSelNote)
}

// start ynb 20191109
void RightNoteList::getDurtimgByRecodefinised(QString filepath)
{
    if(nullptr != audioPlayer)
    {
        m_Recodefinised = true;
        audioPlayer->setMedia(QUrl::fromLocalFile(filepath));
    }
}
// end ynb 20191109

bool RightNoteList::eventFilter(QObject *o, QEvent *e)
{
    if(0 == o->objectName().compare(QString("QMainWindowClassWindow")))
    {
        switch (e->type())
        {
            case QEvent::MouseButtonRelease:
            if((!m_arrowButtonPressed)&&(!m_actionHoverd))
            {
                hideDArrowMenu();
                m_actionHoverd = false;
                qDebug()<<"RightNoteList MouseButtonRelease hide";
            }
            //qDebug()<<"click filter";
            break;
            case QEvent::MouseButtonPress:
                //qDebug()<<"RightNoteList::MouseButtonPress";
                emit sigBoardPress();
            break;
        }
    }

    return DListWidget::eventFilter(o,e);
}

void RightNoteList::paintEvent(QPaintEvent *event)
{
    //qDebug()<<"RightNoteList::paintEvent";
}

void RightNoteList::resizeEvent(QResizeEvent * event)
{
    DListWidget::resizeEvent(event);
    adjustWidgetItemWidth();
    if(nullptr != m_currPlayingItem && nullptr != m_myslider)
    {
        QRect rect = m_currPlayingItem->getWaveRect();
        QPoint waveformPoint = m_currPlayingItem->mapTo(this, QPoint(rect.x(), rect.y()));
        m_myslider->setGeometry( waveformPoint.x() - m_myslider->getHandlerWidth() / 2, waveformPoint.y() - 27, rect.width() + m_myslider->getHandlerWidth(), m_myslider->m_defaultHeight);
        //m_TestSlider->setGeometry( waveformPoint.x(), waveformPoint.y() - 27, rect.width(), m_myslider->m_defaultHeight);
    }
}

void RightNoteList::keyPressEvent(QKeyEvent *k)
{
    if(k->key() == Qt::Key_Space)
    {
        qDebug()<<"Key_Space";
    }
}

void RightNoteList::handleMenuBtnClicked(QPoint menuArrowPointGlobal, QPoint menuArrowPointToItem, QWidget *textNoteItem, NOTE note)
{
    bool flag = Intancer::get_Intancer()->getTryToDelEmptyTextNote();
    //if(nullptr == m_arrowMenu)
    if(!flag)
    {
        if(!m_contextMenu->isVisible())
        //if(!m_arrowMenu->isVisible())
        {
            QPoint itemGlobalPoint = textNoteItem->mapTo(this, menuArrowPointToItem);
            m_currSelItem= this->itemAt(itemGlobalPoint);
            m_currSelNote = note;

            //如果音频文件不存在
            if(VOICE == note.noteType)
            {
                if(!UiUtil::checkFileExist(note.contentPath))
                {
                    if(nullptr != m_contextMenu)
                    {
                        m_saveAsAction->setDisabled(true);
                    }
                }
                else {
                    m_saveAsAction->setDisabled(false);
                }
            }
            else {
                m_saveAsAction->setDisabled(false);
            }

            showDArrowMenu(menuArrowPointGlobal.x(), menuArrowPointGlobal.y(),note.noteType);
            qDebug()<<"handleMenuBtnClicked show";
        }
        else {
            hideDArrowMenu();
    //        m_arrowMenu->setVisible(false);
    //        m_contextMenu->setVisible(false);
            m_actionHoverd = false;
            qDebug()<<"handleMenuBtnClicked hide";
        }
    }

}


void RightNoteList::handleDelItem(bool)
{
    m_delConfirmDialog->show();
    hideDArrowMenu();
//    m_arrowMenu->setVisible(false);
//    m_contextMenu->setVisible(false);
    m_actionHoverd = false;
//    return;
}

void RightNoteList::OnActionHoverd()
{
    m_actionHoverd = true;
}

void RightNoteList::OnLeaveContentMenu()
{
    m_actionHoverd = false;
    //handleVScrollBarChanged(-1);
}

void RightNoteList::handleVScrollBarChanged(int value)
{
    qDebug()<<"handleVScrollBarChanged:"<<value;
    if((nullptr != m_myslider)&&(nullptr != m_currPlayingItem))
    {
        if (!m_myslider->isHidden())
        {
            QRect rect;
            if(m_currPlayingItem->getwaveformPoint(rect))
            {
                QPoint waveformPoint = m_currPlayingItem->mapTo(this, QPoint(rect.x(), rect.y()));
                qDebug()<<"waveformPoint.y():"<<waveformPoint.y();
                m_myslider->move(m_myslider->x(),waveformPoint.y() - 27);
            }
        }
    }
}

void RightNoteList::onDisableAddBtn()
{
    if(nullptr != m_addTextBtn)
    {
        m_addTextBtn->setDisableBtn(true);
    }
}

void RightNoteList::onAbleAddBtn(bool changed)
{
    if(nullptr != m_addTextBtn)
    {
        qDebug()<<"m_addTextBtn->setDisableBtn(false);";
        m_addTextBtn->setDisableBtn(false);
    }
}

void RightNoteList::onTextChangedFlagChange(bool changed)
{
//    if(m_textClicked)
//    {
//        m_textChanged = changed;
//        //emit sigChangeCurFolderToTop();
//    }
}

void RightNoteList::onCallDelDialog(NOTE textNote)
{
    m_currSelNote = textNote;
    m_currSelItem = getListItemById(textNote.id);
    if(nullptr != m_currSelItem)
    {
        //m_delConfirmDialog->show();
        Intancer::get_Intancer()->setTryToDelEmptyTextNote(true);
        handleDelDialogClicked(1,"");
    }
}

void RightNoteList::onCheckEditState(NOTE note)
{
    int count = this->count();
    for(int i = 0; i < count - 1; i++)
    {
        TextNoteItem *pTextItem = (TextNoteItem *)this->itemWidget(this->item(i));
        if((note.id != pTextItem->getId()) && (note.noteType == pTextItem->getType()))
        {
            pTextItem->tryToFouceout();
        }
    }

//    m_textClicked = true;
//    if(m_textChanged && m_textClicked)
//    {
        emit sigChangeCurFolderToTop();
//    }
//    m_textChanged = false;
//    //m_textClicked = false;
}

void RightNoteList::onfouceOutAllTextItem()
{
    this->fouceOutAllTextItem();
}

//s ynbboy 20191109
void RightNoteList::getduration(qint64 position)
{
    if(m_Recodefinised)
    {
       m_Recodefinised = false;
       duringTime = position;
       emit positionByfinishRecord(position);
    }
}
//e ynbboy 20191109

void RightNoteList::handleSaveAsItem(bool)
{
    hideDArrowMenu();
//    m_arrowMenu->setVisible(false);
//    m_contextMenu->setVisible(false);
    m_actionHoverd = false;

    SAVE_INFO saveInfo;
    saveInfo.note = m_currSelNote;
    if (TEXT == m_currSelNote.noteType)
    {
        saveInfo.windowTitle = QString(tr("另存为TXT"));
        saveInfo.fileExtension = QString(".txt");
    }
    else
    {
        saveInfo.windowTitle = QString(tr("另存为MP3"));
        saveInfo.fileExtension = QString(".mp3");
    }
    showFileDialog(saveInfo);

}

void RightNoteList::showFileDialog(SAVE_INFO saveInfo)
{
    DFileDialog fileDialog(this);
        fileDialog.setWindowTitle(saveInfo.windowTitle);
        fileDialog.setDirectory(m_defaultTxtPath);
        fileDialog.setFileMode(DFileDialog::Directory);
        fileDialog.setAcceptMode(DFileDialog::AcceptSave);

        QDir dir(m_defaultTxtPath);
        QStringList files;
        QString newName;
        if (TEXT == m_currSelNote.noteType) {
            fileDialog.setDefaultSuffix("txt");
            fileDialog.setNameFilter(tr("TXT(*.txt)"));
            scanData(dir,".txt", files);
            getNewName(newName,files);
            fileDialog.selectFile("记事本" + newName);
        }else {
            fileDialog.setDefaultSuffix("mp3");
            fileDialog.setNameFilter(tr("MP3(*.mp3)"));
            scanData(dir,".mp3", files);
            getNewName(newName,files);
            fileDialog.selectFile("记事本" + newName);
        }

    //fileDialog->setFilter(QDir::filePath());
    if(fileDialog.exec() == QDialog::Accepted) {
        QString path = fileDialog.selectedFiles()[0];
//        QString fileName = fileDialog.getLineEditValue(tr("文件名"));
        QString fileName = QFileInfo(path).fileName();
//        QString filePath = path + '/' + fileName;
        QString filePath = path;
        if (fileName.isEmpty())
        {
            DMessageBox::information(this, tr(""), tr("文件名不能为空"));

        }
//        else if (!UiUtil::checkFileExtension(fileName, saveInfo.fileExtension))
//        {
//            DMessageBox::information(this, tr(""), tr("文件扩展名必须是") + saveInfo.fileExtension);
//        }
        else if(UiUtil::checkFileExist(filePath))
        {
            m_fileExistsDialog->setSavePath(filePath);
            m_fileExistsDialog->setNote(m_currSelNote);
            m_fileExistsDialog->show();
            //DDialog *fileExistDialog = UiUtil::createDialog(QString(""), QString(tr("文件名已存在，是否覆盖？")), nullptr, QString(tr("是")), QString(tr("否")));;
        }
        else
        {
            bool result = false;
            //filePath.append(saveInfo.fileExtension);
            if (VOICE == m_currSelNote.noteType)
            {
                result = UiUtil::saveMP3(UiUtil::getRecordingVoiceFullPath(m_currSelNote.contentPath), filePath);
            }
            else
            {
                result = UiUtil::saveTxt(filePath, m_currSelNote.contentText);
            }
            //handleSaveFileEnd(result);
        }

    } else {
       //QMessageBox::information(NULL, tr("Path"), tr("You didn't select any files."));
    }
}

void RightNoteList::scanData(const QDir &fromDir, const QString &filter, QStringList &files)
{
    QFileInfoList fileInfoList = fromDir.entryInfoList(QDir::Files);
    //QFileInfoList fileInfoList = fromDir.entryInfoList();
    files.clear();
    for(int i = 0; i < fileInfoList.count(); i++)
    {
        if(fileInfoList.at(i).fileName().endsWith(filter))
        {
            QString filename = fileInfoList.at(i).fileName();
            filename = filename.remove(filename.count() - filter.count(),filter.count());
            if(filename.count() > 3)
            {
                QString prefilename = filename;
                QString tmpfilename = filename.remove(3,filename.count() - 3);
                if(0 == tmpfilename.compare("记事本"))
                {
                    prefilename = prefilename.remove(0,3);
                    files.append(prefilename);
                }
            }
        }
    }
}

void RightNoteList::getNewName(QString &outName, QStringList files)
{
    int newName = 0;
    bool find = false;
    while(!find)
    {
        newName++;
        outName = QString::number(newName);
        bool hassame = false;
        for(int i = 0; i < files.count(); i++)
        {
            if(0 == files.at(i).compare(outName))
            {
                hassame = true;
                break;
            }
        }

        if(!hassame)
        {
            find = true;
        }
    }
}

QListWidgetItem *RightNoteList::getListItemById(int id)
{
    QListWidgetItem * tmpItem = nullptr;
    for(int i = 0; i < this->count(); i++)
    {
        tmpItem = this->item(i);
        TextNoteItem* pTextNoteItem = (TextNoteItem*)this->itemWidget(tmpItem);
        if(id == pTextNoteItem->getId())
        {
            break;
        }
    }
    return tmpItem;
}

void RightNoteList::handleDelDialogClicked(int index, const QString &text)
{
    if (index == 1)
    {
        //==== start add 20191105  bug2162
        NOTE_TYPE delType2 = m_currSelNote.noteType;
        if(TEXT == delType2)
        {
            if (!UiUtil::autoDeleteTxt(m_currSelNote))
            {
                qDebug() << "error: delete file error";
            }
        }

        //==== start add 20191105  bug2162
        if (m_noteController->deleteNote(m_currSelNote))
        {
            bool move = false;
            int moveMovment = 0;
            if(nullptr != m_currPlayingItem)
            {

                int delRow = -1;
                NOTE_TYPE delType = m_currSelNote.noteType;
                getRowByID(m_currSelNote.id,delType,delRow);
                int playRow = -1;
                NOTE_TYPE playType = VOICE;
                getRowByID(m_currPlayingItem->getNoteID(),playType,playRow);

                if(m_currPlayingItem->getNoteID() == m_currSelNote.id)
                {
                    audioPlayer->stop();
                }

                if(delRow < playRow)
                {
                    move = true;
                    moveMovment = this->itemWidget(m_currSelItem)->height();
                    qDebug()<<"moveMovment: "<<moveMovment;
                }

            }

            this->removeItemWidget(m_currSelItem);
            delete m_currSelItem;
            m_currSelItem = nullptr;
            m_addTextBtn->setDisableBtn(false);
            emit sig_TextEditNotEmpty(false);

            if(move)
            {
                changeSliderPosByHand(moveMovment);
            }

            if(VOICE == m_currSelNote.noteType)
            {
                Intancer::get_Intancer()->delHeightForRightList(VOICENOTE_HEIGHT);
            }
            else if(TEXT == m_currSelNote.noteType)
            {
                Intancer::get_Intancer()->delHeightForRightList(TEXTNOTE_HEIGHT);
            }

            //删除成功并且处于搜索状态下的话，再次出发搜索
            if(Intancer::get_Intancer()->getSearchingFlag())
            {
                emit sig_research();
            }

            emit sigChangeCurFolderToTop();

        }
        else {
            qDebug() << "error: delete item error";
        }
    }
    else {
        NOTE_TYPE delType = m_currSelNote.noteType;
        if(TEXT == delType)
        {
            TextNoteItem *pPreDelItem = (TextNoteItem*)this->itemWidget(m_currSelItem);
            pPreDelItem->changeToEditMode();
        }
    }
    Intancer::get_Intancer()->setTryToDelEmptyTextNote(false);
    //handleVScrollBarChanged(-1);
}

void RightNoteList::handleCloseDialogClicked()
{
    NOTE_TYPE delType = m_currSelNote.noteType;
    if(TEXT == delType)
    {
        TextNoteItem *pPreDelItem = (TextNoteItem*)this->itemWidget(m_currSelItem);
        pPreDelItem->changeToEditMode();
    }
    Intancer::get_Intancer()->setTryToDelEmptyTextNote(false);
}

void RightNoteList::handleClickRecordButton()
{
    // Must stop player before new record.
    if (QMediaPlayer::StoppedState !=audioPlayer->state())
    {
        //m_currPlayingItem->handleStopPlay();
        audioPlayer->stop();
        //audioPlayer->setMedia(QUrl::fromLocalFile("default"));//clear path
        //m_currPlayingItem = nullptr;
    }
}

void RightNoteList::play(VoiceNoteItem * voiceNoteItem, QString filepath, QRect waveformPos)
{
    if (filepath != getPlayingFilepath()) {
        audioPlayer->stop();

        m_currPlayingItem = voiceNoteItem;


        if(!UiUtil::checkFileExist(filepath))
        {
            m_myslider->hide();
            m_currPlayingItem->onlySetResumeNoSig();
            m_currPlayingItem->setPlayDiseable();
            m_currPlayingItem->clearWaveformContent();
            m_noticeNotExistDialog->show();
            return;
        }
        audioPlayer->setMedia(QUrl::fromLocalFile(filepath));
    }
    else {
        m_currPlayingItem = voiceNoteItem;
        if(!UiUtil::checkFileExist(filepath))
        {
            m_myslider->hide();
            return;
        }
    }

    connect(m_myslider,SIGNAL(sigSliderPos(int)),m_currPlayingItem->m_waveform,SLOT(OnSetWavePos(int)));

    //waveform->show();


    audioPlayer->play();
    if (m_myslider->isHidden())
    {
        QPoint waveformPoint = voiceNoteItem->mapTo(this, QPoint(waveformPos.x(), waveformPos.y()));
        //QPoint x = voiceNoteItem->mapTo(this, QPoint(waveformPos.x(), waveformPos.y()));
//        qDebug() << "width: " << waveformPos.width() << "height: " << m_myslider->y();
//        // - m_myslider->m_handleTextHeight
//        qDebug()<<"play y:"<<waveformPoint.y() - 27;
//        qDebug()<<"before m_currPlayingItem->pos().y():"<<m_currPlayingItem->pos().y();
        m_myslider->setTimeText(QString(tr("00:00")));
        m_myslider->setGeometry( waveformPoint.x() - m_myslider->getHandlerWidth() / 2, waveformPoint.y() - 27, waveformPos.width() + m_myslider->getHandlerWidth(), m_myslider->m_defaultHeight);
        m_myslider->showSlider();
        curWaveformPosWidth = waveformPos.width() + m_myslider->getHandlerWidth();
        int sliderwidth = m_myslider->width() - m_myslider->getSliderHandlerWidth();
        //m_myslider->setRange(0, waveformPos.width());
        int duringtime = m_currPlayingItem->getRecodeTime();
        int maxtime = duringtime/1000;
        m_myslider->setRange(0, maxtime);
        m_myslider->setSliderPostion(0); //ynb 20191109
        m_myslider->show();

        //m_TestSlider->setGeometry( waveformPoint.x(), waveformPoint.y() - 27, waveformPos.width(), m_myslider->m_defaultHeight);
    }
}

void RightNoteList::pause()
{
    audioPlayer->pause();
}

void RightNoteList::stop()
{
    audioPlayer->stop();
}

//void RightNoteList::resume()
//{
//    audioPlayer->play();
//}

//void RightNoteList::stop(QString filepath)
//{
//    if (filepath == getPlayingFilepath()) {
//        audioPlayer->stop();
//    }
//}

//void RightNoteList::stopPlayer()
//{
//    audioPlayer->stop();
//}

QString RightNoteList::getPlayingFilepath()
{
    if (audioPlayer->isAudioAvailable()) {
        return audioPlayer->media().resources().first().url().path();
    } else {
        return "";
    }
}

void RightNoteList::changeSliderPosByHand(int moveMovment)
{
    if(nullptr != m_myslider)
    {
        if (!m_myslider->isHidden())
        {
            qDebug()<<"before move slider y:"<<m_myslider->y();
            m_myslider->move(m_myslider->x(), m_myslider->y() - moveMovment);
            qDebug()<<"after move slider y:"<<m_myslider->y();

            if(this->verticalScrollBar()->isVisible())
            {
                int value = this->verticalScrollBar()->value();
                this->verticalScrollBar()->setValue(value - 1);
            }
        }
    }
}

bool RightNoteList::getRowByID(int id, NOTE_TYPE type,  int &row)
{
    bool ret = false;

    int count = this->count();
    for(int i = 0; i < count; i++)
    {
        QListWidgetItem* ptmp = this->item(i);

        if(VOICE == type)
        {
            VoiceNoteItem* pWidget = (VoiceNoteItem*)this->itemWidget(ptmp);
            if(pWidget->getNoteID() == id)
            {
                ret = true;
                row = i;
                break;
            }
        }
        else if(TEXT == type)
        {
            TextNoteItem* pWidget = (TextNoteItem*)this->itemWidget(ptmp);
            if(pWidget->getId() == id)
            {
                ret = true;
                row = i;
                break;
            }
        }
        else
        {
            break;
        }
    }

    return ret;
}

void RightNoteList::handlePlayingStateChanged(QMediaPlayer::State state)
{
    if (QMediaPlayer::StoppedState == state)
    {
        if(nullptr != m_currPlayingItem)
        {
            disconnect(m_myslider,SIGNAL(sigSliderPos(int)),m_currPlayingItem->m_waveform,SLOT(OnSetWavePos(int)));
        }

        m_currPlayingItem->m_waveform->setWavePosition(-1);
        m_currPlayingItem->handleStopPlay();
        m_currPlayingItem = nullptr;
        m_myslider->hide();
    }
}


void RightNoteList::handleAudioPositionChanged(qint64 position)
{
//    if(nullptr != m_currPlayingItem)
//    {
//        //int audioLength = audioPlayer->duration();
//        int audioLength = m_currPlayingItem->m_note.voiceTime;
//        int sliderPos = 0;
//        qDebug()<<"position:"<<position;
//        if (audioLength > 0)
//        {
//            sliderPos = position * ( m_myslider->width()) / audioLength;
//        }

//        qDebug() << "handleAudioPositionChanged:" << position;
//        qDebug() << "sliderPos:" <<sliderPos;
//        m_currPlayingItem->m_waveform->setWavePosition(sliderPos);
//        //QTime curTime(0,0,0);
//        QTime curTime(0, position / 60000, qRound((position % 60000) / 1000.0));
//        //QTime curTime(0, position / 60000, (position % 60000) / 1000.0);
//        QString curTimeStr = curTime.toString(tr("mm:ss"));
//        qDebug()<<"curTime:"<<curTime;
//        m_myslider->setTimeText(curTimeStr);
//        //m_myslider->setTimeText(UiUtil::formatMillisecond(position));
//        m_myslider->setSliderPostion(sliderPos);

//    }
    //s ynbboy 20191109
    if(nullptr != m_currPlayingItem)
    {
        qDebug()<<"position:"<<position;
        if (0 == position)
        {
            QTime curTime(0, 0, 0);
            QString curTimeStr = curTime.toString(tr("mm:ss"));
            m_myslider->setTimeText(curTimeStr);
        }

       int s = int((position) / 1000.0);

       if (position == 0 || m_IsSliderBarReleased)
       {
          seconds = s + 1 ;
          m_IsSliderBarReleased = false;
       }

       int audioLength = int(audioPlayer->duration());
       int maxTime = audioLength/1000;
       qDebug()<<"s:"<<s;
       qDebug()<<"audioLength:"<<audioLength;
       qDebug()<<"maxTime:"<<maxTime;
       if ((s == seconds) || ((audioLength - position) < 1000 && position == audioLength))
       {
//            int sliderPos = 0;
//            int stepLenth = 0;
//            if (audioLength > 0)
//            {
//                long lwidth = m_myslider->width() - m_myslider->getSliderHandlerWidth();
//                double aa =  double((float)position/audioLength);
//                sliderPos = int(aa * lwidth + 0.5);
//            }
            QTime curTime(0, 0, 0);
            curTime = curTime.addSecs(seconds);
            QString curTimeStr;
            if(curTime.hour())
            {
                curTimeStr = "60:00";
            }
            else
            {
                curTimeStr = curTime.toString(tr("mm:ss"));
            }

            m_myslider->setTimeText(curTimeStr);
            m_myslider->setSliderPostion(s);
//            int sliderPos = m_myslider->gethandlePos();
//            m_currPlayingItem->m_waveform->setWavePosition(sliderPos);
            //m_myslider->setSliderPostion(sliderPos);

            qDebug() << "handleAudioPositionChanged:" << position;
//            qDebug() << "sliderPos:" <<sliderPos;
            qDebug()<<"seconds:"<<seconds;
            qDebug()<<"curTime:"<<curTime;

            seconds++;
       }
    }
    //e ynbboy 20191109
}

void RightNoteList::handleSliderReleased()
{
    m_IsSliderBarReleased = true; //ynb 20191109
    if(nullptr != m_currPlayingItem)
    {
        int sliderpos = m_myslider->sliderPosition();
//        int voiceTime = m_currPlayingItem->m_note.voiceTime;
//        int lenth = m_myslider->width() - m_myslider->getHandlerWidth();
//        int audioPos = sliderpos * voiceTime / lenth;
//        qDebug() << "audioPos:" << audioPos;
//        qDebug() << ",m_myslider->sliderPosition(): " << m_myslider->sliderPosition() ;
//        qDebug() << ",m_currPlayingItem->m_note.voiceTime: " << m_currPlayingItem->m_note.voiceTime ;
//        qDebug() << ",m_myslider->width(): " << m_myslider->width() - m_myslider->getHandlerWidth();

        //audioPlayer->setPosition(sliderpos);
        qint64 audioPos = sliderpos * 1000;
        //audioPos = 3000000;
        qDebug()<<"audioPos:"<<audioPos;
        audioPlayer->setPosition(audioPos);
    }
}

void RightNoteList::handleSliderPressed()
{
//    int audioPos = m_myslider->sliderPosition() * m_currPlayingItem->m_note.voiceTime / (m_myslider->width() - m_myslider->getHandlerWidth());
//    qDebug() << "audioPos:" << audioPos << ",m_myslider->sliderPosition(): " << m_myslider->sliderPosition() << ",m_currPlayingItem->m_note.voiceTime: " << m_currPlayingItem->m_note.voiceTime << ",m_myslider->width(): " << m_myslider->width() - m_myslider->getHandlerWidth();

//    audioPlayer->setPosition(audioPos);
}

void RightNoteList::handleSliderMove(int value)
{
//    int audioPos = m_myslider->sliderPosition() * m_currPlayingItem->m_note.voiceTime / (m_myslider->width() - m_myslider->getHandlerWidth());
//    qDebug() << "audioPos:" << audioPos << ",m_myslider->sliderPosition(): " << m_myslider->sliderPosition() << ",m_currPlayingItem->m_note.voiceTime: " << m_currPlayingItem->m_note.voiceTime << ",m_myslider->width(): " << m_myslider->width() - m_myslider->getHandlerWidth();

//    audioPlayer->setPosition(audioPos);
}

void RightNoteList::handleSaveFileEnd(bool result)
{
    QString title;

    if (result)
    {
        title = tr("保存文件成功!");
    }
    else
    {
        title = tr("保存文件失败!");
    }
    m_saveFileEndDialog->setTitle(title);
    m_saveFileEndDialog->show();
    //UiUtil::createConfirmDialog(QString(""), title, this);
}

void RightNoteList::handleMenuBtnPressed()
{
    m_arrowButtonPressed = true;
}

void RightNoteList::handleMenuBtnReleased()
{
    m_arrowButtonPressed = false;
}
void RightNoteList::adjustWidgetItemWidth()
{
    QListWidgetItem *ptmp = nullptr;
    for(int i = 0; i < this->count(); i++)
    {

        ptmp = this->item(i);

        QWidget* ptmpWidget = this->itemWidget(ptmp);
        int listWidth = this->width();
        int newWidth = this->width() - 23;
        ptmpWidget->setFixedSize(QSize(newWidth,ptmpWidget->height()));

        if(i != this->count() - 1)
        {
            this->item(i)->setSizeHint(QSize(listWidth,ptmpWidget->height()));
        }
    }
}
