#ifndef WIDGETTORRNTESCONTROLLER_H
#define WIDGETTORRNTESCONTROLLER_H

#include <QFileInfo>
#include <QWidget>
#include <QTableWidget>
#include "declare_struct.h"

using QString_rwrc = std::reference_wrapper<const QString>;
using QString_crwr = QString_rwrc;

struct TorrentPart
{
	QFileInfo fi;
	QByteArray content;
};

struct IValue
{
	QString value;
	int pos = undefined;
	int len = undefined;

	inline static int undefined = -1;
};

struct TorrentData
{
	QString torrentDownloadName;
	TorrentPart fastresume;
	TorrentPart torrent;
	IValue uploaded;
	QString uploaderReadable;

	QString SetUploadedAndWrite(uint64_t newValue);
};

declare_struct_2_fields_move(TextAndError, QString, text, QString, error);
declare_struct_2_fields_move(IValueAndError, IValue, iValue, QString, error);
declare_struct_2_fields_move(ReadRes, QByteArray, content, bool, success);

class WidgetTorrentsController : public QWidget
{
	Q_OBJECT

public:
	WidgetTorrentsController(QWidget *parent = nullptr);
	~WidgetTorrentsController() = default;

	static ReadRes ReadFile(const QFileInfo &fi);
	static bool WriteFile(const QFileInfo &fi, const QByteArray &content);

private:
	void CreateTableMenu();

	void SlotScan();
	void SlotSetUploaded();

	TorrentData * TorrentByRow(int row, bool showErrorMsg = true);
	TorrentData * CurrentTorrent();

	bool DefineName(const QString &keywordName, TorrentData &torrent, TorrentPart &part, QStringList &errors);
	TextAndError GetBytesValue(const QString &valueName, const QByteArray &content);
	IValueAndError GetIValue(const QString &valueName, const QByteArray &content);

	QTableWidget *table;
	std::map<QString, TorrentData> torrents;
	std::map<QString, TorrentData*> torrentsByDownloadName;
	std::vector<TorrentData*> torrentsInTableOrder;
};
#endif // WIDGETTORRNTESCONTROLLER_H
