#include "WidgetTorrentsController.h"

#include <QHBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QDebug>
#include <QStandardPaths>

#include "MyQShortings.h"
#include "MyQDifferent.h"
#include "MyQDialogs.h"
#include "MyQFileDir.h"
#include "MyQExecute.h"

QString TorrentData::SetUploadedAndWrite(uint64_t newValue)
{
	if(uploaded.value.isEmpty() or uploaded.len == uploaded.undefined or uploaded.pos == uploaded.undefined)
		return "uploaded value is undefined";

	if(not fastresume.fi.isFile() or fastresume.fi.isSymLink())
		return "fastresume is wrong";

	if(fastresume.content.isEmpty())
		return "fastresume content is empty";

	if(uploaded.pos+uploaded.len >= fastresume.content.size())
		return "fastresume content is to short";

	fastresume.content.replace(uploaded.pos, uploaded.len, QSn(newValue).toUtf8());

	if(not WidgetTorrentsController::WriteFile(fastresume.fi, fastresume.content))
		return "error writing file";

	return "";
}

WidgetTorrentsController::WidgetTorrentsController(QWidget *parent)
	: QWidget(parent)
{
	resize(860, 680);

	QVBoxLayout *vloMain = new QVBoxLayout(this);
	QHBoxLayout *hlo1 = new QHBoxLayout;
	QHBoxLayout *hlo2 = new QHBoxLayout;
	vloMain->addLayout(hlo1);
	vloMain->addLayout(hlo2);

	QPushButton *btnScan = new QPushButton("Scan");
	hlo1->addWidget(btnScan);
	connect(btnScan,&QPushButton::clicked, this, &WidgetTorrentsController::SlotScan);

	QPushButton *btnSetUploaded = new QPushButton("Set uploaded");
	hlo1->addWidget(btnSetUploaded);
	connect(btnSetUploaded,&QPushButton::clicked, this, &WidgetTorrentsController::SlotSetUploaded);

	hlo1->addStretch();

	table = new QTableWidget;
	table->setColumnCount(2);
	table->verticalHeader()->hide();
	QTimer::singleShot(0,[this]()
	{
		auto width = table->viewport()->width();
		table->setColumnWidth(0, width*0.8);
		table->setColumnWidth(1, width*0.15);
	});
	hlo2->addWidget(table);

	CreateTableMenu();
}

QString fastresume = "fastresume";
QString torrent = "torrent";
QString queue = "queue";
QString qBt_name_str = "qBt-name";
QString name_torrent_str = "name";
QString total_uploaded = "total_uploaded";

