#include "folerwidgetitem.h"

#include <QString>
#include <uiutil.h>
#include <DPalette>
#include <QLabel>
#include <QPalette>
//FolerWidgetItem::FolerWidgetItem()
FolerWidgetItem::FolerWidgetItem(FOLDER folder, FolderController *folderCtr, QString searchKey)
{
    this->m_folder = folder;
    this->m_folderCtr = folderCtr;
    this->m_searchKey = searchKey;
    initUI();
    initConnection();
}

FolerWidgetItem::~FolerWidgetItem()
{
}

void FolerWidgetItem::initUI()
{

    //this->resize(196, 60);
    setNormalBackground();
    //this->setStyleSheet("background: red");
    this->setFixedSize(230, 64);

    m_imageLabel = new QLabel(this);
    m_imageLabel->setGeometry(QRect(10, 10, 40, 40));
    m_imageLabel->setObjectName("imageLabel");
    //m_imageLabel->size();
    //QPixmap pixmap = getPixmap(m_imageLabel->size(), m_folder.imgPath);

//    bool convertFlag = getPixmap(imageLabel->size(), folder.imgPath, pixmap);
//    if (convertFlag)
//    {
    QPixmap docIcon = UiUtil::getPixmap(m_imageLabel->size(), m_folder.imgPath);
    QPixmap raddocIcon = UiUtil::PixmapToRound(docIcon,20);
        m_imageLabel->setPixmap(raddocIcon);
//    }

    m_nameLabel = new QLabel();

    //m_nameLabel->setGeometry(QRect(70, 10, 110, 21));
    m_nameLabel->setLineWidth(150);
    m_nameLabel->setObjectName("nameLabel");

    QFont labelFont;
    labelFont.setFamily("SourceHanSansSC");
    labelFont.setPointSize(14);
    m_nameLabel->setFont(labelFont);

    QPalette pe;
    pe.setColor(QPalette::Text,QColor(QRgb(0x001A2E)));
    m_nameLabel->setPalette(pe);
    bool isConverted = false;
    QString folderNameElided = UiUtil::getElidedText(m_nameLabel->font(), m_folder.folderName, FOLDER_MAX_WIDTH, isConverted);

    m_nameLabel->setText(UiUtil::getHtmlText(folderNameElided, 14, m_searchKey));
    m_nameLabel->setMouseTracking(false);
    m_nameLabel->setAttribute(Qt::WA_TransparentForMouseEvents,true);

    m_lineEdit = new DLineEdit();
    //m_lineEdit->setGeometry(QRect(70, 10, 110, 21));
    m_lineEdit->setObjectName("nameEdit");

    m_lineEdit->setText(m_folder.folderName);
    //m_lineEdit->setVisible(false);

    m_stackedWidget = new QStackedWidget(this);
    m_stackedWidget->setGeometry(QRect(70, 10, 110, 21));
    m_stackedWidget->setObjectName("stackedWidget");

    m_stackedWidget->addWidget(m_nameLabel);
    m_stackedWidget->addWidget(m_lineEdit);
    m_stackedWidget->setCurrentIndex(0);

    m_createTimeLabel = new QLabel(this);
    m_createTimeLabel->setGeometry(QRect(70, 31, 110, 18));
    //m_createTimeLabel->setGeometry(QRect(70, 40, 110, 16));
    //m_createTimeLabel->setLineWidth(150);
    m_createTimeLabel->setObjectName("createTimeLabel");
    labelFont.setPointSize(9);
    m_createTimeLabel->setFont(labelFont);
    pe.setColor(QPalette::Text,QColor(QRgb(0x526A7F)));
    m_createTimeLabel->setPalette(pe);
    //m_createTimeLabel->setText(getCreateTimeLabel(m_folder.createTime));
    m_createTimeLabel->setText(UiUtil::convertFolderDate(m_folder.createTime));

}

void FolerWidgetItem::initConnection()
{
    connect(m_lineEdit, &DLineEdit::editingFinished, this, &FolerWidgetItem::checkNameValid);
}


void FolerWidgetItem::setItemBackground(QString imgPath)
{
    QPalette pal = this->palette();
    pal.setBrush(QPalette::Background, QBrush(QPixmap(imgPath)));
    this->setPalette(pal);
}
void FolerWidgetItem::setNormalBackground()
{
    //setItemBackground(":/image/folder_normal.png");
//    QPalette p = this->palette();
//    p.setBrush(this->backgroundRole(),QBrush(QColor(51,51,51)));
//    this->setPalette(p);
}

void FolerWidgetItem::changeToEditMode()
{
//    m_nameLabel->setVisible(false);
//    m_lineEdit->setVisible(true);
    m_stackedWidget->setCurrentIndex(1);

    m_lineEdit->selectAll();
    m_lineEdit->setCursorPosition(m_lineEdit->text().size());
    m_lineEdit->setFocus();
    m_createTimeLabel->setVisible(false);

}

void FolerWidgetItem::changeToClickMode()
{
    QPalette pe;
    pe.setColor(QPalette::Text,Qt::white);
    m_nameLabel->setPalette(pe);
    m_createTimeLabel->setPalette(pe);
}

void FolerWidgetItem::changeToUnClickMode()
{
    QPalette pe;
    pe.setColor(QPalette::Text,QColor(QRgb(0x001A2E)));
    m_nameLabel->setPalette(pe);
    pe.setColor(QPalette::Text,QColor(QRgb(0x526A7F)));
    m_createTimeLabel->setPalette(pe);
}

void FolerWidgetItem::checkNameValid()
{
    if ((m_lineEdit->text().length() > 0) && (m_lineEdit->text().length() < 64)) {
        //todo:更新数据库
        m_folder.folderName = m_lineEdit->text();
        if (m_folderCtr->checkFolderNameExist(m_folder))
        {
            m_lineEdit->setAlert(true);
            m_lineEdit->showAlertMessage("目录名重复！");
        }
        else
        {
            if(!m_folderCtr->updateFolder(m_folder))
            {
                m_lineEdit->setAlert(true);
                m_lineEdit->showAlertMessage("修改目录名失败");

            }
            m_nameLabel->setText(UiUtil::getHtmlText(m_lineEdit->text(), 14, m_searchKey));
            m_stackedWidget->setCurrentIndex(0);
//            m_nameLabel->setVisible(true);
//            m_lineEdit->setVisible(false);
        }
        m_createTimeLabel->setVisible(true);
    } else {
        //警告用户输入不能为空
        m_lineEdit->setAlert(true);
        m_lineEdit->showAlertMessage("输入字符长度必须在0-64位之间");
    }

}

