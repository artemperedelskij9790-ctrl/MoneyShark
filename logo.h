#ifndef LOGO_H
#define LOGO_H

#include <QIcon>
#include <QPixmap>
#include <QWidget>

QPixmap renderMoneySharkLogo(const QSize &size);
QIcon moneySharkIcon();

class MoneySharkLogoWidget : public QWidget
{
public:
    explicit MoneySharkLogoWidget(QWidget *parent = nullptr);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

protected:
    void paintEvent(QPaintEvent *event) override;
};

#endif // LOGO_H
