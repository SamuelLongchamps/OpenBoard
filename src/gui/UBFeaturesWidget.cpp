#include <QDomDocument>

#include "UBFeaturesWidget.h"
#include "domain/UBAbstractWidget.h"
#include "gui/UBThumbnailWidget.h"
#include "gui/UBLibraryWidget.h"
#include "frameworks/UBFileSystemUtils.h"
#include "core/UBApplication.h"
#include "core/UBDownloadManager.h"
#include "globals/UBGlobals.h"
#include "board/UBBoardController.h"

const char *objNamePathList = "PathList";
const char *objNameFeatureList = "FeatureList";

const QMargins FeatureListMargins(0, 0, 0, 30);
const int FeatureListBorderOffset = 10;
const char featureTypeSplitter = ':';
static const QString mimeSankoreFeatureTypes = "Sankore/featureTypes";

UBFeaturesWidget::UBFeaturesWidget(QWidget *parent, const char *name):UBDockPaletteWidget(parent)	
{
    setObjectName(name);
    mName = "FeaturesWidget";
    mVisibleState = true;

    SET_STYLE_SHEET();

    mIconToLeft = QPixmap(":images/library_open.png");
    mIconToRight = QPixmap(":images/library_close.png");
    setAcceptDrops(true);
	
    //Main UBFeature functionality
    controller = new UBFeaturesController(this);

    //Main layout including all the widgets in palette
    layout = new QVBoxLayout(this);

    //Path icon view on the top of the palette
    pathListView = new UBFeaturesListView(this, objNamePathList);
    controller->assignPathListView(pathListView);

    //Maintains the view of the main part of the palette. Consists of
    //mNavigator
    //featureProperties
    //webVeiw
    stackedWidget = new QStackedWidget(this);

    //Main features icon view with QSlider on the bottom
    mNavigator = new UBFeaturesNavigatorWidget(this);
    controller->assignFeaturesListVeiw(mNavigator->listView());
    mNavigator->setSliderPosition(UBSettings::settings()->featureSliderPosition->get().toInt());

    //Specifies the properties of a standalone element
    featureProperties = new UBFeatureProperties(this);

    //Used to show search bar on the search widget
    webView = new UBFeaturesWebView(this);

        //filling stackwidget
        stackedWidget->addWidget(mNavigator);
        stackedWidget->addWidget(featureProperties);
        stackedWidget->addWidget(webView);
        stackedWidget->setCurrentIndex(ID_LISTVIEW);
        currentStackedWidget = ID_LISTVIEW;

    //Bottom actionbar for DnD, quick search etc
    mActionBar = new UBFeaturesActionBar(controller, this);
		
    //Filling main layout
    layout->addWidget( pathListView );
	layout->addWidget( stackedWidget );
    layout->addWidget(mActionBar);

    connect(mNavigator->listView(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(currentSelected(const QModelIndex &)));
    connect(mActionBar, SIGNAL(searchElement(const QString &)), this, SLOT( searchStarted(const QString &)));
    connect(mActionBar, SIGNAL(newFolderToCreate()), this, SLOT(createNewFolder()));
    connect(mActionBar, SIGNAL(deleteElements(const UBFeaturesMimeData *)), this, SLOT(deleteElements(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(deleteSelectedElements()), this, SLOT(deleteSelectedElements()));
    connect(mActionBar, SIGNAL(addToFavorite(const UBFeaturesMimeData *)), this, SLOT(addToFavorite(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(removeFromFavorite(const UBFeaturesMimeData *)), this, SLOT(removeFromFavorite(const UBFeaturesMimeData *)));
    connect(mActionBar, SIGNAL(addElementsToFavorite() ), this, SLOT ( addElementsToFavorite()) );
    connect(mActionBar, SIGNAL(removeElementsFromFavorite()), this, SLOT (removeElementsFromFavorite()));

    connect( mActionBar, SIGNAL(rescanModel()), this, SLOT(rescanModel()));
    connect(pathListView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(currentSelected(const QModelIndex &)));
    connect( UBApplication::boardController, SIGNAL(displayMetadata(QMap<QString,QString>)), this, SLOT(onDisplayMetadata( QMap<QString,QString>)));
    connect( UBDownloadManager::downloadManager(), SIGNAL( addDownloadedFileToLibrary( bool, QUrl, QString, QByteArray)), this, SLOT(onAddDownloadedFileToLibrary(bool, QUrl, QString,QByteArray)));
}

UBFeaturesWidget::~UBFeaturesWidget()
{
}

void UBFeaturesWidget::searchStarted(const QString &pattern)
{
    controller->searchStarted(pattern, mNavigator->listView());
}

void UBFeaturesWidget::currentSelected(const QModelIndex &current)
{
    if (!current.isValid()) {
        qWarning() << "SLOT:currentSelected, invalid index catched";
        return;
    }

    QString objName = sender()->objectName();

    if (objName.isEmpty()) {
        qWarning() << "incorrrect sender";
    }

    QListView *calledList = 0;
    if (objName == objNamePathList) {
        calledList = pathListView;
    } else if (objName == objNameFeatureList) {
        calledList = mNavigator->listView();
    }

    UBFeature feature = controller->getFeature(current, calledList);
//    QSortFilterProxyModel *model = dynamic_cast<QSortFilterProxyModel *>( mNavigator->listView()->model() );
//    UBFeature feature = model->data(current, Qt::UserRole + 1).value<UBFeature>();

    if ( feature.isFolder() ) {
        QString newPath = feature.getFullVirtualPath();

        controller->setCurrentElement(feature);
        controller->siftElements(newPath);

        switchToListView();

        if ( feature.getType() == FEATURE_FAVORITE ) {
            mActionBar->setCurrentState( IN_FAVORITE );

        }  else   if ( feature.getType() == FEATURE_CATEGORY && feature.getName() == "root" ) {
            mActionBar->setCurrentState( IN_ROOT );

        } else if (feature.getType() == FEATURE_TRASH) {
            mActionBar->setCurrentState( IN_TRASH );

        } else {
            mActionBar->setCurrentState( IN_FOLDER );
        }

    } else if ( feature.getType() == FEATURE_SEARCH ) {
        webView->showElement( feature );
        switchToWebView();

    } else {
        featureProperties->showElement( feature );
        switchToProperties();
        mActionBar->setCurrentState( IN_PROPERTIES );
    }
}

void UBFeaturesWidget::createNewFolder()
{
	UBNewFolderDlg dlg;
    if(QDialog::Accepted == dlg.exec())  {
        controller->addNewFolder(dlg.folderName());
    }
}

void UBFeaturesWidget::deleteElements( const UBFeaturesMimeData * mimeData )
{
    if (!mimeData->features().count() )
        return;

    QList<UBFeature> featuresList = mimeData->features();
	
    foreach ( UBFeature curFeature, featuresList ) {
        if ( curFeature.inTrash()) {
            controller->deleteItem(curFeature.getFullPath());

        } else {
           controller->moveToTrash(curFeature);
        }
    }

    controller->refreshModels();
}

void UBFeaturesWidget::deleteSelectedElements()
{
    QModelIndexList selected = mNavigator->listView()->selectionModel()->selectedIndexes();

    foreach ( QModelIndex sel, selected )
    {
        UBFeature feature = sel.data(Qt::UserRole + 1).value<UBFeature>();
        if (feature.isDeletable()) {
            if (feature.inTrash()) {
                controller->deleteItem(feature);
            } else {
                controller->moveToTrash(feature, true);
            }
        }
    }
   controller->refreshModels();
}

void UBFeaturesWidget::rescanModel()
{
    controller->rescanModel();
}

void UBFeaturesWidget::addToFavorite( const UBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();
    foreach ( QUrl url, urls ) {
        controller->addToFavorite(url);
    }

    controller->refreshModels();
}

void UBFeaturesWidget::removeFromFavorite( const UBFeaturesMimeData * mimeData )
{
    if ( !mimeData->hasUrls() )
        return;

    QList<QUrl> urls = mimeData->urls();

    foreach( QUrl url, urls ) {
        controller->removeFromFavorite(url);
    }
}

void UBFeaturesWidget::onDisplayMetadata( QMap<QString,QString> metadata )
{
	UBFeature feature( QString(), QPixmap(":images/libpalette/notFound.png"), QString(), metadata["Url"], FEATURE_ITEM );
	feature.setMetadata( metadata );

	featureProperties->showElement( feature );
	switchToProperties();
	mActionBar->setCurrentState( IN_PROPERTIES );
}

void UBFeaturesWidget::onAddDownloadedFileToLibrary(bool pSuccess, QUrl sourceUrl, QString pContentHeader, QByteArray pData)
{
    Q_UNUSED(pContentHeader)

    if ( pSuccess )
    {
        UBFeature newFeature = controller->addDownloadedFile( sourceUrl, pData );
        if ( newFeature != UBFeature() )
        {
            featuresModel->addItem( newFeature );
            QSortFilterProxyModel *model = dynamic_cast<QSortFilterProxyModel *>( mNavigator->listView()->model() );
            model->invalidate();
        }
    }
}

void UBFeaturesWidget::addElementsToFavorite()
{
    if ( currentStackedWidget == ID_PROPERTIES ) {
        UBFeature feature = featureProperties->getCurrentElement();
        if ( feature != UBFeature() && !UBApplication::isFromWeb(feature.getFullPath().toString())) {
            controller->addToFavorite( feature.getFullPath() );
        }

    } else if ( currentStackedWidget == ID_LISTVIEW ) {
        QModelIndexList selected = mNavigator->listView()->selectionModel()->selectedIndexes();
        for ( int i = 0; i < selected.size(); ++i ) {
            UBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<UBFeature>();
            controller->addToFavorite(feature.getFullPath());
       }
    }

    controller->refreshModels();
}

void UBFeaturesWidget::removeElementsFromFavorite()
{
    QModelIndexList selected = mNavigator->listView()->selectionModel()->selectedIndexes();
    QList <QUrl> items;
    for ( int i = 0; i < selected.size(); ++i )  {
        UBFeature feature = selected.at(i).data( Qt::UserRole + 1 ).value<UBFeature>();
        items.append( feature.getFullPath() );
    }

    foreach ( QUrl url, items )  {
        controller->removeFromFavorite(url, true);
    }

    controller->refreshModels();
}

void UBFeaturesWidget::switchToListView()
{
	stackedWidget->setCurrentIndex(ID_LISTVIEW);
	currentStackedWidget = ID_LISTVIEW;
}

void UBFeaturesWidget::switchToProperties()
{
	stackedWidget->setCurrentIndex(ID_PROPERTIES);
	currentStackedWidget = ID_PROPERTIES;
}

void UBFeaturesWidget::switchToWebView()
{
	stackedWidget->setCurrentIndex(ID_WEBVIEW);
	currentStackedWidget = ID_WEBVIEW;
}

void UBFeaturesWidget::updateSliderPosition()
{
//    //Calculating the topleft position of the slider
//    int scrollBarVOffset = featuresListView->horizontalScrollBar()->isVisible() ? featuresListView->horizontalScrollBar()->height() : 0;
//    int xSlider = scrollbarHorisontalPadding();
//    int ySlider = featuresListView->height() - (thumbSlider->height() + scrollbarVerticalIndent() + scrollBarVOffset);
//    thumbSlider->move( xSlider, ySlider);

//    //Calculating new slider's size
//    int scrollBarHOffset = featuresListView->verticalScrollBar()->isVisible() ? featuresListView->verticalScrollBar()->width() : 0;
//    int wSlider = featuresListView->width() - 2 * scrollbarHorisontalPadding() - scrollBarHOffset;
//    int hSlider = thumbSlider->height();
//    thumbSlider->resize(wSlider, hSlider);
}

/*

void UBFeaturesWidget::currentPathChanged(const QString &path)
{
	int newDepth = path.count("/");
	pathViewer->truncatePath(newDepth);
	featuresListView->setModel( featuresProxyModel );
	featuresProxyModel->setFilterFixedString(path);
	featuresProxyModel->invalidate();
	switchToListView();
}
*/

QStringList UBFeaturesMimeData::formats() const
{
    return QMimeData::formats();
}


UBFeaturesListView::UBFeaturesListView( QWidget* parent, const char* name ) 
: QListView(parent)
{
	setObjectName(name);
}

/*
void UBFeaturesListView::mousePressEvent( QMouseEvent *event )
{
	rubberOrigin = event->pos();
	rubberBand->setGeometry( QRect( rubberOrigin, QSize() ) );
	//qDebug()  << rubberOrigin.x() << rubberOrigin.y();
	rubberBand->show();
	QListView::mousePressEvent(event);
}

void UBFeaturesListView::mouseMoveEvent( QMouseEvent *event )
{
	QPoint current = event->pos();
	rubberBand->setGeometry( QRect( rubberOrigin, current ).normalized() );

	//setSelection( rubberBand->rect(), QItemSelectionModel::Select );
	QListView::mouseMoveEvent(event);
}

void UBFeaturesListView::mouseReleaseEvent( QMouseEvent *event )
{
	rubberBand->hide();
	QListView::mouseReleaseEvent(event);
}

*/
void UBFeaturesListView::dragEnterEvent( QDragEnterEvent *event )
{
    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() )
		event->acceptProposedAction();
}

void UBFeaturesListView::dragMoveEvent( QDragMoveEvent *event )
{
    const UBFeaturesMimeData *fMimeData = qobject_cast<const UBFeaturesMimeData*>(event->mimeData());
    QModelIndex index = indexAt(event->pos());
    UBFeature onFeature = model()->data(index, Qt::UserRole + 1).value<UBFeature>();
    if (fMimeData) {
        if (!index.isValid() || !onFeature.isFolder()) {
            event->ignore();
            return;
        }
     }

    if ( event->mimeData()->hasUrls() || event->mimeData()->hasImage() ) {
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void UBFeaturesListView::dropEvent( QDropEvent *event )
{
    QWidget *eventSource = event->source();
    if (eventSource && eventSource->objectName() == objNameFeatureList) {
        event->setDropAction( Qt::MoveAction );
    }

    QListView::dropEvent( event );
}

void UBFeaturesListView::thumbnailSizeChanged( int value )
{
    setIconSize(QSize(value, value));
    setGridSize(QSize(value + 20, value + 20 ));

    UBSettings::settings()->featureSliderPosition->set(value);
}

UBFeaturesNavigatorWidget::UBFeaturesNavigatorWidget(QWidget *parent, const char *name) :
    QWidget(parent), mListView(0), mListSlder(0)

{
//    if ('\0' == name) {
        name = "UBFeaturesNavigatorWidget";
//    }

    setObjectName(name);
    SET_STYLE_SHEET()

    mListView = new UBFeaturesListView(this, objNameFeatureList);

    mListSlder = new QSlider(Qt::Horizontal, this);

    mListSlder->setMinimum(UBFeaturesWidget::minThumbnailSize);
    mListSlder->setMaximum(UBFeaturesWidget::maxThumbnailSize);
    mListSlder->setValue(UBFeaturesWidget::minThumbnailSize);
    mListSlder->setMinimumHeight(20);

    mListView->setParent(this);
    QVBoxLayout *mainLayer = new QVBoxLayout(this);

    mainLayer->addWidget(mListView, 1);
    mainLayer->addWidget(mListSlder, 0);


    connect(mListSlder, SIGNAL(valueChanged(int)), mListView, SLOT(thumbnailSizeChanged(int)));
}

void UBFeaturesNavigatorWidget::setSliderPosition(int pValue)
{
    mListSlder->setValue(pValue);
}

UBFeaturesWebView::UBFeaturesWebView(QWidget* parent, const char* name):QWidget(parent)
    , mpView(NULL)
    , mpWebSettings(NULL)
    , mpLayout(NULL)
    , mpSankoreAPI(NULL)
{
    setObjectName(name);

    SET_STYLE_SHEET();

    mpLayout = new QVBoxLayout();
    setLayout(mpLayout);

    mpView = new QWebView(this);
    mpView->setObjectName("SearchEngineView");
    mpSankoreAPI = new UBWidgetUniboardAPI(UBApplication::boardController->activeScene());
    mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);

    mpWebSettings = QWebSettings::globalSettings();
    mpWebSettings->setAttribute(QWebSettings::JavaEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::PluginsEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::LocalStorageDatabaseEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::OfflineWebApplicationCacheEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::OfflineStorageDatabaseEnabled, true);
    mpWebSettings->setAttribute(QWebSettings::JavascriptCanAccessClipboard, true);
    mpWebSettings->setAttribute(QWebSettings::DnsPrefetchEnabled, true);

    mpLayout->addWidget(mpView);

    connect(mpView, SIGNAL(loadFinished(bool)), this, SLOT(onLoadFinished(bool)));
}

UBFeaturesWebView::~UBFeaturesWebView()
{
    if( NULL != mpSankoreAPI )
    {
        delete mpSankoreAPI;
        mpSankoreAPI = NULL;
    }
    if( NULL != mpView )
    {
        delete mpView;
        mpView = NULL;
    }
    if( NULL != mpLayout )
    {
        delete mpLayout;
        mpLayout = NULL;
    }
}

void UBFeaturesWebView::showElement(const UBFeature &elem)
{
	QString qsWidgetName;
    QString path = elem.getFullPath().toLocalFile();

	QString qsConfigPath = QString("%0/config.xml").arg(path);

	if(QFile::exists(qsConfigPath))
	{
		QFile f(qsConfigPath);
		if(f.open(QIODevice::ReadOnly))
		{
			QDomDocument domDoc;
			domDoc.setContent(QString(f.readAll()));
			QDomElement root = domDoc.documentElement();

			QDomNode node = root.firstChild();
			while(!node.isNull())
			{
				if(node.toElement().tagName() == "content")
				{
					QDomAttr srcAttr = node.toElement().attributeNode("src");
					qsWidgetName = srcAttr.value();
					break;
				}
				node = node.nextSibling();
			}
			f.close();
		}
	}

	mpView->load(QUrl::fromLocalFile(QString("%0/%1").arg(path).arg(qsWidgetName)));    
}

void UBFeaturesWebView::onLoadFinished(bool ok)
{
    if(ok && NULL != mpSankoreAPI){
        mpView->page()->mainFrame()->addToJavaScriptWindowObject("sankore", mpSankoreAPI);
    }
}


UBFeatureProperties::UBFeatureProperties( QWidget *parent, const char *name ) : QWidget(parent)
    , mpLayout(NULL)
    , mpButtonLayout(NULL)
    , mpAddPageButton(NULL)
    , mpAddToLibButton(NULL)
    , mpSetAsBackgroundButton(NULL)
    , mpObjInfoLabel(NULL)
    , mpObjInfos(NULL)
    , mpThumbnail(NULL)
    , mpOrigPixmap(NULL)
    , mpElement(NULL)
{
	setObjectName(name);

    SET_STYLE_SHEET();
    //setStyleSheet(UBApplication::globalStyleSheet());

    // Create the GUI
    mpLayout = new QVBoxLayout(this);
    setLayout(mpLayout);

    maxThumbHeight = height() / 4;

    mpThumbnail = new QLabel();
    QPixmap icon(":images/libpalette/notFound.png");
    icon.scaledToWidth(THUMBNAIL_WIDTH);

    mpThumbnail->setPixmap(icon);
    mpThumbnail->setObjectName("DockPaletteWidgetBox");
    mpThumbnail->setStyleSheet("background:white;");
    mpThumbnail->setAlignment(Qt::AlignHCenter);
    mpLayout->addWidget(mpThumbnail, 0);

    mpButtonLayout = new QHBoxLayout();
    mpLayout->addLayout(mpButtonLayout, 0);

    mpAddPageButton = new UBFeatureItemButton();
    mpAddPageButton->setText(tr("Add to page"));
    mpButtonLayout->addWidget(mpAddPageButton);

    mpSetAsBackgroundButton = new UBFeatureItemButton();
    mpSetAsBackgroundButton->setText(tr("Set as background"));
    mpButtonLayout->addWidget(mpSetAsBackgroundButton);

    mpAddToLibButton = new UBFeatureItemButton();
    mpAddToLibButton->setText(tr("Add to library"));
    mpButtonLayout->addWidget(mpAddToLibButton);

    mpButtonLayout->addStretch(1);

	mpObjInfoLabel = new QLabel(tr("Object informations"));
    mpObjInfoLabel->setStyleSheet(QString("color: #888888; font-size : 18px; font-weight:bold;"));
    mpLayout->addWidget(mpObjInfoLabel, 0);

    mpObjInfos = new QTreeWidget(this);
    mpObjInfos->setColumnCount(2);
    mpObjInfos->header()->hide();
    mpObjInfos->setAlternatingRowColors(true);
    mpObjInfos->setRootIsDecorated(false);
    mpObjInfos->setObjectName("DockPaletteWidgetBox");
    mpObjInfos->setStyleSheet("background:white;");
    mpLayout->addWidget(mpObjInfos, 1);

	connect( mpAddPageButton, SIGNAL(clicked()), this, SLOT(onAddToPage()) );
    connect( mpSetAsBackgroundButton, SIGNAL( clicked() ), this, SLOT( onSetAsBackground() ) );
    connect( mpAddToLibButton, SIGNAL( clicked() ), this, SLOT(onAddToLib() ) );
}

UBFeatureProperties::~UBFeatureProperties()
{
	if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
	if ( mpElement )
	{
		delete mpElement;
		mpElement = NULL;
	}
    if ( mpThumbnail )
    {
        delete mpThumbnail;
        mpThumbnail = NULL;
    }
    if ( mpButtonLayout )
    {
        delete mpButtonLayout;
        mpButtonLayout = NULL;
    }
    if ( mpAddPageButton )
    {
        delete mpAddPageButton;
        mpAddPageButton = NULL;
    }
    if ( mpSetAsBackgroundButton )
    {
        delete mpSetAsBackgroundButton;
        mpSetAsBackgroundButton = NULL;
    }
    if ( mpAddToLibButton )
    {
        delete mpAddToLibButton;
        mpAddToLibButton = NULL;
    }
    if ( mpObjInfoLabel )
    {
        delete mpObjInfoLabel;
        mpObjInfoLabel = NULL;
    }
    if ( mpObjInfos )
    {
        delete mpObjInfos;
        mpObjInfos = NULL;
    }
}

void UBFeatureProperties::resizeEvent( QResizeEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

void UBFeatureProperties::showEvent (QShowEvent *event )
{
    Q_UNUSED(event);
    adaptSize();
}

UBFeature UBFeatureProperties::getCurrentElement() const
{
    if ( mpElement )
        return *mpElement;

    return UBFeature();
}

void UBFeatureProperties::adaptSize()
{
    if( NULL != mpOrigPixmap )
    {
        if( width() < THUMBNAIL_WIDTH + 40 )
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( width() - 40 ) );
        }
        else
        {
            mpThumbnail->setPixmap( mpOrigPixmap->scaledToWidth( THUMBNAIL_WIDTH ) );
        }
    }
}

void UBFeatureProperties::showElement( const UBFeature &elem )
{
	if ( mpOrigPixmap )
    {
        delete mpOrigPixmap;
        mpOrigPixmap = NULL;
    }
	if ( mpElement )
	{
		delete mpElement;
		mpElement = NULL;
	}
    mpElement = new UBFeature( elem );
	mpOrigPixmap = new QPixmap( elem.getThumbnail() );
	mpThumbnail->setPixmap(elem.getThumbnail().scaledToWidth(THUMBNAIL_WIDTH));
    populateMetadata();

    if ( UBApplication::isFromWeb( elem.getFullPath().toString() ) )
	{
        mpAddToLibButton->show();
		if( elem.getMetadata()["Type"].toLower().contains("image") )
		{
            mpSetAsBackgroundButton->show();
        }
		else
		{
            mpSetAsBackgroundButton->hide();
        }
    }
	else
	{
        mpAddToLibButton->hide();
        if (UBFileSystemUtils::mimeTypeFromFileName( elem.getFullPath().toLocalFile() ).contains("image"))
		{
            mpSetAsBackgroundButton->show();
        }
	    else
		{
            mpSetAsBackgroundButton->hide();
        }
    }
}

void UBFeatureProperties::populateMetadata()
{
    if(NULL != mpObjInfos){
        mpObjInfos->clear();
		QMap<QString, QString> metas = mpElement->getMetadata();
        QList<QString> lKeys = metas.keys();
        QList<QString> lValues = metas.values();

        for(int i=0; i< metas.size(); i++){
            QStringList values;
            values << lKeys.at(i);
            values << lValues.at(i);
            mpItem = new QTreeWidgetItem(values);
            mpObjInfos->addTopLevelItem(mpItem);
        }
        mpObjInfos->resizeColumnToContents(0);
    }
}

void UBFeatureProperties::onAddToPage()
{
	QWidget *w = parentWidget()->parentWidget();
    UBFeaturesWidget* featuresWidget = dynamic_cast<UBFeaturesWidget*>( w );
    featuresWidget->getFeaturesController()->addItemToPage( *mpElement );
}

void UBFeatureProperties::onAddToLib()
{
    if ( UBApplication::isFromWeb(  mpElement->getFullPath().toString() ) )
    {
        sDownloadFileDesc desc;
        desc.isBackground = false;
        desc.modal = false;
        desc.name = QFileInfo( mpElement->getFullPath().toString()).fileName();
        qDebug() << desc.name;
        desc.url = mpElement->getFullPath().toString();
        qDebug() << desc.url;
        UBDownloadManager::downloadManager()->addFileToDownload(desc);
    }
}


void UBFeatureProperties::onSetAsBackground()
{
    QWidget *w = parentWidget()->parentWidget();
    UBFeaturesWidget* featuresWidget = dynamic_cast<UBFeaturesWidget*>( w );
    featuresWidget->getFeaturesController()->addItemAsBackground( *mpElement );
}



UBFeatureItemButton::UBFeatureItemButton(QWidget *parent, const char *name):QPushButton(parent)
{
    setObjectName(name);
    setStyleSheet(QString("background-color : #DDDDDD; color : #555555; border-radius : 6px; padding : 5px; font-weight : bold; font-size : 12px;"));
}

UBFeatureItemButton::~UBFeatureItemButton()
{

}

QVariant UBFeaturesModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
        return QVariant();

    if (role == Qt::DisplayRole) {
		return featuresList->at(index.row()).getName();
    }

    else if (role == Qt::DecorationRole) {
		return QIcon( featuresList->at(index.row()).getThumbnail() );

    } else if (role == Qt::UserRole) {
		return featuresList->at(index.row()).getVirtualPath();

    }	else if (role == Qt::UserRole + 1) {
		//return featuresList->at(index.row()).getType();
		UBFeature f = featuresList->at(index.row());
		return QVariant::fromValue( f );
	}

	return QVariant();
}

QMimeData* UBFeaturesModel::mimeData(const QModelIndexList &indexes) const
{
    UBFeaturesMimeData *mimeData = new UBFeaturesMimeData();
	QList <QUrl> urlList;
    QList <UBFeature> featuresList;
    QByteArray typeData;

    foreach (QModelIndex index, indexes) {

        if (index.isValid()) {
            UBFeature element = data(index, Qt::UserRole + 1).value<UBFeature>();
            urlList.push_back( element.getFullPath() );
            QString curPath = element.getFullPath().toLocalFile();
            featuresList.append(element);

            if (!typeData.isNull()) {
                typeData += UBFeaturesController::featureTypeSplitter();
            }
            typeData += QString::number(element.getType()).toAscii();
		}
	}

    mimeData->setUrls(urlList);
    mimeData->setFeatures(featuresList);
    mimeData->setData(mimeSankoreFeatureTypes, typeData);

    return mimeData;
}

bool UBFeaturesModel::dropMimeData(const QMimeData *mimeData, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row)

    const UBFeaturesMimeData *fMimeData = qobject_cast<const UBFeaturesMimeData*>(mimeData);
    bool dataFromSameModel = false;

    if (fMimeData)
        dataFromSameModel = true;

    if ((!mimeData->hasUrls() && !mimeData->hasImage()) )
		return false;
	if ( action == Qt::IgnoreAction )
		return true;
	if ( column > 0 )
		return false;

    UBFeature parentFeature;
    if (!parent.isValid()) {
		parentFeature = dynamic_cast<UBFeaturesWidget *>(QObject::parent())->getFeaturesController()->getCurrentElement();
    } else {
        parentFeature = parent.data( Qt::UserRole + 1).value<UBFeature>();
    }

    if (mimeData->hasUrls()) {
        QList<UBFeature> featList = fMimeData->features();
        for (int i = 0; i < featList.count(); i++) {
            UBFeature sourceElement;
            if (dataFromSameModel) {
                sourceElement = featList.at(i);
                moveData(sourceElement, parentFeature, Qt::MoveAction);
            }
        }
    } else if ( mimeData->hasImage() ) {
        QImage image = qvariant_cast<QImage>( mimeData->imageData() );
        UBFeature element = dynamic_cast<UBFeaturesWidget *>(QObject::parent())->getFeaturesController()->importImage( image, parentFeature );
        addItem( element );
    }

	return true;
}

void UBFeaturesModel::addItem( const UBFeature &item )
{
	beginInsertRows( QModelIndex(), featuresList->size(), featuresList->size() );
	featuresList->append( item );
	endInsertRows();
}

void UBFeaturesModel::deleteFavoriteItem( const QString &path )
{
	for ( int i = 0; i < featuresList->size(); ++i )
	{
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) &&
			!QString::compare( featuresList->at(i).getVirtualPath(), "/root/favorites", Qt::CaseInsensitive ) )
		{
			removeRow( i, QModelIndex() );
			return;
		}
	}
}

