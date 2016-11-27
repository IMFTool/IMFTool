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