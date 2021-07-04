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

	qApp->setWindowIcon(appIcon);
	QSystemTrayIcon systemTray;
	systemTray.setIcon(appIcon);

	std::atomic<bool> clipboardUpdated{ true };
	QObject::connect(qApp->clipboard(), &QClipboard::dataChanged, [&]
		{
			clipboardUpdated = true;
		});
	
	std::function<TransparentMainWindow*(QString)> createPinWindow = [&](QString fileName)->TransparentMainWindow*
	{
		auto w = new TransparentMainWindow();
		w->show();
		if(!w->loadMovie(fileName))
		{
			w->deleteLater();
			return nullptr;
		}
		QObject::connect(w, &TransparentMainWindow::cloneWindow, createPinWindow);
		return w;
	};
	std::function<TransparentMainWindow*(QImage)> createPinWindowByImage = [&](QImage image)
	{
		auto w = new TransparentMainWindow();
		w->show();
		w->loadMovie(image);
		QObject::connect(w, &TransparentMainWindow::cloneWindow, createPinWindow);
		return w;
	};
	

	QObject::connect(&a, &SingleApplication::newInstanceStartup, [&](QString commandLine)
		{
			const QCommandLineOption inputImageOption("i", QObject::tr("Input Image File Path"), "input");
			QCommandLineParser parser;
			parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsLongOptions);
			parser.setOptionsAfterPositionalArgumentsMode(QCommandLineParser::ParseAsPositionalArguments);
			parser.addOption(inputImageOption);
			if (parser.parse(commandLine.split(' ')) && parser.isSet(inputImageOption))
				createPinWindow(parser.value(inputImageOption));
			else if (clipboardUpdated)
			{
				clipboardUpdated = false;
				
				const auto mimeData = qApp->clipboard()->mimeData();
				
				bool success = false;
				if (mimeData->hasUrls())
					for (auto url : mimeData->urls())
						if (createPinWindow(url.toLocalFile()) != nullptr)
							success = true;
				if (!success && mimeData->hasImage())
				{
					createPinWindowByImage(qvariant_cast<QImage>(mimeData->imageData()));
				}
			}
			else
				systemTray.showMessage(QObject::tr("Invalid parameter"), commandLine, appIcon);
		});


	QMenu menu;
	menu.addAction("&Load", [&]
		{
			auto fileName = QFileDialog::getOpenFileName(nullptr, "Load image", ""
				, "All Support Image Files(*.bmp;*.jpg;*.jpeg;*.png;*.gif);;Static Images Files(*.bmp;*.jpg;*.jpeg;*.png);;Dynamic Image Files(*.gif);;All Files(*.*)");
			if (!fileName.isEmpty())
			{
				auto w = new TransparentMainWindow();
				w->show();
				w->loadMovie(fileName);

				QObject::connect(w, &TransparentMainWindow::cloneWindow, createPinWindow);
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
		createPinWindow(parser.value(optImage));
	else
		createPinWindow(":/icon/C_zerotwo_stand.png");	
	
	qApp->setQuitOnLastWindowClosed(false);

	return a.exec();
}