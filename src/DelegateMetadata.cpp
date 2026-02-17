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
#include "DelegateMetadata.h"
#include "global.h"
#include "MetadataExtractorCommon.h"
#include <QTextFormat>
#include <QTextDocument>
#include <QTextCursor>
#include <QTextTableCell>
#include <QStyleOptionViewItem>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QTextEdit>


DelegateMetadata::DelegateMetadata(QObject *pParent /*= NULL*/) :
QStyledItemDelegate(pParent) {

	
}

void DelegateMetadata::paint(QPainter *pPainter, const QStyleOptionViewItem &rOption, const QModelIndex &rIndex) const {

	if(rIndex.data(UserRoleMetadata).canConvert<Metadata>()) {
		Metadata metadata = qvariant_cast<Metadata>(rIndex.data(UserRoleMetadata));
		QStyleOptionViewItem options = rOption;
		initStyleOption(&options, rIndex);
		const QWidget *widget = options.widget;
		QStyle *style = widget ? widget->style() : QApplication::style();
		style->drawControl(QStyle::CE_ItemViewItem, &options, pPainter, widget);

		QTextOption text_option;
		text_option.setWrapMode(QTextOption::NoWrap);

		QTextDocument doc;
		doc.setTextWidth(rOption.rect.width());
		doc.setDefaultTextOption(text_option);
		metadata.GetAsTextDocument(doc);

		pPainter->save();
		//pPainter->translate(options.rect.topLeft());
		QRect translated_rect(0, 0, (int)doc.size().width() + .5, (int)doc.size().height() + .5);
		translated_rect.moveCenter(options.rect.center());
		pPainter->translate(translated_rect.topLeft());
		QAbstractTextDocumentLayout::PaintContext ctx;
		ctx.palette = options.palette;
		QRect clip(0, 0, options.rect.width(), options.rect.height());
		pPainter->setClipRect(clip);
		ctx.clip = clip;
		doc.documentLayout()->draw(pPainter, ctx);
		pPainter->restore();
	}
	else {
		QStyledItemDelegate::paint(pPainter, rOption, rIndex);
	}
}

QSize DelegateMetadata::sizeHint(const QStyleOptionViewItem &rOption, const QModelIndex &rIndex) const {

	if(rIndex.data(UserRoleMetadata).canConvert<Metadata>()) {
		Metadata metadata = qvariant_cast<Metadata>(rIndex.data(UserRoleMetadata));
		QTextOption text_option;
		text_option.setWrapMode(QTextOption::NoWrap);
		QTextDocument doc;
		doc.setTextWidth(rOption.rect.width());
		doc.setDefaultTextOption(text_option);
		metadata.GetAsTextDocument(doc);
		QSize size = QSize(doc.size().width(), doc.size().height());
		return size;
	}
	else {
		return QStyledItemDelegate::sizeHint(rOption, rIndex);
	}
}