void UBFeaturesModel::deleteItem( const QString &path )
{
    for ( int i = 0; i < featuresList->size(); ++i )
	{
        if ( !QString::compare( featuresList->at(i).getFullPath().toString(), path, Qt::CaseInsensitive ) )
		{
			removeRow( i, QModelIndex() );
			return;
		}
	}
}

void UBFeaturesModel::deleteItem(const UBFeature &feature)
{
    int i = featuresList->indexOf(feature);
    if (i == -1) {
        qDebug() << "no matches in deleting item from UBFEaturesModel";
        return;
    }
    removeRow(i, QModelIndex());
}

bool UBFeaturesModel::removeRows( int row, int count, const QModelIndex & parent )
{
	if ( row < 0 )
		return false;
	if ( row + count > featuresList->size() )
		return false;
	beginRemoveRows( parent, row, row + count - 1 );
	//featuresList->remove( row, count );
	featuresList->erase( featuresList->begin() + row, featuresList->begin() + row + count );
	endRemoveRows();
	return true;
}

bool UBFeaturesModel::removeRow(  int row, const QModelIndex & parent )
{
	if ( row < 0 )
		return false;
	if ( row >= featuresList->size() )
		return false;
	beginRemoveRows( parent, row, row );
	//featuresList->remove( row );
	featuresList->erase( featuresList->begin() + row );
	endRemoveRows();
	return true;
}

