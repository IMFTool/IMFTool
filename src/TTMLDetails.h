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

class TTMLDetails : public QWidget {
	Q_OBJECT
public:
	TTMLDetails(QWidget *pParent = NULL);
	QCheckBox *show_regions;
private:
	void InitLayout();
	QTextEdit *ttml_text;
	QLabel *ttml_time;
	QLabel *ttml_search_time;
	QComboBox *render_style;
	QCheckBox *wrap_text;
	QString last_tt = "";
	int lastStyle = 0; // default
	public slots:
	void rShowTTML(const QVector<QString>&, QString, int);
	void renderStyleChanged(int);
	void wrapTextChanged(int);
};
