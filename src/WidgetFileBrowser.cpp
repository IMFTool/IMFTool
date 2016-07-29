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
#include "WidgetFileBrowser.h"
#include "global.h"
#include <QFileSystemModel>
#include <QTreeView>
#include <QHBoxLayout>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <QHeaderView>


WidgetFileBrowser::WidgetFileBrowser(QWidget *pParent /*= NULL*/) :
QWidget(pParent), mpModelFS(NULL), mpViewFS(NULL), mLastIndexSelected() {

	InitLayout();
	InitModel();
}

WidgetFileBrowser::~WidgetFileBrowser() {

}

void WidgetFileBrowser::InitLayout() {

	mpViewFS = new QTreeView(this);
	QHBoxLayout *p_layout = new QHBoxLayout();
	p_layout->setMargin(0);
	p_layout->addWidget(mpViewFS);
	setLayout(p_layout);

	connect(mpViewFS, SIGNAL(activated(const QModelIndex &)), this, SLOT(FilterTreeViewClicked(const QModelIndex &)));
	connect(mpViewFS, SIGNAL(clicked(const QModelIndex &)), this, SLOT(FilterTreeViewClicked(const QModelIndex &)));
}

void WidgetFileBrowser::InitModel() {

	mpModelFS = new QFileSystemModel(this);
	mpModelFS->setNameFilterDisables(true);
	mpModelFS->setNameFilters(QStringList("*.exr"));
	mpModelFS->setIconProvider(new IconProviderExrWav(mpModelFS));
	mpModelFS->setRootPath(QDir::homePath());
	mLastIndexSelected = mpModelFS->index(QDir::currentPath());
	mpViewFS->setModel(mpModelFS);
	mpViewFS->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
	mpViewFS->setColumnHidden(2, true);
}

void WidgetFileBrowser::FilterTreeViewClicked(const QModelIndex & rIndex) {

	QFileInfo info = mpModelFS->fileInfo(rIndex);
	if(info.suffix() == "exr") {
		if(mLastIndexSelected.isValid()) {
			if(mLastIndexSelected.parent() != rIndex.parent() || mLastIndexSelected.row() != rIndex.row()) {
				mLastIndexSelected = rIndex;
				emit ExrFileSelectionChanged(info);
			}
		}
		else {
			mLastIndexSelected = mpModelFS->index(QDir::homePath());
		}
	}
}