void UBFeaturesModel::moveData(const UBFeature &source, const UBFeature &destination
                               , Qt::DropAction action = Qt::CopyAction, bool deleteManualy)
{
    UBFeaturesController *curController = qobject_cast<UBFeaturesController *>(QObject::parent());
    if (!curController)
        return;

    QString sourcePath = source.getFullPath().toLocalFile();
    QString sourceVirtualPath = source.getVirtualPath();

    UBFeatureElementType sourceType = source.getType();
    QPixmap sourceIcon = source.getThumbnail();

    Q_ASSERT( QFileInfo( sourcePath ).exists() );

    QString name = QFileInfo( sourcePath ).fileName();
    QString destPath = destination.getFullPath().toLocalFile();

    QString destVirtualPath = destination.getFullVirtualPath();
    QString destFullPath = destPath + "/" + name;

    if ( sourcePath.compare(destFullPath, Qt::CaseInsensitive ) || destination.getType() != FEATURE_TRASH)
    {
        UBFileSystemUtils::copy(sourcePath, destFullPath);
        if (action == Qt::MoveAction) {
            curController->deleteItem( source.getFullPath() );
        }
    }

    //Passing all the source container ubdating dependancy pathes
    if (sourceType == FEATURE_FOLDER) {
        for (int i = 0; i < featuresList->count(); i++) {

            UBFeature &curFeature = (*featuresList)[i];

            QString curFeatureFullPath = curFeature.getFullPath().toLocalFile();
            QString curFeatureVirtualPath = curFeature.getVirtualPath();

            if (curFeatureFullPath.contains(sourcePath) && curFeatureFullPath != sourcePath) {

                UBFeature copyFeature = curFeature;
                QUrl newPath = QUrl::fromLocalFile(curFeatureFullPath.replace(sourcePath, destFullPath));
                QString newVirtualPath = curFeatureVirtualPath.replace(sourceVirtualPath, destVirtualPath);
                //when copying to trash don't change the real path
                if (destination.getType() != FEATURE_TRASH) {
                    // processing copy or move action for real FS
                    if (action == Qt::CopyAction) {
                        copyFeature.setFullPath(newPath);
                    } else {
                        curFeature.setFullPath(newPath);
                    }
                }
                // processing copy or move action for real FS
                if (action == Qt::CopyAction) {
                    copyFeature.setFullVirtualPath(newVirtualPath);
                } else {
                    curFeature.setFullVirtualPath(newVirtualPath);
                }

                if (action == Qt::CopyAction) {
                    addItem(copyFeature);
                }
            }
        }
    }

    UBFeature newElement( destVirtualPath, sourceIcon, name, QUrl::fromLocalFile(destFullPath), sourceType );
    addItem(newElement);

    if (deleteManualy) {
        deleteItem(source);
    }

    emit dataRestructured();
}

