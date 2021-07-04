﻿#include "TransparentMainWindow.h"
#include "SingleApplication.h"
#include <QSystemTrayIcon>
#include <QMenu>
#include <QMessageBox>
#include <QCommandLineParser>
#include <QDesktopServices>
#include <QClipboard>
#include <QMimeData>

int main(int argc,  char**argv)
{
	SingleApplication a(argc, argv);
	
	if (a.instanceRunning())
		return 0;

	auto appIcon = QIcon(":/icon/pin.png");

	QMainWindow mainWindow;
	
	qApp->setWindowIcon(appIcon);
	QSystemTrayIcon systemTray;
	systemTray.setIcon(appIcon);

	std::atomic<bool> clipboardUpdated{ true };
	QObject::connect(qApp->clipboard(), &QClipboard::dataChanged, [&]
		{
			clipboardUpdated = true;
		});
	std::function<TransparentMainWindow* (QByteArray)> createPinWindow;
	std::function<TransparentMainWindow* (QString)> createPinWindowByFile;
	std::function<TransparentMainWindow* (QImage)> createPinWindowByImage;
	std::function<void (const QMimeData*)> createPinWindowByMimeData;
	

	createPinWindowByMimeData = [&](const QMimeData* mimeData)
	{
		bool success = false;
		if (mimeData->hasUrls())
			for (auto url : mimeData->urls())
				if (url.isLocalFile())
					if (createPinWindowByFile(url.toLocalFile()) != nullptr)
						success = true;
		if (!success && mimeData->hasImage())
		{
			createPinWindowByImage(qvariant_cast<QImage>(mimeData->imageData()));
		}
	};

	createPinWindow = [&](QByteArray data)->TransparentMainWindow*
	{
		auto w = new TransparentMainWindow(&mainWindow);
		w->show();
		if (!w->loadMovie(data))
		{
			w->deleteLater();
			return nullptr;
		}
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(QByteArray)const>(&TransparentMainWindow::cloneWindow), createPinWindow);
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(const QMimeData*)const>(&TransparentMainWindow::cloneWindow), createPinWindowByMimeData);
		return w;
	};
	createPinWindowByFile = [&](QString fileName)->TransparentMainWindow*
	{
		auto w = new TransparentMainWindow(&mainWindow);
		w->show();
		if(!w->loadMovie(fileName))
		{
			w->deleteLater();
			return nullptr;
		}
		
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(QByteArray)const>(&TransparentMainWindow::cloneWindow), createPinWindow);
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(const QMimeData*)const>(&TransparentMainWindow::cloneWindow), createPinWindowByMimeData);
		return w;
	};
	createPinWindowByImage = [&](QImage image)
	{
		auto w = new TransparentMainWindow(&mainWindow);
		w->show();
		w->loadMovie(image);
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(QByteArray)const>(&TransparentMainWindow::cloneWindow), createPinWindow);
		QObject::connect(w, static_cast<void(TransparentMainWindow::*)(const QMimeData*)const>(&TransparentMainWindow::cloneWindow), createPinWindowByMimeData);
		return w;
	};
	

	QObject::connect(&a, &SingleApplication::newInstanceStartup, [&](QStringList commandLine)
		{
			const QCommandLineOption inputImageOption("i", QObject::tr("Input Image File Path"), "input");
			QCommandLineParser parser;
			parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
			parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
			parser.addOption(inputImageOption);
			if (parser.parse(commandLine) && parser.isSet(inputImageOption))
				createPinWindowByFile(parser.value(inputImageOption));
			else if (clipboardUpdated)
			{
				clipboardUpdated = false;
				createPinWindowByMimeData(qApp->clipboard()->mimeData());
			}
			else
			{
				systemTray.showMessage(QObject::tr("Invalid parameter"), commandLine.join(' '), appIcon);
			}
		});


	QMenu menu;
	menu.addAction("&Open", [&]
		{
			if (auto fileName = QFileDialog::getOpenFileName(nullptr, "Load image", ""
				, "All Support Image Files(*.bmp;*.jpg;*.jpeg;*.png;*.gif);;Static Images Files(*.bmp;*.jpg;*.jpeg;*.png);;Dynamic Image Files(*.gif);;All Files(*.*)");
				!fileName.isEmpty())
				createPinWindowByFile(fileName);
		});

	menu.addAction("&From Clipboard", [&]
		{
			createPinWindowByMimeData(qApp->clipboard()->mimeData());
		});


	menu.addAction("&Close All", [&]
		{
			auto& children = mainWindow.children();

			for (auto children : children)
			{
				if (auto window = dynamic_cast<TransparentMainWindow*>(children))
				{
					window->close();
				}
			}
		});
	
	menu.addAction("&About", [=]
		{
			QDesktopServices::openUrl(QUrl("https://github.com/Ohto-Ai"));
		});
	
	menu.addAction(menu.style()->standardIcon(QStyle::StandardPixmap::SP_MessageBoxCritical), "&Quit", [=]
		{
			if (QMessageBox::question(nullptr, QObject::tr("Quit"), QObject::tr("Quit the application?")) == QMessageBox::Yes)
				qApp->quit();
		});
	
	systemTray.setContextMenu(&menu);
	
	systemTray.show();


	QCommandLineParser parser;
	parser.addHelpOption();
	parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
	parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
	QCommandLineOption optImage("i", QObject::tr("Input Image File Path"), "input");
	parser.addOption(optImage);
	if (parser.parse(SingleApplication::arguments()) && parser.isSet(optImage))
		createPinWindowByFile(parser.value(optImage));
	else
		createPinWindowByFile(":/icon/C_zerotwo_stand.png");	
	
	qApp->setQuitOnLastWindowClosed(false);

	return a.exec();
}