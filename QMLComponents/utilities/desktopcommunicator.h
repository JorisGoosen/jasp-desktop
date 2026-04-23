#ifndef DESKTOPCOMMUNICATOR_H
#define DESKTOPCOMMUNICATOR_H

#include <QObject>
#include <condition_variable>
#include <mutex>

///This class only exists to allow signal-slot connections to be made between certain classes in Desktop and in QMLComponents.
/// And to easily split that off when building for R -only
class DesktopCommunicator : public QObject
{
	Q_OBJECT
public:
	explicit DesktopCommunicator(QObject *parent = nullptr);
	
	static DesktopCommunicator * singleton();

	bool useNativeFileDialog();
	bool engineSandbox();
	bool queryEncryptionSettings(bool readingMode = false);
	char askCsvDelimiter(char autoDelimiter);

signals:
	void queryEncryptionSettingsSignal(bool readingMode);
	void askCsvDelimiterSignal(char autoDelimiter);
	void currentJaspThemeChanged();
	void uiScaleChanged();
	void interfaceFontChanged();
	bool useNativeFileDialogSignal(); //< For internal use only, `bool useNativeFileDialog();` is what you want
	bool engineSandboxSignal();

public slots:
	void encryptionSettingsQueryComplete(bool submit);
	void delimiterChosen(char delimiter);

private:
	static DesktopCommunicator * _singleton;

	bool						_queryCondition	= false, // against spurious wakeup
								_querySubmitted	= false,
								_csvCondition	= false;
	char						_csvSubmitted	= '\0';
	std::mutex					_queryLock,
								_csvLock;
	std::condition_variable		_query_cv,
								_csv_cv;
};

#endif // DESKTOPCOMMUNICATOR_H