Qt::ItemFlags UBFeaturesModel::flags( const QModelIndex &index ) const
{
    Qt::ItemFlags resultFlags = QAbstractItemModel::flags(index);
	if ( index.isValid() )
	{
		UBFeature item = index.data( Qt::UserRole + 1 ).value<UBFeature>();
        if ( item.getType() == FEATURE_INTERACTIVE
            || item.getType() == FEATURE_ITEM
            || item.getType() == FEATURE_INTERNAL
            || item.getType() == FEATURE_FOLDER)

            resultFlags |= Qt::ItemIsDragEnabled;

        if ( item.isFolder() && !item.getVirtualPath().isNull() )
            resultFlags |= Qt::ItemIsDropEnabled;
    }

    return resultFlags;
}


QStringList UBFeaturesModel::mimeTypes() const
{
	QStringList types;
    types << "text/uri-list" << "image/png" << "image/tiff" << "image/gif" << "image/jpeg";
    return types;
}

int UBFeaturesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid() || !featuresList)
        return 0;
    else
        return featuresList->size();
}

bool UBFeaturesProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	QString path = index.data( Qt::UserRole ).toString();
	
	return filterRegExp().exactMatch(path);
}

bool UBFeaturesSearchProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	/*QString name = sourceModel()->data(index, Qt::DisplayRole).toString();
	eUBLibElementType type = (eUBLibElementType)sourceModel()->data(index, Qt::UserRole + 1).toInt();*/

	UBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<UBFeature>();
    bool isFile = feature.getType() == FEATURE_INTERACTIVE
        || feature.getType() == FEATURE_INTERNAL
        || feature.getType() == FEATURE_ITEM;

	return isFile && filterRegExp().exactMatch( feature.getName() );
}

