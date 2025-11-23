#ifndef WIDGETTORRNTESCONTROLLER_H
#define WIDGETTORRNTESCONTROLLER_H

#include <QFileInfo>
#include <QWidget>
#include <QTableWidget>
#include "declare_struct.h"

using QString_rwrc = std::reference_wrapper<const QString>;
using QString_crwr = QString_rwrc;

declare_struct_2_fields_move(TextAndError, QString, text, QString, error);
declare_struct_2_fields_move(ReadRes, QByteArray, content, bool, success);

struct TorrentPart
{
	QFileInfo fi;
	QByteArray content;
};

struct TorrentData
{
	QString torrentDownloadName;
	TorrentPart fastresume;
	TorrentPart torrent;
	QString uploaded;
};

class WidgetTorrentsController : public QWidget
{
	Q_OBJECT

public:
	WidgetTorrentsController(QWidget *parent = nullptr);
	~WidgetTorrentsController() = default;

private:
	void SlotScan();
	ReadRes ReadFile(const QFileInfo &fi);
	bool DefineName(const QString &keywordName, TorrentData &torrent, TorrentPart &part, QStringList &errors);
	TextAndError GetBytesValue(const QString &valueName, const QByteArray &content);
	TextAndError GetIValue(const QString &valueName, const QByteArray &content);

	QTableWidget *table;
};
#endif // WIDGETTORRNTESCONTROLLER_H
