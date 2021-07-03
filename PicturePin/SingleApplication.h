#ifndef SINGLEAPPLICATION_H
#define SINGLEAPPLICATION_H

#include <QApplication>

class QLocalServer;

class SingleApplication : public QApplication
{
	Q_OBJECT
public:
	SingleApplication(int& argc, char** argv);
	bool instanceRunning() const;					// ʵ���Ѿ�����
	void receiveNewLocalConnection();				// �յ��µ�����
signals:
	void newInstanceStartup(QString commandLine);	// ��ʵ������
private:
	
	void initLocalConnection();		// ��ʼ����������	
	void createLocalServer();		// ���������
	bool isInstanceRunning;			// �Ƿ��Ѿ���ʵ��������
	QLocalServer* localServer;		// ����socket Server
	QString serverName;				// ��������
};

#endif // SINGLEAPPLICATION_H