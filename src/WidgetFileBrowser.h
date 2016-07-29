/* Copyright(C) 2016 Björn Stresing, Denis Manthey, Wolfgang Ruppel
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
#include <QWidget>
#include <QPersistentModelIndex>


class QFileSystemModel;
class QTreeView;
class QFileInfo;

class WidgetFileBrowser : public QWidget {

	Q_OBJECT

public:
	WidgetFileBrowser(QWidget *pParent = NULL);
	virtual ~WidgetFileBrowser();

signals:
	void ExrFileSelectionChanged(const QFileInfo &rFileInfo);

	private slots:
	void FilterTreeViewClicked(const QModelIndex &rIndex);

private:
	Q_DISABLE_COPY(WidgetFileBrowser);
	void InitLayout();
	void InitModel();

	QFileSystemModel			*mpModelFS;
	QTreeView							*mpViewFS;
	QPersistentModelIndex	 mLastIndexSelected;
};