void WidgetTorrentsController::SlotScan()
{
	QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
	QString qBittorrentPath = appDataPath + "/qBittorrent/BT_backup";

	torrentsInTableOrder.clear();
	torrentsByDownloadName.clear();
	torrents.clear();

	QStringList errors;

	auto files = QDir(qBittorrentPath).entryInfoList({"*."+fastresume, "*."+torrent});
	for(auto &fi:files)
	{
		if(not fi.isFile() or fi.isSymLink()) continue;

		if(fi.suffix() == fastresume)
		{
			auto &torrentRef = torrents[fi.fileName().chopped(fastresume.size()+1)];
			torrentRef.fastresume.fi = std::move(fi);
		}
		else if(fi.suffix() == torrent)
		{
			auto &torrentRef = torrents[fi.fileName().chopped(torrent.size()+1)];
			torrentRef.torrent.fi = std::move(fi);
		}
		else if(fi.fileName() == queue) {}
		else errors += "Unexpacted file: "+fi.fileName();
	}

	if(errors.isEmpty() == false)
	{
		errors.prepend("Errors while scan dir: ");
		MyQDialogs::ShowText(errors);
		errors.clear();
	}

	for(auto &pair:torrents)
	{
		auto &torrent = pair.second;

		int errorsCntPrev = errors.size();
		bool defRes = DefineName(qBt_name_str, torrent, torrent.fastresume, errors);
		if(not defRes)
		{
			if(errorsCntPrev == errors.size())
			{
				bool defRes = DefineName(name_torrent_str, torrent, torrent.torrent, errors);
				if(not defRes)
				{
					errors += "Can't define download name by  files "+torrent.fastresume.fi.fileName()
							+ " and " + torrent.torrent.fi.fileName();
				}
			}
		}

		auto getRes = GetIValue(total_uploaded, torrent.fastresume.content);
		if(getRes.error.isEmpty())
		{
			torrent.uploaded = getRes.iValue;
			torrent.uploaderReadable = MyQString::BytesToString(torrent.uploaded.value.toULongLong());
		}
		else
		{
			errors += "Can't define uploaded by file "+torrent.fastresume.fi.fileName()+" error: "+getRes.error;
			//qdbg << "error "+getRes.error;
		}
		//qdbg << torrent.torrentDownloadName;
		//qdbg << MyQString::BytesToString(torrent.uploaded) << torrent.fastresume.fi.fileName();
		//qdbg << "------------------------------------------------------------------------------------";

	}

	if(errors.isEmpty() == false)
	{
		errors.prepend("Errors while scan files: ");
		MyQDialogs::ShowText(errors);
		errors.clear();
	}

	for(auto &pair:torrents)
	{
		auto &torrent = pair.second;

		int i = 2;
		QString toAdd;
		while(true)
		{
			auto findRes = torrentsByDownloadName.find(torrent.torrentDownloadName.toLower());
			if(findRes != torrentsByDownloadName.end())
			{
				if(toAdd.isEmpty() == false) torrent.torrentDownloadName.chop(toAdd.length());
				toAdd = " - " + QSn(i);
				torrent.torrentDownloadName = torrent.torrentDownloadName + toAdd;
				i++;
			}
			else break;
		}

		torrentsByDownloadName[torrent.torrentDownloadName.toLower()] = &torrent;
	}

	table->setRowCount(0);
	for(auto &pair:torrentsByDownloadName)
	{
		auto &t = *pair.second;

		table->setRowCount(table->rowCount()+1);
		int row = table->rowCount()-1;
		table->setItem(row, 0, new QTableWidgetItem(t.torrentDownloadName));
		table->setItem(row, 1, new QTableWidgetItem(t.uploaderReadable));

		torrentsInTableOrder.push_back(&t);
	}
}

void WidgetTorrentsController::SlotSetUploaded()
{
	if(torrents.empty()) return;

	QStringList torrentsStrs;
	for(auto &t:torrentsInTableOrder) torrentsStrs += t->torrentDownloadName + " (current "+t->uploaderReadable+")";
	auto res = MyQDialogs::CheckBoxDialog("Choose torrents to reset uploaded value", torrentsStrs, {}, {}, false, 1260, 640);

	if(res.accepted == false or res.checkedIndexes.empty()) return;

	QStringList errors;
	for(auto index:res.checkedIndexes)
	{
		auto t = TorrentByRow(index, false);
		if(!t) errors += "row "+QSn(index)+" ("+res.allItems[index].text+") is invalid";
		else
		{
			if(auto setRes = t->SetUploadedAndWrite(0); setRes.isEmpty() == false)
				errors += "Error upload set: "+setRes;
		}
	}

	if(errors.isEmpty()) QMbInfo("Сompleted successfully");
	else
	{
		errors.prepend("Сompleted with errors: ");
		MyQDialogs::ShowText(errors);
	}
}

TorrentData * WidgetTorrentsController::TorrentByRow(int row, bool showErrorMsg)
{
	if(row < 0 or row >= (int)torrentsInTableOrder.size())
	{
		if(showErrorMsg) QMbError("Cant define current torrent: invalid row "+QSn(row));
		return nullptr;
	}

	return torrentsInTableOrder[row];
}

TorrentData * WidgetTorrentsController::CurrentTorrent()
{
	auto row = table->currentRow();
	return TorrentByRow(row);
}

ReadRes WidgetTorrentsController::ReadFile(const QFileInfo & fi)
{
	QFile file(fi.filePath());
	if(file.open(QFile::ReadOnly))
	{
		return ReadRes(file.readAll(), true);
	}

	return ReadRes({},false);
}

