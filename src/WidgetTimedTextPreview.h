/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel, Krispin Weiss
 *
 * This program is free software : you can redistribute it and / or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.If not, see <http://www.gnu.org/licenses/>.
 */
#pragma once
#include <QObject>
#include <QDebug>
#include <QWidget>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QSplitter>
#include <QStandardItemModel>
#include <QTableView>
#include <QHeaderView>
#include "ImfCommon.h"

class WidgetTimedTextPreview : public QWidget {
	Q_OBJECT
public:
	WidgetTimedTextPreview(QWidget *pParent = NULL);
	QCheckBox *show_regions;
	void ClearTTML();
private:
	void InitLayout();
	QTextEdit *ttml_text;
	QListWidget *ttml_times;
	QLabel *ttml_search_time;
	QCheckBox *wrap_text;
	bool wrap_text_enabled = true; // default
	QPushButton *next;
	QPushButton *prev;
	QString last_tt = "";
	QSplitter *splitter;
	QStandardItemModel *tableModel;
	QTableView *tableView;
	const QVector<visibleTTtrack> *ttmls;
	QFont font_small;
	QFont font_medium;
	QFont font_html;
	int render_style = 2; // 0 : TEXT, 1 : pTEXT, 2 : HTML
	void createButton(int, int, int, int, bool);
	QVector<QPushButton*> highlighted_btns;
	void showSelection(int, int, int);
private slots:
	void prevClicked(bool);
	void nextClicked(bool);
public slots:
	void rShowTTML(const QVector<visibleTTtrack>&, int);
	void wrapTextChanged(int);
	void pushButtonClicked();
signals:
	void PrevNextSubClicked(bool);
};