bool UBFeaturesPathProxyModel::filterAcceptsRow( int sourceRow, const QModelIndex & sourceParent )const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);
	/*QString name = sourceModel()->data(index, Qt::DisplayRole).toString();
	eUBLibElementType type = (eUBLibElementType)sourceModel()->data(index, Qt::UserRole + 1).toInt();*/

	UBFeature feature = sourceModel()->data(index, Qt::UserRole + 1).value<UBFeature>();
	
	return feature.isFolder() && path.startsWith( feature.getFullVirtualPath() );
}

QString	UBFeaturesItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(locale)

    QString text = value.toString();
	if (listView)
	{
		const QFontMetrics fm = listView->fontMetrics();
        const QSize iSize = listView->gridSize();
		return elidedText( fm, iSize.width(), Qt::ElideRight, text );
	}
	return text;
}

UBFeaturesPathItemDelegate::UBFeaturesPathItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
	arrowPixmap = new QPixmap(":images/navig_arrow.png");
}

QString	UBFeaturesPathItemDelegate::displayText ( const QVariant & value, const QLocale & locale ) const
{
    Q_UNUSED(value)
    Q_UNUSED(locale)

    return QString();
}

void UBFeaturesPathItemDelegate::paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
	UBFeature feature = index.data( Qt::UserRole + 1 ).value<UBFeature>();
	QRect rect = option.rect;
	if ( !feature.getFullPath().isEmpty() )
	{
		painter->drawPixmap( rect.left() - 10, rect.center().y() - 5, *arrowPixmap );
	}
	painter->drawPixmap( rect.left() + 5, rect.center().y() - 5, feature.getThumbnail().scaledToHeight( 30, Qt::SmoothTransformation ) );
}

UBFeaturesPathItemDelegate::~UBFeaturesPathItemDelegate()
{
	if ( arrowPixmap )
	{
		delete arrowPixmap;
		arrowPixmap = NULL;
	}
}