bool WidgetTorrentsController::WriteFile(const QFileInfo & fi, const QByteArray & content)
{
	QFile file(fi.filePath());
	if(file.open(QFile::WriteOnly))
	{
		file.write(content);
		return true;
	}
	return false;
}

void WidgetTorrentsController::CreateTableMenu()
{
	table->setContextMenuPolicy(Qt::ActionsContextMenu);

	table->addAction(new QAction("Показать в проводнике", this));
	connect(table->actions().back(), &QAction::triggered, this, [this](){
		auto torrent = CurrentTorrent();
		if(!torrent) return;

		if(not MyQExecute::ShowInExplorer(torrent->fastresume.fi.filePath()))
			QMbError("Can't show file");
	});

	table->addAction(new QAction("Сбросить uploaded", this));
	connect(table->actions().back(), &QAction::triggered, this, [this](){
		auto torrent = CurrentTorrent();
		if(!torrent) return;

		if(auto setRes = torrent->SetUploadedAndWrite(0); setRes.isEmpty() == false)
			QMbError("Error upload set: "+setRes);
		else QMbInfo("Выполнено успешно");
	});
}

bool WidgetTorrentsController::DefineName(const QString & keywordName, TorrentData & torrent, TorrentPart & part, QStringList & errors)
{
	auto readRes = ReadFile(part.fi);
	if(readRes.success)
	{
		part.content = std::move(readRes.content);
		auto getRes = GetBytesValue(keywordName, part.content);
		if(getRes.error.isEmpty())
		{
			if(getRes.text.isEmpty() == false)
			{
				torrent.torrentDownloadName = std::move(getRes.text);
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			errors += "Error GetValue: "+getRes.error+"\n\nfor file "+part.fi.fileName();
		}
	}
	else errors += "Error ReadFile for file "+part.fi.fileName();

	return false;
}

TextAndError WidgetTorrentsController::GetBytesValue(const QString &valueName, const QByteArray & content)
{
	QByteArray valueByArr = valueName.toUtf8();
	auto index = content.indexOf(valueByArr);
	if(index == -1) return TextAndError("",valueName+" not fount");

	index = index+valueByArr.size();
	auto endIndex = content.indexOf(":", index);
	if(endIndex == -1) return TextAndError("","after "+valueName+" : not fount");

	if(content.indexOf(valueByArr, endIndex) != -1) return TextAndError("","duplicates "+valueName);

	auto bytesCnt = content.mid(index, endIndex-index);
	bool ok;
	int bytesCntInt = QString(bytesCnt).toInt(&ok);
	if(not ok) return TextAndError("","after "+valueName+": incorrect value "+QString(bytesCnt));

	if(content.size() <= endIndex+1 + bytesCntInt) return TextAndError("","unepected content end");

	auto name = content.mid(endIndex+1, bytesCntInt);
	return TextAndError(name,"");
}

IValueAndError WidgetTorrentsController::GetIValue(const QString & valueName, const QByteArray & content)
{
	QString valueNameI = valueName + "i";
	QByteArray valueIByArr = valueNameI.toUtf8();
	auto index = content.indexOf(valueIByArr);
	if(index == -1) return IValueAndError({},valueName+" not fount");

	int indexStartIValue = index+valueIByArr.size();
	if(content.size() <= indexStartIValue) return IValueAndError({},"unepected content end");

	auto endIndex = content.indexOf("e8", indexStartIValue);
	if(endIndex == -1) return IValueAndError({},"after "+valueName+"i e8 not fount");

	IValueAndError ret;
	ret.iValue.pos = indexStartIValue;
	ret.iValue.len = endIndex-indexStartIValue;

	auto value = content.mid(ret.iValue.pos, ret.iValue.len);
	QString valueStr = value;
	bool ok;
	QString(valueStr).toULongLong(&ok);
	if(not ok) return IValueAndError({},"after "+valueNameI+" incorrect value "+QString(valueStr));

	ret.iValue.value = std::move(valueStr);

	return ret;
}
